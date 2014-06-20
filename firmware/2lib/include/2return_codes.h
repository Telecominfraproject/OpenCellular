/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_2_RETURN_CODES_H_
#define VBOOT_2_RETURN_CODES_H_

/*
 * Return codes from verified boot functions.
 *
 * Note that other values may be passed through from vb2ex_*() calls; see
 * the comment for VB2_ERROR_EX below.
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

	/* Mock error for testing */
	VB2_ERROR_MOCK,

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
	 * Common code errors
	 */
	VB2_ERROR_COMMON = VB2_ERROR_BASE + 0x050000,

	/* Buffer is smaller than alignment offset in vb2_align() */
	VB2_ERROR_ALIGN_BIGGER_THAN_SIZE,

	/* Buffer is smaller than request in vb2_align() */
	VB2_ERROR_ALIGN_SIZE,

	/* Parent wraps around in vb2_verify_member_inside() */
	VB2_ERROR_INSIDE_PARENT_WRAPS,

	/* Member wraps around in vb2_verify_member_inside() */
	VB2_ERROR_INSIDE_MEMBER_WRAPS,

	/* Member outside parent in vb2_verify_member_inside() */
	VB2_ERROR_INSIDE_MEMBER_OUTSIDE,

	/* Member data wraps around in vb2_verify_member_inside() */
	VB2_ERROR_INSIDE_DATA_WRAPS,

	/* Member data outside parent in vb2_verify_member_inside() */
	VB2_ERROR_INSIDE_DATA_OUTSIDE,

	/* Bad algorithm in vb2_unpack_key() */
	VB2_ERROR_UNPACK_KEY_ALGORITHM,

	/* Bad key size in vb2_unpack_key() */
	VB2_ERROR_UNPACK_KEY_SIZE,

	/* Bad key alignment in vb2_unpack_key() */
	VB2_ERROR_UNPACK_KEY_ALIGN,

	/* Bad key array size in vb2_unpack_key() */
	VB2_ERROR_UNPACK_KEY_ARRAY_SIZE,

	/* Bad algorithm in vb2_verify_data() */
	VB2_ERROR_VDATA_ALGORITHM,

	/* Incorrect signature size for algorithm in vb2_verify_data() */
	VB2_ERROR_VDATA_SIG_SIZE,

	/* Data smaller than length of signed data in vb2_verify_data() */
	VB2_ERROR_VDATA_NOT_ENOUGH_DATA,

	/* Not enough work buffer for digest in vb2_verify_data() */
	VB2_ERROR_VDATA_WORKBUF_DIGEST,

	/* Not enough work buffer for hash temp data in vb2_verify_data() */
	VB2_ERROR_VDATA_WORKBUF_HASHING,

        /**********************************************************************
	 * Keyblock verification errors (all in vb2_verify_keyblock())
	 */
	VB2_ERROR_KEYBLOCK = VB2_ERROR_BASE + 0x060000,

	/* Data buffer too small for header */
	VB2_ERROR_KEYBLOCK_TOO_SMALL_FOR_HEADER,

	/* Magic number not present */
	VB2_ERROR_KEYBLOCK_MAGIC,

	/* Header version incompatible */
	VB2_ERROR_KEYBLOCK_HEADER_VERSION,

	/* Data buffer too small for keyblock */
	VB2_ERROR_KEYBLOCK_SIZE,

	/* Signature data offset outside keyblock */
	VB2_ERROR_KEYBLOCK_SIG_OUTSIDE,

	/* Signature signed more data than size of keyblock */
	VB2_ERROR_KEYBLOCK_SIGNED_TOO_MUCH,

	/* Signature signed less data than size of keyblock header */
	VB2_ERROR_KEYBLOCK_SIGNED_TOO_LITTLE,

	/* Signature invalid */
	VB2_ERROR_KEYBLOCK_SIG_INVALID,

	/* Data key outside keyblock */
	VB2_ERROR_KEYBLOCK_DATA_KEY_OUTSIDE,

	/* Data key outside signed part of keyblock */
	VB2_ERROR_KEYBLOCK_DATA_KEY_UNSIGNED,

        /**********************************************************************
	 * Preamble verification errors (all in vb2_verify_preamble())
	 */
	VB2_ERROR_PREAMBLE = VB2_ERROR_BASE + 0x070000,

	/* Preamble data too small to contain header */
	VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER,

	/* Header version incompatible */
	VB2_ERROR_PREAMBLE_HEADER_VERSION,

	/* Header version too old */
	VB2_ERROR_PREAMBLE_HEADER_OLD,

	/* Data buffer too small for preamble */
	VB2_ERROR_PREAMBLE_SIZE,

	/* Signature data offset outside preamble */
	VB2_ERROR_PREAMBLE_SIG_OUTSIDE,

	/* Signature signed more data than size of preamble */
	VB2_ERROR_PREAMBLE_SIGNED_TOO_MUCH,

	/* Signature signed less data than size of preamble header */
	VB2_ERROR_PREAMBLE_SIGNED_TOO_LITTLE,

	/* Signature invalid */
	VB2_ERROR_PREAMBLE_SIG_INVALID,

	/* Body signature outside preamble */
	VB2_ERROR_PREAMBLE_BODY_SIG_OUTSIDE,

	/* Kernel subkey outside preamble */
	VB2_ERROR_PREAMBLE_KERNEL_SUBKEY_OUTSIDE,

        /**********************************************************************
	 * Misc higher-level code errors
	 */
	VB2_ERROR_MISC = VB2_ERROR_BASE + 0x080000,

	/* Work buffer too small in vb2_init_context() */
	VB2_ERROR_INITCTX_WORKBUF_SMALL,

	/* Work buffer unaligned in vb2_init_context() */
	VB2_ERROR_INITCTX_WORKBUF_ALIGN,

	/* Work buffer too small in vb2_fw_parse_gbb() */
	VB2_ERROR_GBB_WORKBUF,

	/* Bad magic number in vb2_read_gbb_header() */
	VB2_ERROR_GBB_MAGIC,

	/* Incompatible version in vb2_read_gbb_header() */
	VB2_ERROR_GBB_VERSION,

	/* Old version in vb2_read_gbb_header() */
	VB2_ERROR_GBB_TOO_OLD,

	/* Header size too small in vb2_read_gbb_header() */
	VB2_ERROR_GBB_HEADER_SIZE,

	/* Work buffer too small for root key in vb2_verify_fw_keyblock() */
	VB2_ERROR_FW_KEYBLOCK_WORKBUF_ROOT_KEY,

	/* Work buffer too small for header in vb2_verify_fw_keyblock() */
	VB2_ERROR_FW_KEYBLOCK_WORKBUF_HEADER,

	/* Work buffer too small for keyblock in vb2_verify_fw_keyblock() */
	VB2_ERROR_FW_KEYBLOCK_WORKBUF,

	/* Keyblock version out of range in vb2_verify_fw_keyblock() */
	VB2_ERROR_FW_KEYBLOCK_VERSION_RANGE,

	/* Keyblock version rollback in vb2_verify_fw_keyblock() */
	VB2_ERROR_FW_KEYBLOCK_VERSION_ROLLBACK,

	/* Missing firmware data key in vb2_verify_fw_preamble2() */
	VB2_ERROR_FW_PREAMBLE2_DATA_KEY,

	/* Work buffer too small for header in vb2_verify_fw_preamble2() */
	VB2_ERROR_FW_PREAMBLE2_WORKBUF_HEADER,

	/* Work buffer too small for preamble in vb2_verify_fw_preamble2() */
	VB2_ERROR_FW_PREAMBLE2_WORKBUF,

	/* Firmware version out of range in vb2_verify_fw_preamble2() */
	VB2_ERROR_FW_PREAMBLE2_VERSION_RANGE,

	/* Firmware version rollback in vb2_verify_fw_preamble2() */
	VB2_ERROR_FW_PREAMBLE2_VERSION_ROLLBACK,

        /**********************************************************************
	 * API-level errors
	 */
	VB2_ERROR_API = VB2_ERROR_BASE + 0x090000,

	/* Bag tag in vb2api_init_hash() */
	VB2_ERROR_API_INIT_HASH_TAG,

	/* Preamble not present in vb2api_init_hash() */
	VB2_ERROR_API_INIT_HASH_PREAMBLE,

	/* Work buffer too small in vb2api_init_hash() */
	VB2_ERROR_API_INIT_HASH_WORKBUF,

	/* Missing firmware data key in vb2api_init_hash() */
	VB2_ERROR_API_INIT_HASH_DATA_KEY,

	/* Uninitialized work area in vb2api_extend_hash() */
	VB2_ERROR_API_EXTEND_HASH_WORKBUF,

	/* Too much data hashed in vb2api_extend_hash() */
	VB2_ERROR_API_EXTEND_HASH_SIZE,

	/* Preamble not present in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_PREAMBLE,

	/* Uninitialized work area in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_WORKBUF,

	/* Wrong amount of data hashed in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_SIZE,

	/* Work buffer too small in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_WORKBUF_DIGEST,

	/* Bag tag in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_TAG,

	/* Missing firmware data key in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_DATA_KEY,

	/* Siganature size mismatch in vb2api_check_hash() */
	VB2_ERROR_API_CHECK_HASH_SIG_SIZE,

	/* Phase one needs recovery mode */
	VB2_ERROR_API_PHASE1_RECOVERY,

        /**********************************************************************
	 * Errors which may be generated by implementations of vb2ex functions.
	 * Implementation may also return its own specific errors, which should
	 * NOT be in the range VB2_ERROR_BASE...VB2_ERROR_MAX to avoid
	 * conflicting with future vboot2 error codes.
	 */
	VB2_ERROR_EX = VB2_ERROR_BASE + 0x0a0000,

	/* Read resource not implemented */
	VB2_ERROR_EX_READ_RESOURCE_UNIMPLEMENTED,

	/* Resource index not found */
	VB2_ERROR_EX_READ_RESOURCE_INDEX,

	/* Size of resource not big enough for requested offset and/or size */
	VB2_ERROR_EX_READ_RESOURCE_SIZE,

	/* TPM clear owner failed */
	VB2_ERROR_EX_TPM_CLEAR_OWNER,

	/* TPM clear owner not implemented */
	VB2_ERROR_EX_TPM_CLEAR_OWNER_UNIMPLEMENTED,

        /**********************************************************************
	 * Highest non-zero error generated inside vboot library.  Note that
	 * error codes passed through vboot when it calls external APIs may
	 * still be outside this range.
	 */
	VB2_ERROR_MAX = VB2_ERROR_BASE + 0xffffff,
};

#endif  /* VBOOT_2_RETURN_CODES_H_ */
