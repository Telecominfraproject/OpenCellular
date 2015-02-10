/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Non-volatile storage routines for verified boot. */

#ifndef VBOOT_REFERENCE_NVSTORAGE_H_
#define VBOOT_REFERENCE_NVSTORAGE_H_
#include <stdint.h>

#define VBNV_BLOCK_SIZE 16  /* Size of NV storage block in bytes */

typedef struct VbNvContext {
	/* Raw NV data.  Caller must fill this before calling VbNvSetup(). */
	uint8_t raw[VBNV_BLOCK_SIZE];
	/*
	 * Flag indicating whether raw data has changed.  Set by VbNvTeardown()
	 * if the raw data has changed and needs to be stored to the underlying
	 * non-volatile data store.
	 */
	int raw_changed;

	/*
	 * Internal data for NV storage routines.  Caller should not touch
	 * these fields.
	 */
	int regenerate_crc;
} VbNvContext;

/* Parameter type for VbNvGet(), VbNvSet(). */
typedef enum VbNvParam {
	/*
	 * Parameter values have been reset to defaults (flag for firmware).
	 * 0=clear; 1=set.
	 */
	VBNV_FIRMWARE_SETTINGS_RESET = 0,
	/*
	 * Parameter values have been reset to defaults (flag for kernel).
	 * 0=clear; 1=set.
	 */
	VBNV_KERNEL_SETTINGS_RESET,
	/* Request debug reset on next S3->S0 transition.  0=clear; 1=set. */
	VBNV_DEBUG_RESET_MODE,
	/*
	 * Number of times to try booting RW firmware slot B before slot A.
	 * Valid range: 0-15.
	 *
	 * Vboot2: Number of times to try the firmware in VBNV_FW_TRY_NEXT.
	 *
	 * These refer to the same field, but have different enum values so
	 * case statement don't complain about duplicates.
	 */
	VBNV_TRY_B_COUNT,
	VBNV_FW_TRY_COUNT,
	/*
	 * Request recovery mode on next boot; see VBNB_RECOVERY_* below for
	 * currently defined reason codes.  8-bit value.
	 */
	VBNV_RECOVERY_REQUEST,
	/*
	 * Localization index for screen bitmaps displayed by firmware.
	 * 8-bit value.
	 */
	VBNV_LOCALIZATION_INDEX,
	/* Field reserved for kernel/user-mode use; 32-bit value. */
	VBNV_KERNEL_FIELD,
	/* Allow booting from USB in developer mode.  0=no, 1=yes. */
	VBNV_DEV_BOOT_USB,
	/* Allow booting of legacy OSes in developer mode.  0=no, 1=yes. */
	VBNV_DEV_BOOT_LEGACY,
	/* Only boot Google-signed images in developer mode.  0=no, 1=yes. */
	VBNV_DEV_BOOT_SIGNED_ONLY,
	/*
	 * Set by userspace to request that RO firmware disable dev-mode on the
	 * next boot. This is likely only possible if the dev-switch is
	 * virtual.
	 */
	VBNV_DISABLE_DEV_REQUEST,
	/*
	 * Set and cleared by vboot to request that the video Option ROM be
	 * loaded at boot time, so that BIOS screens can be displayed. 0=no,
	 * 1=yes.
	 */
	VBNV_OPROM_NEEDED,
	/* Request that the firmware clear the TPM owner on the next boot. */
	VBNV_CLEAR_TPM_OWNER_REQUEST,
	/* Flag that TPM owner was cleared on request. */
	VBNV_CLEAR_TPM_OWNER_DONE,
	/* More details on recovery reason */
	VBNV_RECOVERY_SUBCODE,
	/* Request that NVRAM be backed up at next boot if possible. */
	VBNV_BACKUP_NVRAM_REQUEST,

	/* Vboot2: Firmware slot to try next.  0=A, 1=B */
	VBNV_FW_TRY_NEXT,
	/* Vboot2: Firmware slot tried this boot (0=A, 1=B) */
	VBNV_FW_TRIED,
	/* Vboot2: Result of trying that firmware (see vb2_fw_result) */
	VBNV_FW_RESULT,
	/* Firmware slot tried previous boot (0=A, 1=B) */
	VBNV_FW_PREV_TRIED,
	/* Result of trying that firmware (see vb2_fw_result) */
	VBNV_FW_PREV_RESULT,

} VbNvParam;

/* Result of trying the firmware in VBNV_FW_TRIED */
typedef enum VbFwResult {
	/* Unknown */
	VBNV_FW_RESULT_UNKNOWN = 0,

	/* Trying a new slot, but haven't reached success/failure */
	VBNV_FW_RESULT_TRYING = 1,

	/* Successfully booted to the OS */
	VBNV_FW_RESULT_SUCCESS = 2,

	/* Known failure */
	VBNV_FW_RESULT_FAILURE = 3,

} VbFwResult;

/* Recovery reason codes for VBNV_RECOVERY_REQUEST */
/* Recovery not requested. */
#define VBNV_RECOVERY_NOT_REQUESTED   0x00
/*
 * Recovery requested from legacy utility.  (Prior to the NV storage spec,
 * recovery mode was a single bitfield; this value is reserved so that scripts
 * which wrote 1 to the recovery field are distinguishable from scripts whch
 * use the recovery reasons listed here.
 */
#define VBNV_RECOVERY_LEGACY          0x01
/* User manually requested recovery via recovery button */
#define VBNV_RECOVERY_RO_MANUAL       0x02
/* RW firmware failed signature check (neither RW firmware slot was valid) */
#define VBNV_RECOVERY_RO_INVALID_RW   0x03
/* S3 resume failed */
#define VBNV_RECOVERY_RO_S3_RESUME    0x04
/* TPM error in read-only firmware (deprecated) */
#define VBNV_RECOVERY_DEP_RO_TPM_ERROR    0x05
/* Shared data error in read-only firmware */
#define VBNV_RECOVERY_RO_SHARED_DATA  0x06
/* Test error from S3Resume() */
#define VBNV_RECOVERY_RO_TEST_S3      0x07
/* Test error from LoadFirmwareSetup() */
#define VBNV_RECOVERY_RO_TEST_LFS     0x08
/* Test error from LoadFirmware() */
#define VBNV_RECOVERY_RO_TEST_LF      0x09
/*
 * RW firmware failed signature check (neither RW firmware slot was valid).
 * Recovery reason is VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN + the check value
 * for the slot which came closest to validating; see VBSD_LF_CHECK_* in
 * vboot_struct.h.
 */
#define VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN  0x10
#define VBNV_RECOVERY_RO_INVALID_RW_CHECK_MAX  0x1F
/*
 * Firmware boot failure outside of verified boot (RAM init, missing SSD,
 * etc.).
 */
#define VBNV_RECOVERY_RO_FIRMWARE     0x20
/*
 * Recovery mode TPM initialization requires a system reboot.  The system was
 * already in recovery mode for some other reason when this happened.
 */
#define VBNV_RECOVERY_RO_TPM_REBOOT   0x21
/* EC software sync - other error */
#define VBNV_RECOVERY_EC_SOFTWARE_SYNC 0x22
/* EC software sync - unable to determine active EC image */
#define VBNV_RECOVERY_EC_UNKNOWN_IMAGE 0x23
/* EC software sync - error obtaining EC image hash (deprecated) */
#define VBNV_RECOVERY_DEP_EC_HASH         0x24
/* EC software sync - error obtaining expected EC image */
#define VBNV_RECOVERY_EC_EXPECTED_IMAGE 0x25
/* EC software sync - error updating EC */
#define VBNV_RECOVERY_EC_UPDATE       0x26
/* EC software sync - unable to jump to EC-RW */
#define VBNV_RECOVERY_EC_JUMP_RW      0x27
/* EC software sync - unable to protect / unprotect EC-RW */
#define VBNV_RECOVERY_EC_PROTECT      0x28
/* EC software sync - error obtaining expected EC hash */
#define VBNV_RECOVERY_EC_EXPECTED_HASH 0x29
/* EC software sync - expected EC image doesn't match hash */
#define VBNV_RECOVERY_EC_HASH_MISMATCH 0x2A
/* VB2: Secure data inititalization error */
#define VBNV_RECOVERY_VB2_SECDATA_INIT 0x2B
/* VB2: GBB header is bad */
#define VBNV_RECOVERY_VB2_GBB_HEADER  0x2C
/* VB2: Unable to clear TPM owner */
#define VBNV_RECOVERY_VB2_TPM_CLEAR_OWNER 0x2D
/* VB2: Error determining/updating virtual dev switch */
#define VBNV_RECOVERY_VB2_DEV_SWITCH  0x2E
/* VB2: Error determining firmware slot */
#define VBNV_RECOVERY_VB2_FW_SLOT     0x2F
/* Unspecified/unknown error in read-only firmware */
#define VBNV_RECOVERY_RO_UNSPECIFIED  0x3F
/*
 * User manually requested recovery by pressing a key at developer
 * warning screen
 */
#define VBNV_RECOVERY_RW_DEV_SCREEN   0x41
/* No OS kernel detected */
#define VBNV_RECOVERY_RW_NO_OS        0x42
/* OS kernel failed signature check */
#define VBNV_RECOVERY_RW_INVALID_OS   0x43
/* TPM error in rewritable firmware (deprecated) */
#define VBNV_RECOVERY_DEP_RW_TPM_ERROR    0x44
/* RW firmware in dev mode, but dev switch is off */
#define VBNV_RECOVERY_RW_DEV_MISMATCH 0x45
/* Shared data error in rewritable firmware */
#define VBNV_RECOVERY_RW_SHARED_DATA  0x46
/* Test error from LoadKernel() */
#define VBNV_RECOVERY_RW_TEST_LK      0x47
/* No bootable disk found (deprecated)*/
#define VBNV_RECOVERY_DEP_RW_NO_DISK      0x48
/* Rebooting did not correct TPM_E_FAIL or TPM_E_FAILEDSELFTEST  */
#define VBNV_RECOVERY_TPM_E_FAIL      0x49
/* TPM setup error in read-only firmware */
#define VBNV_RECOVERY_RO_TPM_S_ERROR  0x50
/* TPM write error in read-only firmware */
#define VBNV_RECOVERY_RO_TPM_W_ERROR  0x51
/* TPM lock error in read-only firmware */
#define VBNV_RECOVERY_RO_TPM_L_ERROR  0x52
/* TPM update error in read-only firmware */
#define VBNV_RECOVERY_RO_TPM_U_ERROR  0x53
/* TPM read error in rewritable firmware */
#define VBNV_RECOVERY_RW_TPM_R_ERROR  0x54
/* TPM write error in rewritable firmware */
#define VBNV_RECOVERY_RW_TPM_W_ERROR  0x55
/* TPM lock error in rewritable firmware */
#define VBNV_RECOVERY_RW_TPM_L_ERROR  0x56
/* EC software sync unable to get EC image hash */
#define VBNV_RECOVERY_EC_HASH_FAILED  0x57
/* EC software sync invalid image hash size */
#define VBNV_RECOVERY_EC_HASH_SIZE    0x58
/* Unspecified error while trying to load kernel */
#define VBNV_RECOVERY_LK_UNSPECIFIED  0x59
/* No bootable storage device in system */
#define VBNV_RECOVERY_RW_NO_DISK      0x5A
/* No bootable kernel found on disk */
#define VBNV_RECOVERY_RW_NO_KERNEL    0x5B
/* Unspecified/unknown error in rewritable firmware */
#define VBNV_RECOVERY_RW_UNSPECIFIED  0x7F
/* DM-verity error */
#define VBNV_RECOVERY_KE_DM_VERITY    0x81
/* Unspecified/unknown error in kernel */
#define VBNV_RECOVERY_KE_UNSPECIFIED  0xBF
/* Recovery mode test from user-mode */
#define VBNV_RECOVERY_US_TEST         0xC1
/* Unspecified/unknown error in user-mode */
#define VBNV_RECOVERY_US_UNSPECIFIED  0xFF

/**
 * Initialize the NV storage library.
 *
 * This must be called before any other functions in this library.  Returns 0
 * if success, non-zero if error.
 *
 * Proper calling procedure:
 *    1) Allocate a context struct.
 *    2) If multi-threaded/multi-process, acquire a lock to prevent
 *       other processes from modifying the underlying storage.
 *    3) Read underlying storage and fill in context->raw.
 *    4) Call VbNvSetup().
 *
 * If you have access to global variables, you may want to wrap all that in
 * your own VbNvOpen() function.  We don't do that in here because there are no
 * global variables in UEFI BIOS during the PEI phase (that's also why we have
 * to pass around a context pointer).
 */
int VbNvSetup(VbNvContext *context);

/**
 * Clean up and flush changes back to the raw data.
 *
 * This must be called after other functions in this library.  Returns 0 if
 * success, non-zero if error.
 *
 * Proper calling procedure:
 *    1) Call VbNvExit().
 *    2) If context.raw_changed, write data back to underlying storage.
 *    3) Release any lock you acquired before calling VbNvSetup().
 *    4) Free the context struct.
 *
 * If you have access to global variables, you may want to wrap this
 * in your own VbNvClose() function.
 */
int VbNvTeardown(VbNvContext *context);

/**
 * Read a NV storage parameter into *dest.
 *
 * Returns 0 if success, non-zero if error.
 *
 * This may only be called between VbNvSetup() and VbNvTeardown().
 */
int VbNvGet(VbNvContext *context, VbNvParam param, uint32_t *dest);

/**
 * Set a NV storage param to a new value.
 *
 * Returns 0 if success, non-zero if error.
 *
 * This may only be called between VbNvSetup() and VbNvTeardown().
 */
int VbNvSet(VbNvContext *context, VbNvParam param, uint32_t value);

/**
 * Attempt to restore some fields of a lost VbNvContext from a backup area.
 * The rest of the fields are unchanged, so they'd need to be set to their
 * appropriate defaults by calling VbNvSetup() first (which is usually how we
 * know the fields have been lost).
 *
 * Returns 0 if success, non-zero if error.
 *
 * This may only be called between VbNvSetup() and VbNvTeardown().
 */
int RestoreNvFromBackup(VbNvContext *vnc);

/**
 * Attempt to save some fields of the VbNvContext to a backup area.
 *
 * Returns 0 if success, non-zero if error. If it succeeds, it will clear the
 * VBNV_BACKUP_NVRAM_REQUEST flag in the VbNvContext.
 *
 * This may only be called when the backup area is writable.
 */
int SaveNvToBackup(VbNvContext *vnc);

#endif  /* VBOOT_REFERENCE_NVSTORAGE_H_ */
