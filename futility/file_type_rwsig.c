/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Some instances of the Chrome OS embedded controller firmware can't do a
 * normal software sync handshake at boot, but will verify their own RW images
 * instead. This is typically done by putting a struct vb2_packed_key in the RO
 * image and a corresponding struct vb2_signature in the RW image.
 *
 * This file provides the basic implementation for that approach.
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "file_type.h"
#include "futility.h"
#include "futility_options.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "util_misc.h"

/*
 * Reserved space for the public key and signature. This may not be enough for
 * larger key sizes since the vb2 structs are more than just the raw bits.
 */
#define PUBKEY_RSVD_SIZE    2048
#define SIGNATURE_RSVD_SIZE 1024

/* True if start + size > max */
static int bigger_than(uint32_t start, uint32_t size, uint32_t max)
{
	int r = start > max || size > max || start > max - size;
	if (r)
		Debug("%s: 0x%x + 0x%x > 0x%x\n", __func__, start, size, max);
	return r;
}

/* True if one region overlaps the other */
static int overlaps(uint32_t start_a, uint32_t size_a,
		    uint32_t start_b, uint32_t size_b)
{
	if (start_a < start_b && start_a <= start_b - size_a)
		return 0;
	if (start_b < start_a && start_b <= start_a - size_b)
		return 0;
	Debug("%s: 0x%x + 0x%x overlaps 0x%x + 0x%x\n",
	      __func__, start_a, size_a, start_b, size_b);
	return 1;
}

/* Return 1 if okay, 0 if not */
static int parse_size_opts(const uint8_t *buf, uint32_t len,
			   uint32_t *rw_offset_ptr, uint32_t *rw_size_ptr,
			   uint32_t *pkey_offset_ptr, uint32_t *sig_offset_ptr)
{
	uint32_t rw_offset, rw_size, pkey_offset, sig_offset;

	/* Start with defaults */

	/* The image has both RO and RW, evenly split, RO first. */
	rw_size = rw_offset = len / 2;

	/* The public key is up against the end of the RO half */
	pkey_offset = rw_offset - PUBKEY_RSVD_SIZE;

	/* The signature key is up against the end of the whole image */
	sig_offset = len - SIGNATURE_RSVD_SIZE;

	/* The RW image to be signed doesn't include the signature */
	rw_size -= SIGNATURE_RSVD_SIZE;

	/* FIXME: Override the defaults here by looking for an FMAP or similar
	 * structure telling us where the parts are. */

	/* We can override any of that with explicit args */
	if (sign_option.rw_offset != 0xffffffff)
		rw_offset = sign_option.rw_offset;
	if (sign_option.rw_size != 0xffffffff)
		rw_size = sign_option.rw_size;
	if (sign_option.pkey_offset != 0xffffffff)
		pkey_offset = sign_option.pkey_offset;
	if (sign_option.sig_offset != 0xffffffff)
		sig_offset = sign_option.sig_offset;

	Debug("pkey_offset 0x%08x\n", pkey_offset);
	Debug("rw_offset   0x%08x\n", rw_offset);
	Debug("rw_size     0x%08x\n", rw_size);
	Debug("sig_offset  0x%08x\n", sig_offset);

	/* Now let's do some sanity checks. */
	if (bigger_than(rw_offset, rw_size, len) ||
	    overlaps(rw_offset, rw_size, pkey_offset, PUBKEY_RSVD_SIZE) ||
	    overlaps(rw_offset, rw_size, sig_offset, SIGNATURE_RSVD_SIZE) ||
	    overlaps(pkey_offset, PUBKEY_RSVD_SIZE,
		     sig_offset, SIGNATURE_RSVD_SIZE)) {
		printf("size/offset values are bogus\n");
		return 0;
	}

	*rw_offset_ptr = rw_offset;
	*rw_size_ptr = rw_size;
	*pkey_offset_ptr = pkey_offset;
	*sig_offset_ptr = sig_offset;

	return 1;
}

int ft_sign_rwsig(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	struct vb2_signature *sig = 0;
	int retval = 1;
	uint32_t rw_offset, rw_size;		/* what to sign */
	uint32_t pkey_offset, sig_offset;	/* where to put blobs */
	uint32_t r;

	Debug("%s(): name %s\n", __func__, name);
	Debug("%s(): len  0x%08x (%d)\n", __func__, len, len);

	/* Figure out what to sign and where to put the blobs */
	if (!parse_size_opts(buf, len,
			     &rw_offset, &rw_size,
			     &pkey_offset, &sig_offset))
		goto done;

	/* Sign the blob */
	r = vb2_sign_data(&sig, buf + rw_offset, rw_size,
			  sign_option.prikey, 0);
	if (r) {
		fprintf(stderr,
			"Unable to sign data (error 0x%08x, if that helps)\n",
			r);
		goto done;
	}

	Debug("sig_offset   0x%08x\n", sig_offset);
	Debug("sig_size     0x%08x\n", sig->c.total_size);

	if (sig->c.total_size > SIGNATURE_RSVD_SIZE)
		fprintf(stderr, "WARNING: The signature may be too large"
			" (0x%08x > %08x)\n",
			sig->c.total_size, SIGNATURE_RSVD_SIZE);

	/* Update the signature */
	memcpy(buf + sig_offset, sig, sig->c.total_size);

	/* If weren't given a public key, we're done */
	if (!sign_option.pkey) {
		fprintf(stderr, "No public key given; not updating RO\n");
		retval = 0;
		goto done;
	}

	Debug("pkey_offset  0x%08x\n", pkey_offset);
	Debug("pkey_size    0x%08x\n", sign_option.pkey->c.total_size);

	if (sign_option.pkey->c.total_size > PUBKEY_RSVD_SIZE)
		fprintf(stderr, "WARNING: The public key may be too large"
			" (0x%08x > %08x)\n",
			sign_option.pkey->c.total_size, PUBKEY_RSVD_SIZE);

	/* Update the public key */
	memcpy(buf + pkey_offset, sign_option.pkey,
	       sign_option.pkey->c.total_size);

	/* Finally */
	retval = 0;
done:
	if (sign_option.prikey)
		vb2_private_key_free(sign_option.prikey);
	if (sign_option.pkey)
		free(sign_option.pkey);

	return retval;
}
