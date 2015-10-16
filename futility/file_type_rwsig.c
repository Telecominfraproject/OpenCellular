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

#define SIGNATURE_RSVD_SIZE 1024

static inline void vb2_print_bytes(const void *ptr, uint32_t len)
{
	const uint8_t *buf = (const uint8_t *)ptr;
	int i;
	for (i = 0; i < len; i++)
		printf("%02x", *buf++);
}

static void show_sig(const char *name, const struct vb2_signature *sig)
{
	const struct vb2_text_vs_enum *entry;
	printf("Signature:             %s\n", name);
	printf("  Vboot API:           2.1\n");
	printf("  Desc:                \"%s\"\n", vb2_common_desc(sig));
	entry = vb2_lookup_by_num(vb2_text_vs_sig, sig->sig_alg);
	printf("  Signature Algorithm: %d %s\n", sig->sig_alg,
	       entry ? entry->name : "(invalid)");
	entry = vb2_lookup_by_num(vb2_text_vs_hash, sig->hash_alg);
	printf("  Hash Algorithm:      %d %s\n", sig->hash_alg,
	       entry ? entry->name : "(invalid)");
	printf("  Total size:          0x%x (%d)\n", sig->c.total_size,
	       sig->c.total_size);
	printf("  ID:                  ");
	vb2_print_bytes(&sig->id, sizeof(sig->id));
	printf("\n");
	printf("  Data size:           0x%x (%d)\n", sig->data_size,
	       sig->data_size);
}

int ft_show_rwsig(const char *name, uint8_t *buf, uint32_t len, void *nuthin)
{
	const struct vb2_signature *sig = 0;
	struct vb2_public_key key;
	uint8_t workbuf[VB2_VERIFY_DATA_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;
	uint32_t data_size, sig_size = SIGNATURE_RSVD_SIZE;
	uint8_t *data;

	Debug("%s(): name %s\n", __func__, name);
	Debug("%s(): len  0x%08x (%d)\n", __func__, len, len);

	/* Am I just looking at a signature file? */
	Debug("Looking for signature at 0x0\n");
	sig = (const struct vb2_signature *)buf;
	if (VB2_SUCCESS == vb2_verify_signature(sig, len)) {
		show_sig(name, sig);
		if (!show_option.fv) {
			printf("No data available to verify\n");
			return show_option.strict;
		}
		data = show_option.fv;
		data_size = show_option.fv_size;
	} else {
		/* Where would it be? */
		if (show_option.sig_size)
			sig_size = show_option.sig_size;

		Debug("Looking for signature at 0x%x\n", len - sig_size);

		if (len < sig_size) {
			Debug("File is too small\n");
			return 1;
		}

		sig = (const struct vb2_signature *)(buf + len - sig_size);
		if (VB2_SUCCESS == vb2_verify_signature(sig, sig_size)) {
			show_sig(name, sig);
			data = buf;
			data_size = sig->data_size;
		} else {
			return 1;
		}
	}

	if (!show_option.pkey) {
		printf("No public key available to verify with\n");
		return show_option.strict;
	}

	/* We already did this once, so it should work again */
	if (vb2_unpack_key(&key,
			   (const uint8_t *)show_option.pkey,
			   show_option.pkey->c.total_size)) {
		Debug("Can't unpack pubkey\n");
		return 1;
	}

	/* The sig is destroyed by the verify operation, so make a copy */
	{
		uint8_t sigbuf[sig->c.total_size];
		memcpy(sigbuf, sig, sizeof(sigbuf));

		vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

		if (vb2_verify_data(data, data_size,
				    (struct vb2_signature *)sigbuf,
				    (const struct vb2_public_key *)&key,
				    &wb)) {
			printf("Signature verification failed\n");
			return 1;
		}
	}

	printf("Signature verification succeeded.\n");
	return 0;
}

int ft_sign_rwsig(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	struct vb2_signature *sig = 0;
	uint32_t r, data_size = len, sig_size = SIGNATURE_RSVD_SIZE;
	int retval = 1;

	Debug("%s(): name %s\n", __func__, name);
	Debug("%s(): len  0x%08x (%d)\n", __func__, len, len);

	/* If we don't have a distinct OUTFILE, look for an existing sig */
	if (sign_option.inout_file_count < 2) {
		const struct vb2_signature *old_sig;

		/* Where would it be? */
		if (sign_option.sig_size)
			sig_size = sign_option.sig_size;

		Debug("Looking for old signature at 0x%x\n", len - sig_size);

		if (len < sig_size) {
			fprintf(stderr, "File is too small\n");
			return 1;
		}

		/* Take a look */
		old_sig = (const struct vb2_signature *)(buf + len - sig_size);
		if (vb2_verify_signature(old_sig, sig_size)) {
			fprintf(stderr, "Can't find a valid signature\n");
			return 1;
		}

		/* Use the same exent again */
		data_size = old_sig->data_size;

		Debug("Found sig: data_size is 0x%x (%d)\n", data_size,
		      data_size);
	}

	/* Unless overridden */
	if (sign_option.data_size)
		data_size = sign_option.data_size;

	/* Sign the blob */
	r = vb2_sign_data(&sig, buf, data_size, sign_option.prikey, 0);
	if (r) {
		fprintf(stderr,
			"Unable to sign data (error 0x%08x, if that helps)\n",
			r);
		goto done;
	}

	if (sign_option.inout_file_count < 2) {
		/* Overwrite the old signature */
		if (sig->c.total_size > sig_size) {
			fprintf(stderr, "New sig is too large (%d > %d)\n",
				sig->c.total_size, sig_size);
			goto done;
		}
		memset(buf + len - sig_size, 0xff, sig_size);
		memcpy(buf + len - sig_size, sig, sig->c.total_size);
	} else {
		/* Write the signature to a new file */
		r = vb2_write_object(sign_option.outfile, sig);
		if (r) {
			fprintf(stderr, "Unable to write sig"
				" (error 0x%08x, if that helps)\n", r);
			goto done;
		}
	}

	/* Finally */
	retval = 0;
done:
	if (sig)
		free(sig);
	if (sign_option.prikey)
		vb2_private_key_free(sign_option.prikey);

	return retval;
}

enum futil_file_type ft_recognize_rwsig(uint8_t *buf, uint32_t len)
{
	if (!vb2_verify_signature((const struct vb2_signature *)buf, len))
		return FILE_TYPE_RWSIG;

	if (len >= SIGNATURE_RSVD_SIZE &&
	    !vb2_verify_signature((const struct vb2_signature *)
				  (buf + len - SIGNATURE_RSVD_SIZE),
				  SIGNATURE_RSVD_SIZE))
		return FILE_TYPE_RWSIG;

	return FILE_TYPE_UNKNOWN;
}
