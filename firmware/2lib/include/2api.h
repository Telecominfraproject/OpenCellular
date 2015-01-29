/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* APIs between calling firmware and vboot_reference
 *
 * General notes:
 *
 * TODO: split this file into a vboot_entry_points.h file which contains the
 * entry points for the firmware to call vboot_reference, and a
 * vboot_firmware_exports.h which contains the APIs to be implemented by the
 * calling firmware and exported to vboot_reference.
 *
 * Notes:
 *    * Assumes this code is never called in the S3 resume path.  TPM resume
 *      must be done elsewhere, and VB2_NV_DEBUG_RESET_MODE is ignored.
 */

#ifndef VBOOT_2_API_H_
#define VBOOT_2_API_H_
#include <stdint.h>

#include "2common.h"
#include "2crypto.h"
#include "2fw_hash_tags.h"
#include "2guid.h"
#include "2recovery_reasons.h"
#include "2return_codes.h"

/* Size of non-volatile data used by vboot */
#define VB2_NVDATA_SIZE 16

/* Size of secure data used by vboot */
#define VB2_SECDATA_SIZE 10

/*
 * Recommended size of work buffer.
 *
 * TODO: The recommended size really depends on which key algorithms are
 * used.  Should have a better / more accurate recommendation than this.
 */
#define VB2_WORKBUF_RECOMMENDED_SIZE (12 * 1024)

/* Recommended buffer size for vb2api_get_pcr_digest */
#define VB2_PCR_DIGEST_RECOMMENDED_SIZE 32

/* Flags for vb2_context.
 *
 * Unless otherwise noted, flags are set by verified boot and may be read (but
 * not set or cleared) by the caller.
 */
enum vb2_context_flags {

	/*
	 * Verified boot has changed nvdata[].  Caller must save nvdata[] back
	 * to its underlying storage, then may clear this flag.
	 */
	VB2_CONTEXT_NVDATA_CHANGED = (1 << 0),

	/*
	 * Verified boot has changed secdata[].  Caller must save secdata[]
	 * back to its underlying storage, then may clear this flag.
	 */
	VB2_CONTEXT_SECDATA_CHANGED = (1 << 1),

	/* Recovery mode is requested this boot */
	VB2_CONTEXT_RECOVERY_MODE = (1 << 2),

	/* Developer mode is requested this boot */
	VB2_CONTEXT_DEVELOPER_MODE = (1 << 3),

	/*
	 * Force recovery mode due to physical user request.  Caller may set
	 * this flag when initializing the context.
	 */
	VB2_CONTEXT_FORCE_RECOVERY_MODE = (1 << 4),

	/*
	 * Force developer mode enabled.  Caller may set this flag when
	 * initializing the context.
	 */
	VB2_CONTEXT_FORCE_DEVELOPER_MODE = (1 << 5),

	/* Using firmware slot B.  If this flag is clear, using slot A. */
	VB2_CONTEXT_FW_SLOT_B = (1 << 6),

	/* RAM should be cleared by caller this boot */
	VB2_CONTEXT_CLEAR_RAM = (1 << 7),
};

/*
 * Context for firmware verification.  Pass this to all vboot APIs.
 *
 * Caller may relocate this between calls to vboot APIs.
 */
struct vb2_context {
	/**********************************************************************
	 * Fields which must be initialized by caller.
	 */

	/*
	 * Flags; see vb2_context_flags.  Some flags may only be set by caller
	 * prior to calling vboot functions.
	 */
	uint32_t flags;

	/*
	 * Work buffer, and length in bytes.  Caller may relocate this between
	 * calls to vboot APIs; it contains no internal pointers.  Caller must
	 * not examine the contents of this work buffer directly.
	 */
	uint8_t *workbuf;
	uint32_t workbuf_size;

	/*
	 * Non-volatile data.  Caller must fill this from some non-volatile
	 * location.  If the VB2_CONTEXT_NVDATA_CHANGED flag is set when a
	 * vb2api function returns, caller must save the data back to the
	 * non-volatile location and then clear the flag.
	 */
	uint8_t nvdata[VB2_NVDATA_SIZE];

	/*
	 * Secure data.  Caller must fill this from some secure non-volatile
	 * location.  If the VB2_CONTEXT_SECDATA_CHANGED flag is set when a
	 * function returns, caller must save the data back to the secure
	 * non-volatile location and then clear the flag.
	 */
	uint8_t secdata[VB2_SECDATA_SIZE];

	/*
	 * Context pointer for use by caller.  Verified boot never looks at
	 * this.  Put context here if you need it for APIs that verified boot
	 * may call (vb2ex_...() functions).
	 */
	void *non_vboot_context;

	/**********************************************************************
	 * Fields caller may examine after calling vb2api_fw_phase1().  Caller
         * must set these fields to 0 before calling any vboot functions.
	 */

	/*
	 * Amount of work buffer used so far.  Verified boot sub-calls use
	 * this to know where the unused work area starts.  Caller may use
	 * this between calls to vboot APIs to know how much data must be
	 * copied when relocating the work buffer.
	 */
	uint32_t workbuf_used;
};

enum vb2_resource_index {

	/* Google binary block */
	VB2_RES_GBB,

	/*
	 * Verified boot block (keyblock+preamble).  Use VB2_CONTEXT_FW_SLOT_B
	 * to determine whether this refers to slot A or slot B; vboot will
	 * set that flag to the proper state before reading the vblock.
	 */
	VB2_RES_FW_VBLOCK,
};

/* Digest ID for vbapi_get_pcr_digest() */
enum vb2_pcr_digest {
	/* Digest based on current developer and recovery mode flags */
	BOOT_MODE_PCR,

	/* SHA-256 hash digest of HWID, from GBB */
	HWID_DIGEST_PCR,
};

/******************************************************************************
 * APIs provided by verified boot.
 *
 * At a high level, call functions in the order described below.  After each
 * call, examine vb2_context.flags to determine whether nvdata or secdata
 * needs to be written.
 *
 * If you need to cause the boot process to fail at any point, call
 * vb2api_fail().  Then check vb2_context.flags to see what data needs to be
 * written.  Then reboot.
 *
 *	Load nvdata from wherever you keep it.
 *
 *	Load secdata from wherever you keep it.
 *
 *      	If it wasn't there at all (for example, this is the first boot
 *		of a new system in the factory), call vb2api_secdata_create()
 *		to initialize the data.
 *
 *		If access to your storage is unreliable (reads/writes may
 *		contain corrupt data), you may call vb2api_secdata_check() to
 *		determine if the data was valid, and retry reading if it
 *		wasn't.  (In that case, you should also read back and check the
 *		data after any time you write it, to make sure it was written
 *		correctly.)
 *
 *	Call vb2api_fw_phase1().  At present, this nominally decides whether
 *	recovery mode is needed this boot.
 *
 *	Call vb2api_fw_phase2().  At present, this nominally decides which
 *	firmware slot will be attempted (A or B).
 *
 *	Call vb2api_fw_phase3().  At present, this nominally verifies the
 *	firmware keyblock and preamble.
 *
 *	Lock down wherever you keep secdata.  It should no longer be writable
 *	this boot.
 *
 *	Verify the hash of each section of code/data you need to boot the RW
 *	firmware.  For each section:
 *
 *		Call vb2_init_hash() to see if the hash exists.
 *
 *		Load the data for the section.  Call vb2_extend_hash() on the
 *		data as you load it.  You can load it all at once and make one
 *		call, or load and hash-extend a block at a time.
 *
 *		Call vb2_check_hash() to see if the hash is valid.
 *
 *			If it is valid, you may use the data and/or execute
 *			code from that section.
 *
 *			If the hash was invalid, you must reboot.
 *
 * At this point, firmware verification is done, and vb2_context contains the
 * kernel key needed to verify the kernel.  That context should be preserved
 * and passed on to kernel selection.  For now, that requires translating it
 * into the old VbSharedData format (via a func which does not yet exist...)
 */

/**
 * Sanity-check the contents of the secure storage context.
 *
 * Use this if reading from secure storage may be flaky, and you want to retry
 * reading it several times.
 *
 * This may be called before vb2api_phase1().
 *
 * @param ctx		Context pointer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2api_secdata_check(const struct vb2_context *ctx);

/**
 * Create fresh data in the secure storage context.
 *
 * Use this only when initializing the secure storage context on a new machine
 * the first time it boots.  Do NOT simply use this if vb2api_secdata_check()
 * (or any other API in this library) fails; that could allow the secure data
 * to be rolled back to an insecure state.
 *
 * This may be called before vb2api_phase1().
 *
 * @param ctx		Context pointer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2api_secdata_create(struct vb2_context *ctx);

/**
 * Report firmware failure to vboot.
 *
 * This may be called before vb2api_phase1() to indicate errors in the boot
 * process prior to the start of vboot.
 *
 * If this is called after vb2api_phase1(), on return, the calling firmware
 * should check for updates to secdata and/or nvdata, then reboot.
 *
 * @param reason	Recovery reason
 * @param subcode	Recovery subcode
 */
void vb2api_fail(struct vb2_context *ctx, uint8_t reason, uint8_t subcode);

/**
 * Firmware selection, phase 1.
 *
 * On error, the calling firmware should jump directly to recovery-mode
 * firmware without rebooting.
 *
 * @param ctx		Vboot context
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2api_fw_phase1(struct vb2_context *ctx);

/**
 * Firmware selection, phase 2.
 *
 * On error, the calling firmware should check for updates to secdata and/or
 * nvdata, then reboot.
 *
 * @param ctx		Vboot context
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2api_fw_phase2(struct vb2_context *ctx);

/**
 * Firmware selection, phase 3.
 *
 * On error, the calling firmware should check for updates to secdata and/or
 * nvdata, then reboot.
 *
 * On success, the calling firmware should lock down secdata before continuing
 * with the boot process.
 *
 * @param ctx		Vboot context
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2api_fw_phase3(struct vb2_context *ctx);

/**
 * Initialize hashing data for the specified tag.
 *
 * @param ctx		Vboot context
 * @param tag		Tag to start hashing (enum vb2_hash_tag)
 * @param size		If non-null, expected size of data for tag will be
 *			stored here on output.
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2api_init_hash(struct vb2_context *ctx, uint32_t tag, uint32_t *size);

/**
 * Same, but for new-style structs.
 */
int vb2api_init_hash2(struct vb2_context *ctx,
		      const struct vb2_guid *guid,
		      uint32_t *size);

/**
 * Extend the hash started by vb2api_init_hash() with additional data.
 *
 * (This is the same for both old and new style structs.)
 *
 * @param ctx		Vboot context
 * @param buf		Data to hash
 * @param size		Size of data in bytes
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2api_extend_hash(struct vb2_context *ctx,
		       const void *buf,
		       uint32_t size);

/**
 * Check the hash value started by vb2api_init_hash().
 *
 * @param ctx		Vboot context
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2api_check_hash(struct vb2_context *ctx);

/**
 * Get a PCR digest
 *
 * @param ctx		Vboot context
 * @param which_digest	PCR index of the digest
 * @param dest		Destination where the digest is copied.
 * 			Recommended size is VB2_PCR_DIGEST_RECOMMENDED_SIZE.
 * @param dest_size	IN: size of the buffer pointed by dest
 * 			OUT: size of the copied digest
 * @return VB2_SUCCESS, or error code on error
 */
int vb2api_get_pcr_digest(struct vb2_context *ctx,
			  enum vb2_pcr_digest which_digest,
			  uint8_t *dest,
			  uint32_t *dest_size);

/*****************************************************************************/
/* APIs provided by the caller to verified boot */

/**
 * Clear the TPM owner.
 *
 * @param ctx		Vboot context
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2ex_tpm_clear_owner(struct vb2_context *ctx);

/**
 * Read a verified boot resource.
 *
 * @param ctx		Vboot context
 * @param index		Resource index to read
 * @param offset	Byte offset within resource to start at
 * @param buf		Destination for data
 * @param size		Amount of data to read
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2ex_read_resource(struct vb2_context *ctx,
			enum vb2_resource_index index,
			uint32_t offset,
			void *buf,
			uint32_t size);

void vb2ex_printf(const char *func, const char *fmt, ...);

/**
 * Initialize the hardware crypto engine to calculate a block-style digest.
 *
 * @param hash_alg	Hash algorithm to use
 * @param data_size	Expected total size of data to hash
 * @return VB2_SUCCESS, or non-zero error code (HWCRYPTO_UNSUPPORTED not fatal).
 */
int vb2ex_hwcrypto_digest_init(enum vb2_hash_algorithm hash_alg,
			       uint32_t data_size);

/**
 * Extend the hash in the hardware crypto engine with another block of data.
 *
 * @param buf		Next data block to hash
 * @param size		Length of data block in bytes
 * @return VB2_SUCCESS, or non-zero error code.
 */
int vb2ex_hwcrypto_digest_extend(const uint8_t *buf, uint32_t size);

/**
 * Finalize the digest in the hardware crypto engine and extract the result.
 *
 * @param digest	Destination buffer for resulting digest
 * @param digest_size	Length of digest buffer in bytes
 * @return VB2_SUCCESS, or non-zero error code.
 */
int vb2ex_hwcrypto_digest_finalize(uint8_t *digest, uint32_t digest_size);

#endif  /* VBOOT_2_API_H_ */
