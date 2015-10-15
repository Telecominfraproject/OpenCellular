/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * The USB Type-C chargers released with Samus ("Pixel (2015)") have upgradable
 * firmware. Due to space considerations, we don't have room for handy things
 * like an FMAP or headers for the signatures. Accordingly, all the normally
 * variable factors (image size, signature algorithms, etc.) are hard coded
 * and the image itself just looks like a bunch of random numbers.
 *
 * This file handles those images, but PLEASE don't use it as a template for
 * new devices. Look at file_type_rwsig.c instead.
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "file_type.h"
#include "futility.h"
#include "futility_options.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "util_misc.h"

/* Return 1 if okay, 0 if not */
static int parse_size_opts(uint32_t len,
			   uint32_t *ro_size_ptr, uint32_t *rw_size_ptr,
			   uint32_t *ro_offset_ptr, uint32_t * rw_offset_ptr)
{
	uint32_t ro_size, rw_size, ro_offset, rw_offset;

	/* Assume the image has both RO and RW, evenly split. */
	ro_offset = 0;
	ro_size = rw_size = rw_offset = len / 2;

	/* Unless told otherwise... */
	if (sign_option.ro_size != 0xffffffff)
		ro_size = sign_option.ro_size;
	if (sign_option.ro_offset != 0xffffffff)
		ro_offset = sign_option.ro_offset;

	/* If RO is missing, the whole thing must be RW */
	if (!ro_size) {
		rw_size = len;
		rw_offset = 0;
	}

	/* Unless that's overridden too */
	if (sign_option.rw_size != 0xffffffff)
		rw_size = sign_option.rw_size;
	if (sign_option.rw_offset != 0xffffffff)
		rw_offset = sign_option.rw_offset;

	Debug("ro_size     0x%08x\n", ro_size);
	Debug("ro_offset   0x%08x\n", ro_offset);
	Debug("rw_size     0x%08x\n", rw_size);
	Debug("rw_offset   0x%08x\n", rw_offset);

	/* Now let's do some sanity checks. */
	if (ro_size > len || ro_offset > len - ro_size ||
	    rw_size > len || rw_offset > len - rw_size) {
		printf("size/offset values are bogus\n");
		return 0;
	}

	*ro_size_ptr = ro_size;
	*rw_size_ptr = rw_size;
	*ro_offset_ptr = ro_offset;
	*rw_offset_ptr = rw_offset;

	return 1;
}

int ft_sign_usbpd1(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	struct vb2_private_key *key_ptr = 0;
	struct vb2_signature *sig_ptr = 0;
	uint8_t *keyb_data = 0;
	uint32_t keyb_size;
	int retval = 1;
	uint32_t sig_size;
	uint32_t sig_offset;
	uint32_t pub_size;
	uint32_t pub_offset;
	uint32_t ro_size;
	uint32_t rw_size;
	uint32_t ro_offset;
	uint32_t rw_offset;
	uint32_t r;

	Debug("%s(): name %s\n", __func__, name);
	Debug("%s(): len  0x%08x (%d)\n", __func__, len, len);

	/* Get image locations */
	if (!parse_size_opts(len, &ro_size, &rw_size, &ro_offset, &rw_offset))
		goto done;

	/* Read the signing keypair file */
	if (vb2_private_key_read_pem(&key_ptr, sign_option.pem_signpriv)) {
		fprintf(stderr, "Unable to read keypair from %s\n",
			sign_option.pem_signpriv);
		goto done;
	}

	/* Set the algs */
	key_ptr->hash_alg = sign_option.hash_alg;
	key_ptr->sig_alg = vb2_rsa_sig_alg(key_ptr->rsa_private_key);
	if (key_ptr->sig_alg == VB2_SIG_INVALID) {
		fprintf(stderr, "Unsupported sig algorithm in RSA key\n");
		goto done;
	}

	/* Figure out what needs signing */
	sig_size = vb2_rsa_sig_size(key_ptr->sig_alg);
	if (rw_size < sig_size) {
		fprintf(stderr,
			"The RW image is too small to hold the signature"
			" (0x%08x < %08x)\n", rw_size, sig_size);
		goto done;
	}
	rw_size -= sig_size;
	sig_offset = rw_offset + rw_size;

	Debug("rw_size   => 0x%08x\n", rw_size);
	Debug("rw_offset => 0x%08x\n", rw_offset);
	Debug("sig_size     0x%08x\n", sig_size);
	Debug("sig_offset   0x%08x\n", sig_offset);

	/* Sign the blob */
	r = vb2_sign_data(&sig_ptr, buf + rw_offset, rw_size, key_ptr, "Bah");
	if (r) {
		fprintf(stderr,
			"Unable to sign data (error 0x%08x, if that helps)\n",
			r);
		goto done;
	}

	/* Double-check the size */
	if (sig_ptr->sig_size != sig_size) {
		fprintf(stderr,
			"ERROR: sig size is %d bytes, not %d as expected.\n",
			sig_ptr->sig_size, sig_size);
		goto done;
	}

	/* Okay, looking good. Update the signature. */
	memcpy(buf + sig_offset,
	       (uint8_t *)sig_ptr + sig_ptr->sig_offset,
	       sig_ptr->sig_size);


	/* If there's no RO section, we're done. */
	if (!ro_size) {
		retval = 0;
		goto done;
	}

	/* Otherwise, now update the public key */
	if (vb_keyb_from_rsa(key_ptr->rsa_private_key,
			     &keyb_data, &keyb_size)) {
		fprintf(stderr, "Couldn't extract the public key\n");
		goto done;
	}
	Debug("keyb_size is 0x%x (%d):\n", keyb_size, keyb_size);

	/*
	 * Of course the packed public key format is different. Why would you
	 * think otherwise? Since the dawn of time, vboot has used this:
	 *
	 *   uint32_t  nwords        size of RSA key in 32-bit words
	 *   uint32_t  n0inv         magic RSA n0inv
	 *   uint32_t  n[nwords]     magic RSA modulus little endian array
	 *   uint32_t  rr[nwords]    magic RSA R^2 little endian array
	 *
	 * But for no discernable reason, the usbpd1 format uses this:
	 *
	 *   uint32_t  n[nwords]     magic RSA modulus little endian array
	 *   uint32_t  rr[nwords]    magic RSA R^2 little endian array
	 *   uint32_t  n0inv         magic RSA n0inv
	 *
	 * There's no nwords field, and n0inv is last insted of first. Sigh.
	 */
	pub_size = keyb_size - 4;

	/* align pubkey size to 16-byte boundary */
	uint32_t pub_pad = pub_size;
	pub_size = (pub_size + 16) / 16 * 16;
	pub_pad = pub_size - pub_pad;

	pub_offset = ro_offset + ro_size - pub_size;

	if (ro_size < pub_size) {
		fprintf(stderr,
			"The RO image is too small to hold the public key"
			" (0x%08x < %08x)\n", ro_size, pub_size);
		goto done;
	}

	/* How many bytes in the arrays? */
	uint32_t nbytes = 4 * (*(uint32_t *)keyb_data);
	/* Source offsets from keyb_data */
	uint32_t src_ofs_n0inv = 4;
	uint32_t src_ofs_n = src_ofs_n0inv + 4;
	uint32_t src_ofs_rr = src_ofs_n + nbytes;
	/* Dest offsets from buf */
	uint32_t dst_ofs_n = pub_offset + 0;
	uint32_t dst_ofs_rr = dst_ofs_n + nbytes;
	uint32_t dst_ofs_n0inv = dst_ofs_rr + nbytes;

	Debug("len 0x%08x ro_size 0x%08x ro_offset 0x%08x\n",
	       len, ro_size, ro_offset);
	Debug("pub_size 0x%08x pub_offset 0x%08x nbytes 0x%08x\n",
	       pub_size, pub_offset, nbytes);
	Debug("pub_pad 0x%08x\n", pub_pad);

	/* Copy n[nwords] */
	memcpy(buf + dst_ofs_n,
	       keyb_data + src_ofs_n,
	       nbytes);
	/* Copy rr[nwords] */
	memcpy(buf + dst_ofs_rr,
	       keyb_data + src_ofs_rr,
	       nbytes);
	/* Copy n0inv */
	memcpy(buf + dst_ofs_n0inv,
	       keyb_data + src_ofs_n0inv,
	       4);
	/* Pad with 0xff */
	memset(buf + dst_ofs_n0inv + 4, 0xff, pub_pad);

	/* Finally */
	retval = 0;
done:
	if (key_ptr)
		vb2_private_key_free(key_ptr);
	if (keyb_data)
		free(keyb_data);

	return retval;
}


/*
 * Algorithms that we want to try, in order. We've only ever shipped with
 * RSA2048 / SHA256, but the others should work in tests.
 */
static enum vb2_signature_algorithm sigs[] = {
	VB2_SIG_RSA2048,
	VB2_SIG_RSA1024,
	VB2_SIG_RSA4096,
	VB2_SIG_RSA8192,
};
static enum vb2_hash_algorithm hashes[] = {
	VB2_HASH_SHA256,
	VB2_HASH_SHA1,
	VB2_HASH_SHA512,
};

/*
 * The size of the public key structure used by usbpd1 is
 * 2 x RSANUMBYTES for n and rr fields
 * plus 4 for n0inv, aligned on a multiple of 16
 */
static uint32_t usbpd1_packed_key_size(enum vb2_signature_algorithm sig_alg)
{
	switch (sig_alg) {
	case VB2_SIG_RSA1024:
		return 272;
	case VB2_SIG_RSA2048:
		return 528;
	case VB2_SIG_RSA4096:
		return 1040;
	case VB2_SIG_RSA8192:
		return 2064;
	default:
		return 0;
	}
}
static void vb2_pubkey_from_usbpd1(struct vb2_public_key *key,
				   enum vb2_signature_algorithm sig_alg,
				   enum vb2_hash_algorithm hash_alg,
				   const uint8_t *o_pubkey,
				   uint32_t o_pubkey_size)
{
	key->arrsize = vb2_rsa_sig_size(sig_alg) / sizeof(uint32_t);
	key->n0inv = *((uint32_t *)o_pubkey + 2 * key->arrsize);
	key->n = (uint32_t *)o_pubkey;
	key->rr = (uint32_t *)o_pubkey + key->arrsize;
	key->sig_alg = sig_alg;
	key->hash_alg = hash_alg;
	key->desc = 0;
	key->version = 0;
	key->id = vb2_hash_id(hash_alg);
}

static int vb2_sig_from_usbpd1(struct vb2_signature **sig,
			       enum vb2_signature_algorithm sig_alg,
			       enum vb2_hash_algorithm hash_alg,
			       const uint8_t *o_sig,
			       uint32_t o_sig_size,
			       uint32_t data_size)
{
	struct vb2_signature s = {
		.c.magic = VB2_MAGIC_SIGNATURE,
		.c.struct_version_major = VB2_SIGNATURE_VERSION_MAJOR,
		.c.struct_version_minor = VB2_SIGNATURE_VERSION_MINOR,
		.c.fixed_size = sizeof(s),
		.sig_alg = sig_alg,
		.hash_alg = hash_alg,
		.data_size = data_size,
		.sig_size = vb2_rsa_sig_size(sig_alg),
		.sig_offset = sizeof(s),
	};
	uint32_t total_size = sizeof(s) + o_sig_size;
	uint8_t *buf = calloc(1, total_size);
	if (!buf)
		return VB2_ERROR_UNKNOWN;

	memcpy(buf, &s, sizeof(s));
	memcpy(buf + sizeof(s), o_sig, o_sig_size);

	*sig = (struct vb2_signature *)buf;
	return VB2_SUCCESS;
}

static void show_usbpd1_stuff(const char *name,
			      enum vb2_signature_algorithm sig_alg,
			      enum vb2_hash_algorithm hash_alg,
			      const uint8_t *o_pubkey, uint32_t o_pubkey_size)
{
	struct vb2_public_key key;
	struct vb2_packed_key *pkey;
	uint8_t *sha1sum;
	int i;

	vb2_pubkey_from_usbpd1(&key, sig_alg, hash_alg,
			       o_pubkey, o_pubkey_size);

	if (vb2_public_key_pack(&pkey, &key))
		return;

	sha1sum = DigestBuf((uint8_t *)pkey + pkey->key_offset,
			pkey->key_size, SHA1_DIGEST_ALGORITHM);

	printf("USB-PD v1 image:       %s\n", name);
	printf("  Algorithm:           %s %s\n",
	       vb2_lookup_by_num(vb2_text_vs_sig, sig_alg)->name,
	       vb2_lookup_by_num(vb2_text_vs_hash, hash_alg)->name);
	printf("  Key sha1sum:         ");
	for (i = 0; i < SHA1_DIGEST_SIZE; i++)
		printf("%02x", sha1sum[i]);
	printf("\n");

	free(sha1sum);
	free(pkey);
}


/* Returns VB2_SUCCESS or random error code */
static int try_our_own(enum vb2_signature_algorithm sig_alg,
		       enum vb2_hash_algorithm hash_alg,
		       const uint8_t *o_pubkey, uint32_t o_pubkey_size,
		       const uint8_t *o_sig, uint32_t o_sig_size,
		       const uint8_t *data, uint32_t data_size)
{
	struct vb2_public_key pubkey;
	struct vb2_signature *sig;
	uint8_t buf[VB2_WORKBUF_RECOMMENDED_SIZE]
		__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb = {
		.buf = buf,
		.size = sizeof(buf),
	};
	int rv = VB2_ERROR_UNKNOWN;

	vb2_pubkey_from_usbpd1(&pubkey, sig_alg, hash_alg,
			       o_pubkey, o_pubkey_size);

	if ((rv = vb2_sig_from_usbpd1(&sig, sig_alg, hash_alg,
				      o_sig, o_sig_size, data_size)))
	    return rv;

	rv = vb2_verify_data(data, data_size, sig, &pubkey, &wb);

	free(sig);

	return rv;
}

/* Returns VB2_SUCCESS if the image validates itself */
static int check_self_consistency(const uint8_t *buf,
				  const char *name,
				  uint32_t ro_size, uint32_t rw_size,
				  uint32_t ro_offset, uint32_t rw_offset,
				  enum vb2_signature_algorithm sig_alg,
				  enum vb2_hash_algorithm hash_alg)
{
	/* Where are the important bits? */
	uint32_t sig_size = vb2_rsa_sig_size(sig_alg);
	uint32_t sig_offset = rw_offset + rw_size - sig_size;
	uint32_t pubkey_size = usbpd1_packed_key_size(sig_alg);
	uint32_t pubkey_offset = ro_offset + ro_size - pubkey_size;
	int rv;

	/* Skip stuff that obviously doesn't work */
	if (sig_size > rw_size || pubkey_size > ro_size)
		return VB2_ERROR_UNKNOWN;

	rv = try_our_own(sig_alg, hash_alg,		   /* algs */
			 buf + pubkey_offset, pubkey_size, /* pubkey blob */
			 buf + sig_offset, sig_size,	   /* sig blob */
			 buf + rw_offset, rw_size - sig_size); /* RW image */

	if (rv == VB2_SUCCESS && name)
		show_usbpd1_stuff(name, sig_alg, hash_alg,
				  buf + pubkey_offset, pubkey_size);

	return rv;
}


int ft_show_usbpd1(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	uint32_t ro_size, rw_size, ro_offset, rw_offset;
	int s, h;

	Debug("%s(): name %s\n", __func__, name);
	Debug("%s(): len  0x%08x (%d)\n", __func__, len, len);

	/* Get image locations */
	if (!parse_size_opts(len, &ro_size, &rw_size, &ro_offset, &rw_offset))
		return 1;

	/* TODO: If we don't have a RO image, ask for a public key
	 * TODO: If we're given an external public key, use it (and its alg) */
	if (!ro_size) {
		printf("Can't find the public key\n");
		return 1;
	}

	/* TODO: Only loop through the numbers we haven't been given */
	for (s = 0; s < ARRAY_SIZE(sigs); s++)
		for (h = 0; h < ARRAY_SIZE(hashes); h++)
			if (!check_self_consistency(buf, name,
						    ro_size, rw_size,
						    ro_offset, rw_offset,
						    sigs[s], hashes[h]))
				return 0;

	printf("This doesn't appear to be a complete usbpd1 image\n");
	return 1;
}

enum futil_file_type ft_recognize_usbpd1(uint8_t *buf, uint32_t len)
{
	uint32_t ro_size, rw_size, ro_offset, rw_offset;
	int s, h;

	/*
	 * Since we don't use any headers to identify or locate the pubkey and
	 * signature, in order to identify blob as the right type we have to
	 * just assume that the RO & RW are 1) both present, and 2) evenly
	 * split. Then we just try to use what we think might be the pubkey to
	 * validate what we think might be the signature.
	 */
	ro_offset = 0;
	ro_size = rw_size = rw_offset = len / 2;

	for (s = 0; s < ARRAY_SIZE(sigs); s++)
		for (h = 0; h < ARRAY_SIZE(hashes); h++)
			if (!check_self_consistency(buf, 0,
						    ro_size, rw_size,
						    ro_offset, rw_offset,
						    sigs[s], hashes[h]))
				return FILE_TYPE_USBPD1;

	return FILE_TYPE_UNKNOWN;
}
