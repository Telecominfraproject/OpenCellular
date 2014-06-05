/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_2_RETURN_CODES_H_
#define VBOOT_2_RETURN_CODES_H_

/*
 * Return codes from verified boot functions.
 *
 * TODO: Go through code and replace VB2_ERROR_UNKNOWN with more specific
 * error codes, and make the existing codes more consistent and useful.
 */
enum vb2_return_code {
	/* Success - no error */
	VB2_SUCCESS = 0,

	/*
	 * All vboot2 error codes start at a large offset from zero, to reduce
	 * the risk of overlap with other error codes (TPM, etc.).
	 */
	VB2_ERROR_BASE = 0x0100000,

	/* Unknown / unspecified error */
	VB2_ERROR_UNKNOWN = VB2_ERROR_BASE + 1,

        /**********************************************************************
	 * SHA errors
	 */
	VB2_ERROR_SHA = VB2_ERROR_BASE + 0x010000,

	/* Bad algorithm in vb2_digest_init() */
	VB2_ERROR_SHA_INIT_ALGORITHM,

	/* Bad algorithm in vb2_digest_extend() */
	VB2_ERROR_SHA_EXTEND_ALGORITHM,

	/* Bad algorithm in vb2_digest_finalize() */
	VB2_ERROR_SHA_FINALIZE_ALGORITHM,

	/* Digest size buffer too small in vb2_digest_finalize() */
	VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE,

        /**********************************************************************
	 * RSA errors
	 */
	VB2_ERROR_RSA = VB2_ERROR_BASE + 0x020000,

	/* Padding mismatch in vb2_check_padding() */
	VB2_ERROR_RSA_PADDING,

	/* Bad algorithm in vb2_check_padding() */
	VB2_ERROR_RSA_PADDING_ALGORITHM,

	/* Null param passed to vb2_verify_digest() */
	VB2_ERROR_RSA_VERIFY_PARAM,

	/* Bad algorithm in vb2_verify_digest() */
	VB2_ERROR_RSA_VERIFY_ALGORITHM,

	/* Bad signature length in vb2_verify_digest() */
	VB2_ERROR_RSA_VERIFY_SIG_LEN,

	/* Work buffer too small in vb2_verify_digest() */
	VB2_ERROR_RSA_VERIFY_WORKBUF,

	/* Digest mismatch in vb2_verify_digest() */
	VB2_ERROR_RSA_VERIFY_DIGEST,

        /**********************************************************************
	 * NV storage errors
	 */
	VB2_ERROR_NV = VB2_ERROR_BASE + 0x030000,

	/* Bad header in vb2_nv_check_crc() */
	VB2_ERROR_NV_HEADER,

	/* Bad CRC in vb2_nv_check_crc() */
	VB2_ERROR_NV_CRC,

        /**********************************************************************
	 * Secure data storage errors
	 */
	VB2_ERROR_SECDATA = VB2_ERROR_BASE + 0x040000,

	/* Bad CRC in vb2_secdata_check_crc() */
	VB2_ERROR_SECDATA_CRC,

	/* Bad struct version in vb2_secdata_init() */
	VB2_ERROR_SECDATA_VERSION,

	/* Invalid param in vb2_secdata_get() */
	VB2_ERROR_SECDATA_GET_PARAM,

	/* Invalid param in vb2_secdata_set() */
	VB2_ERROR_SECDATA_SET_PARAM,

	/* Invalid flags passed to vb2_secdata_set() */
	VB2_ERROR_SECDATA_SET_FLAGS,

        /**********************************************************************
	 * TODO: errors which must still be made specific
	 */
	VB2_ERROR_TODO = VB2_ERROR_BASE + 0xff0000,

	/* Work buffer too small */
	VB2_ERROR_WORKBUF_TOO_SMALL,

	/* Buffer too small (other than the work buffer) */
	VB2_ERROR_BUFFER_TOO_SMALL,

	/* Buffer unaligned */
	VB2_ERROR_BUFFER_UNALIGNED,

	/* Bad GBB header */
	VB2_ERROR_BAD_GBB_HEADER,

	/* Bad algorithm - unknown, or unsupported */
	VB2_ERROR_BAD_ALGORITHM,

	/* Signature check failed */
	VB2_ERROR_BAD_SIGNATURE,

	/* Bad key */
	VB2_ERROR_BAD_KEY,

	/* Bad keyblock */
	VB2_ERROR_BAD_KEYBLOCK,

	/* Bad preamble */
	VB2_ERROR_BAD_PREAMBLE,

	/* Bad firmware keyblock version (out of range, or rollback) */
	VB2_ERROR_FW_KEYBLOCK_VERSION,

	/* Bad firmware version (out of range, or rollback) */
	VB2_ERROR_FW_VERSION,

	/* Bad hash tag */
	VB2_ERROR_BAD_TAG,

        /**********************************************************************
	 * Highest non-zero error generated inside vboot library.  Note that
	 * error codes passed through vboot when it calls external APIs may
	 * still be outside this range.
	 */
	VB2_ERROR_MAX = VB2_ERROR_BASE + 0xffffff,

};

#endif  /* VBOOT_2_RETURN_CODES_H_ */
