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

	/* Unknown / unspecified error */
	VB2_ERROR_UNKNOWN = 0x10000,

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

	/* Bad secure data */
	VB2_ERROR_BAD_SECDATA,

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
};

#endif  /* VBOOT_2_RETURN_CODES_H_ */
