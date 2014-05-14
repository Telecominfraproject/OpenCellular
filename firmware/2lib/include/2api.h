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

#endif  /* VBOOT_2_API_H_ */
