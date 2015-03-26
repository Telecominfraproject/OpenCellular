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
 * new devices.
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

	/*
	 * Check for size args. Note that we're NOT worrying about rollover,
	 * overlapping regions, out of bounds, etc.
	 */
	ro_offset = 0;
	ro_size = rw_size = rw_offset = len / 2;

	/* Override some stuff? */
	if (sign_option.ro_size != 0xffffffff)
		ro_size = sign_option.ro_size;
	if (sign_option.rw_size != 0xffffffff)
		rw_size = sign_option.rw_size;

	Debug("ro_size     0x%08x\n", ro_size);
	Debug("ro_offset   0x%08x\n", ro_offset);

	/* If RO is missing, the whole thing must be RW */
	if (!ro_size) {
		rw_size = len;
		rw_offset = 0;
	}

	/* Unless that's overridden too */
	if (sign_option.ro_offset != 0xffffffff)
		ro_offset = sign_option.ro_offset;
	if (sign_option.rw_offset != 0xffffffff)
		rw_offset = sign_option.rw_offset;

	Debug("rw_size     0x%08x\n", rw_size);
	Debug("rw_offset   0x%08x\n", rw_offset);

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
