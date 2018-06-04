/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* APIs provided by firmware to vboot_reference.
 *
 * General notes:
 *
 * All verified boot functions now start with "Vb" for namespace clarity.  This
 * fixes the problem where uboot and vboot both defined assert().
 *
 * Verified boot APIs to be implemented by the calling firmware and exported to
 * vboot_reference start with "VbEx".
 *
 * TODO: split this file into a vboot_entry_points.h file which contains the
 * entry points for the firmware to call vboot_reference, and a
 * vboot_firmware_exports.h which contains the APIs to be implemented by the
 * calling firmware and exported to vboot_reference.
 */

#ifndef VBOOT_REFERENCE_VBOOT_API_H_
#define VBOOT_REFERENCE_VBOOT_API_H_
#include <stdint.h>
#include <stdlib.h>

#include "gpt.h"

/*****************************************************************************/
/* Error codes */

/*
 * Functions which return an error all return this type.  This is a 32-bit
 * value rather than an int so it's consistent across UEFI, which is 32-bit
 * during PEI and 64-bit during DXE/BDS.
 */
typedef uint32_t VbError_t;

/*
 * Define test_mockable for mocking functions.
 */
#ifndef test_mockable
#define test_mockable __attribute__((weak))
#endif

/*
 * Predefined error numbers.  Success is 0.  Errors are non-zero, but differ
 * between functions.  For example, the TPM functions may pass through TPM
 * error codes, some of which may be recoverable.
 */
enum VbErrorPredefined_t {
	/* No error; function completed successfully. */
	VBERROR_SUCCESS                       = 0,

	/*
	 * The verified boot entry points VbInit(), VbSelectFirmware(),
	 * VbSelectAndLoadKernel() may return the following errors.
	 */
	/* Unknown error */
	VBERROR_UNKNOWN                       = 0x10000,
	/* Unable to initialize shared data */
	VBERROR_INIT_SHARED_DATA              = 0x10001,
	/* Error resuming TPM during a S3 resume */
	VBERROR_TPM_S3_RESUME                 = 0x10002,
	/* VbSelectFirmware() failed to find a valid firmware */
	VBERROR_LOAD_FIRMWARE                 = 0x10003,
	/* Unable to write firmware versions to TPM */
	VBERROR_TPM_WRITE_FIRMWARE            = 0x10004,
	/* Unable to lock firmware versions in TPM */
	VBERROR_TPM_LOCK_FIRMWARE             = 0x10005,
	/* Unable to set boot mode state in TPM */
	VBERROR_TPM_SET_BOOT_MODE_STATE       = 0x10006,
	/* Calling firmware needs to perform a reboot. */
	VBERROR_REBOOT_REQUIRED               = 0x10007,
	/* Unable to set up TPM */
	VBERROR_TPM_FIRMWARE_SETUP            = 0x10008,
	/* Unable to read kernel versions from TPM */
	VBERROR_TPM_READ_KERNEL               = 0x10009,
	/* Attempt to load developer-only firmware with developer switch off */
	VBERROR_DEV_FIRMWARE_SWITCH_MISMATCH  = 0x1000A,
	/* Unable to write kernel versions to TPM */
	VBERROR_TPM_WRITE_KERNEL              = 0x1000B,
	/* Unable to lock kernel versions in TPM */
	VBERROR_TPM_LOCK_KERNEL               = 0x1000C,
	/* Calling firmware requested shutdown via VbExIsShutdownRequested() */
	VBERROR_SHUTDOWN_REQUESTED            = 0x1000D,
	/* Unable to find a boot device on which to look for a kernel */
	VBERROR_NO_DISK_FOUND                 = 0x1000E,
	/* No OS kernel found on any boot device */
	VBERROR_NO_KERNEL_FOUND               = 0x1000F,
	/* All OS kernels found were invalid (corrupt, improperly signed...) */
	VBERROR_INVALID_KERNEL_FOUND          = 0x10010,
	/* VbSelectAndLoadKernel() requested recovery mode */
	VBERROR_LOAD_KERNEL_RECOVERY          = 0x10011,
	/* Other error inside VbSelectAndLoadKernel() */
	VBERROR_LOAD_KERNEL                   = 0x10012,
	/* Invalid Google binary block */
	VBERROR_INVALID_GBB                   = 0x10013,
	/* Invalid bitmap volume */
	VBERROR_INVALID_BMPFV                 = 0x10014,
	/* Invalid screen index */
	VBERROR_INVALID_SCREEN_INDEX          = 0x10015,
	/* Simulated (test) error */
	VBERROR_SIMULATED                     = 0x10016,
	/* Invalid parameter */
	VBERROR_INVALID_PARAMETER             = 0x10017,
	/* VbExBeep() can't make sounds at all */
	VBERROR_NO_SOUND                      = 0x10018,
	/* VbExBeep() can't make sound in the background */
	VBERROR_NO_BACKGROUND_SOUND           = 0x10019,
	/* Need EC to reboot to read-only code to switch RW slot */
	VBERROR_EC_REBOOT_TO_SWITCH_RW        = 0x1001A,
	/* Developer has requested a BIOS shell */
	VBERROR_BIOS_SHELL_REQUESTED          = 0x10020,
	/* Need VGA and don't have it, or vice-versa */
	VBERROR_VGA_OPROM_MISMATCH            = 0x10021,
	/* Need EC to reboot to read-only code */
	VBERROR_EC_REBOOT_TO_RO_REQUIRED      = 0x10022,
	/* Invalid region read parameters */
	VBERROR_REGION_READ_INVALID           = 0x10023,
	/* Cannot read from region */
	VBERROR_REGION_READ_FAILED            = 0x10024,
	/* Unsupported region type */
	VBERROR_UNSUPPORTED_REGION            = 0x10025,
	/* No image present (was returned from VbGbbReadImage(); deprecated */
	VBERROR_NO_IMAGE_PRESENT              = 0x10026,
	/* failed to draw screen */
	VBERROR_SCREEN_DRAW                   = 0x10027,
	/* failed to jump to RW image */
	VBERROR_RW_JUMP_FAILED                = 0x10028,
	/* Error reading FWMP from TPM (note: not present is not an error) */
	VBERROR_TPM_READ_FWMP                 = 0x10029,

	/* VbExEcGetExpectedRWHash() may return the following codes */
	/* Compute expected RW hash from the EC image; BIOS doesn't have it */
	VBERROR_EC_GET_EXPECTED_HASH_FROM_IMAGE = 0x20000,

	/* Detachable UI internal functions may return the following codes */
	/* No error; return to UI loop */
	VBERROR_KEEP_LOOPING			= 0x30000,
};


/*****************************************************************************/
/* Main entry points from firmware into vboot_reference */

/*
 * Minimum and recommended size of shared_data_blob in bytes.  Shared data blob
 * is used to communicate data between calls to VbInit(), VbSelectFirmware(),
 * the OS.  Minimum size is enough to hold all required data for verified boot
 * but may not be able to hold debug output.
 */
#define VB_SHARED_DATA_MIN_SIZE 3072
#define VB_SHARED_DATA_REC_SIZE 16384

/*
 * Data passed by firmware to VbInit(), VbSelectFirmware() and
 * VbSelectAndLoadKernel().
 *
 * Note that in UEFI, these are called by different phases in different
 * processor modes (VbInit() and VbSelectFirmware() = 32-bit PEI,
 * VbSelectAndLoadKernel() = 64-bit BDS), so the data may be at a different
 * location between calls.
 */
typedef struct VbCommonParams {
	/* Pointer to GBB data */
	void *gbb_data;
	/* Size of GBB data in bytes */
	uint32_t gbb_size;

	/*
	 * Shared data blob for data shared between verified boot entry points.
	 * This should be at least VB_SHARED_DATA_MIN_SIZE bytes long, and
	 * ideally is VB_SHARED_DATA_REC_SIZE bytes long.
	 */
	/* Pointer to shared data blob buffer */
	void *shared_data_blob;
	/*
	 * On input, set to size of shared data blob buffer, in bytes.  On
	 * output, this will contain the actual data size placed into the
	 * buffer.
	 */
	uint32_t shared_data_size;

	/*
	 * Internal context/data for verified boot, to maintain state during
	 * calls to other API functions such as VbExHashFirmwareBody().
	 * Allocated and freed inside the entry point; firmware should not look
	 * at this.
	 */
	void *vboot_context;
} VbCommonParams;

/* Flags for VbExGetSwitches() */
/* Recovery button was pressed at boot time. */
#define VB_INIT_FLAG_REC_BUTTON_PRESSED  0x00000002
/* Allow USB boot on transition to dev */
#define VB_INIT_FLAG_ALLOW_USB_BOOT	 0x00004000
/* Mask of deprecated flags */
#define VB_INIT_FLAG_DEPRECATED          0x0000BFFD

/*
 * Output flags for VbInitParams.out_flags.  Used to indicate potential boot
 * paths and configuration to the calling firmware early in the boot process,
 * so that it can properly configure itself for the capabilities subsequently
 * required by VbSelectFirmware() and VbSelectAndLoadKernel().
 */
/*
 * Enable recovery path.  Do not rely on any rewritable data (cached RAM
 * timings, etc.).  Reliable operation is more important than boot speed.
 */
#define VB_INIT_OUT_ENABLE_RECOVERY      0x00000001
/* RAM must be cleared before calling VbSelectFirmware(). */
#define VB_INIT_OUT_CLEAR_RAM            0x00000002
/*
 * Load display drivers; VbExDisplay*() functions may be called.  If this flag
 * is not present, VbExDisplay*() functions will not be called this boot.
 */
#define VB_INIT_OUT_ENABLE_DISPLAY       0x00000004
/*
 * Load USB storage drivers; VbExDisk*() functions may be called with the
 * VB_DISK_FLAG_REMOVABLE flag.  If this flag is not present, VbExDisk*()
 * functions will only be called for fixed disks.
 */
#define VB_INIT_OUT_ENABLE_USB_STORAGE   0x00000008
/* Enable developer path. */
#define VB_INIT_OUT_ENABLE_DEVELOPER     0x00000080
/* Mask of deprecated flags */
#define VB_INIT_OUT_DEPRECATED           0x00000070

/* Data only used by VbInit() */
typedef struct VbInitParams {
	uint32_t deprecated; /* Was init flags */

	/* Outputs from VbInit(); valid only if it returns success. */
	/* Output flags for firmware; see VB_INIT_OUT_*) */
	uint32_t out_flags;
} VbInitParams;

/*
 * Firmware types for VbHashFirmwareBody() and
 * VbSelectFirmwareParams.selected_firmware.  Note that we store these in a
 * uint32_t because enum maps to int, which isn't fixed-size.
 */
enum VbSelectFirmware_t {
	/* Read only firmware for normal or developer path. */
	VB_SELECT_FIRMWARE_READONLY = 3,
	/* Rewritable EC firmware currently set active */
	VB_SELECT_FIRMWARE_EC_ACTIVE = 4,
	/* Rewritable EC firmware currently not set active thus updatable */
	VB_SELECT_FIRMWARE_EC_UPDATE = 5,
	/* Keep this at the end */
	VB_SELECT_FIRMWARE_COUNT,
};

/*
 * We use disk handles rather than indices.  Using indices causes problems if
 * a disk is removed/inserted in the middle of processing.
 */
typedef void *VbExDiskHandle_t;

/* Data used only by VbSelectAndLoadKernel() */
typedef struct VbSelectAndLoadKernelParams {
	/* Inputs to VbSelectAndLoadKernel() */
	/* Destination buffer for kernel (normally at 0x100000 on x86) */
	void *kernel_buffer;
	/* Size of kernel buffer in bytes */
	uint32_t kernel_buffer_size;
	/* input flags.  Currently used for detachables */
	uint32_t inflags;

	/*
	 * Outputs from VbSelectAndLoadKernel(); valid only if it returns
	 * success.
	 */
	/* Handle of disk containing loaded kernel */
	VbExDiskHandle_t disk_handle;
	/* Partition number on disk to boot (1...M) */
	uint32_t partition_number;
	/* Address of bootloader image in RAM */
	uint64_t bootloader_address;
	/* Size of bootloader image in bytes */
	uint32_t bootloader_size;
	/* UniquePartitionGuid for boot partition */
	uint8_t partition_guid[16];
	/* Flags set by signer */
	uint32_t flags;
	/*
	 * TODO: in H2C, all that pretty much just gets passed to the
	 * bootloader as KernelBootloaderOptions, though the disk handle is
	 * passed as an index instead of a handle.  Is that used anymore now
	 * that we're passing partition_guid?
	 */
} VbSelectAndLoadKernelParams;

/* Flag to indicate using detachable menu ui (arrow up/down+power)
 * instead of traditional FW screens with ctrl+D, ctrl+U, etc.
 */
#define VB_SALK_INFLAGS_ENABLE_DETACHABLE_UI (1 << 0)

/**
 * Select and loads the kernel.
 *
 * Returns VBERROR_SUCCESS if success, non-zero if error; on error, caller
 * should reboot. */
VbError_t VbSelectAndLoadKernel(VbCommonParams *cparams,
                                VbSelectAndLoadKernelParams *kparams);

/**
 * Verify Kernel Image loaded in memory.
 *
 * This routine is used by fastboot boot command to verify the kernel image in
 * memory sent by the host device using fastboot protocol. It checks if the
 * image in memory is signed using official recovery keys. In case of GBB
 * override to allow full fastboot functionality, it checks image integrity, but
 * does not check the image signature.
 *
 * @param cparams	Common parameters, e.g. use member caller_context
 *			to point to useful context data
 * @param kparams	kernel params
 * @param boot_image	Image in memory that needs to be verified
 * @param image_size	Size of the image in memory
 * @return VBERROR_... error, VBERROR_SUCCESS on success.
 */
VbError_t VbVerifyMemoryBootImage(VbCommonParams *cparams,
				  VbSelectAndLoadKernelParams *kparams,
				  void *boot_image,
				  size_t image_size);

/**
 * Fastboot API to enter dev mode.
 *
 * This routine is used by fastboot oem unlock command to switch the device into
 * dev mode.
 *
 * NOTE: The caller MUST be in read-only firmware, and MUST have just obtained
 * explicit physical confirmation from the user via a trusted input method
 * before calling this function! Also, on successful return from this function,
 * the caller needs to reboot the device immediately for changes to take effect.
 *
 * @return VBERROR_... error, VBERROR_SUCCESS on success.
 */
VbError_t VbUnlockDevice(void);

/*****************************************************************************/
/* Debug output (from utility.h) */

/**
 * Output an error message and quit.  Does not return.  Supports
 * printf()-style formatting.
 */
void VbExError(const char *format, ...);

/**
 * Output a debug message.  Supports printf()-style formatting.
 */
void VbExDebug(const char *format, ...)
	__attribute__ ((format (__printf__, 1, 2)));

/*****************************************************************************/
/* Timer and delay (first two from utility.h) */

/**
 * Read a microsecond timer.
 *
 * This should have a sufficient number of bits to avoid wraparound for at
 * least 10 minutes.  A 32-bit value would be plenty, but for historical
 * reasons this returns uint64_t.
 */
uint64_t VbExGetTimer(void);

/**
 * Delay for at least the specified number of milliseconds.  Should be accurate
 * to within 10% (a requested delay of 1000 ms should result in an actual delay
 * of between 1000 - 1100 ms).
 */
void VbExSleepMs(uint32_t msec);

/**
 * Play a beep tone of the specified frequency in Hz and duration in msec.
 * This is effectively a VbSleep() variant that makes noise.
 *
 * If the audio codec can run in the background, then:
 *   zero frequency means OFF, non-zero frequency means ON
 *   zero msec means return immediately, non-zero msec means delay (and
 *     then OFF if needed)
 * otherwise,
 *   non-zero msec and non-zero frequency means ON, delay, OFF, return
 *   zero msec or zero frequency means do nothing and return immediately
 *
 * The return value is used by the caller to determine the capabilities. The
 * implementation should always do the best it can if it cannot fully support
 * all features - for example, beeping at a fixed frequency if frequency
 * support is not available.  At a minimum, it must delay for the specified
 * non-zero duration.
 */
VbError_t VbExBeep(uint32_t msec, uint32_t frequency);

/*****************************************************************************/
/* TPM (from tlcl_stub.h) */

/**
 * Initialize the stub library. */
VbError_t VbExTpmInit(void);

/**
 * Close and open the device.  This is needed for running more complex commands
 * at user level, such as TPM_TakeOwnership, since the TPM device can be opened
 * only by one process at a time.
 */
VbError_t VbExTpmClose(void);
VbError_t VbExTpmOpen(void);

/**
 * Send a request_length-byte request to the TPM and receive a response.  On
 * input, response_length is the size of the response buffer in bytes.  On
 * exit, response_length is set to the actual received response length in
 * bytes. */
VbError_t VbExTpmSendReceive(const uint8_t *request, uint32_t request_length,
                             uint8_t *response, uint32_t *response_length);

#ifdef CHROMEOS_ENVIRONMENT

/**
 * Obtain cryptographically secure random bytes. This function is used to
 * generate random nonces for TPM auth sessions for example. As an implication,
 * the generated random bytes should not be predictable for a TPM communication
 * interception attack. This implies a local source of randomness should be
 * used, i.e. this should not be wired to the TPM RNG directly. Otherwise, an
 * attacker with communication interception abilities could launch replay
 * attacks by reusing previous nonces.
 */
VbError_t VbExTpmGetRandom(uint8_t *buf, uint32_t length);

#endif  /* CHROMEOS_ENVIRONMENT */

/*****************************************************************************/
/* Non-volatile storage */

#define VBNV_BLOCK_SIZE 16  /* Size of NV storage block in bytes */

/**
 * Read the VBNV_BLOCK_SIZE-byte non-volatile storage into buf.
 */
VbError_t VbExNvStorageRead(uint8_t *buf);

/**
 * Write the VBNV_BLOCK_SIZE-byte non-volatile storage from buf.
 */
VbError_t VbExNvStorageWrite(const uint8_t *buf);

/*****************************************************************************/
/* Disk access (previously in boot_device.h) */

/* Flags for VbDisk APIs */
/* Disk is removable.  Example removable disks: SD cards, USB keys.  */
#define VB_DISK_FLAG_REMOVABLE 0x00000001
/*
 * Disk is fixed.  If this flag is present, disk is internal to the system and
 * not removable.  Example fixed disks: internal SATA SSD, eMMC.
 */
#define VB_DISK_FLAG_FIXED     0x00000002
/*
 * Note that VB_DISK_FLAG_REMOVABLE and VB_DISK_FLAG_FIXED are
 * mutually-exclusive for a single disk.  VbExDiskGetInfo() may specify both
 * flags to request disks of both types in a single call.
 *
 * At some point we could specify additional flags, but we don't currently
 * have a way to make use of these:
 *
 * USB              Device is known to be attached to USB.  Note that the SD
 *                  card reader inside x86 systems is attached to USB so this
 *                  isn't super useful.
 * SD               Device is known to be a SD card.  Note that external card
 *                  readers might not return this information, so also of
 *                  questionable use.
 * READ_ONLY        Device is known to be read-only.  Could be used by recovery
 *                  when processing read-only recovery image.
 */

/*
 * Disks are used in two ways:
 * - As a random-access device to read and write the GPT
 * - As a streaming device to read the kernel
 * These are implemented differently on raw NAND vs eMMC/SATA/USB
 * - On eMMC/SATA/USB, both of these refer to the same underlying
 *   storage, so they have the same size and LBA size. In this case,
 *   the GPT should not point to the same address as itself.
 * - On raw NAND, the GPT is held on a portion of the SPI flash.
 *   Random access GPT operations refer to the SPI and streaming
 *   operations refer to NAND. The GPT may therefore point into
 *   the same offsets as itself.
 * These types are distinguished by the following flag and VbDiskInfo
 * has separate fields to describe the random-access ("GPT") and
 * streaming aspects of the disk. If a disk is random-access (i.e.
 * not raw NAND) then these fields are equal.
 */
#define VB_DISK_FLAG_EXTERNAL_GPT	0x00000004

/* Information on a single disk */
typedef struct VbDiskInfo {
	/* Disk handle */
	VbExDiskHandle_t handle;
	/* Size of a random-access LBA sector in bytes */
	uint64_t bytes_per_lba;
	/* Number of random-access LBA sectors on the device.
	 * If streaming_lba_count is 0, this stands in for the size of the
	 * randomly accessed portion as well as the streaming portion.
	 * Otherwise, this is only the randomly-accessed portion. */
	uint64_t lba_count;
	/* Number of streaming sectors on the device */
	uint64_t streaming_lba_count;
	/* Flags (see VB_DISK_FLAG_* constants) */
	uint32_t flags;
	/*
	 * Optional name string, for use in debugging.  May be empty or null if
	 * not available.
	 */
	const char *name;
} VbDiskInfo;

/**
 * Store information into [info] for all disks (storage devices) attached to
 * the system which match all of the disk_flags.
 *
 * On output, count indicates how many disks are present, and [infos_ptr]
 * points to a [count]-sized array of VbDiskInfo structs with the information
 * on those disks; this pointer must be freed by calling VbExDiskFreeInfo().
 * If count=0, infos_ptr may point to NULL.  If [infos_ptr] points to NULL
 * because count=0 or error, it is not necessary to call VbExDiskFreeInfo().
 *
 * A multi-function device (such as a 4-in-1 card reader) should provide
 * multiple disk handles.
 *
 * The firmware must not alter or free the list pointed to by [infos_ptr] until
 * VbExDiskFreeInfo() is called.
 */
VbError_t VbExDiskGetInfo(VbDiskInfo **infos_ptr, uint32_t *count,
                          uint32_t disk_flags);

/**
 * Free a disk information list [infos] previously returned by
 * VbExDiskGetInfo().  If [preserve_handle] != NULL, the firmware must ensure
 * that handle remains valid after this call; all other handles from the info
 * list need not remain valid after this call.
 */
VbError_t VbExDiskFreeInfo(VbDiskInfo *infos,
                           VbExDiskHandle_t preserve_handle);

/**
 * Read lba_count LBA sectors, starting at sector lba_start, from the disk,
 * into the buffer.
 *
 * This is used for random access to the GPT. It is not for the partition
 * contents. The upper limit is lba_count.
 *
 * If the disk handle is invalid (for example, the handle refers to a disk
 * which as been removed), the function must return error but must not
 * crash.
 */
VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void *buffer);

/**
 * Write lba_count LBA sectors, starting at sector lba_start, to the disk, from
 * the buffer.
 *
 * This is used for random access to the GPT. It does not (necessarily) access
 * the streaming portion of the device.
 *
 * If the disk handle is invalid (for example, the handle refers to a disk
 * which as been removed), the function must return error but must not
 * crash.
 */
VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
                        uint64_t lba_count, const void *buffer);

/* Streaming read interface */
typedef void *VbExStream_t;

/**
 * Open a stream on a disk
 *
 * @param handle	Disk to open the stream against
 * @param lba_start	Starting sector offset within the disk to stream from
 * @param lba_count	Maximum extent of the stream in sectors
 * @param stream	out-paramter for the generated stream
 *
 * @return Error code, or VBERROR_SUCCESS.
 *
 * This is used for access to the contents of the actual partitions on the
 * device. It is not used to access the GPT. The size of the content addressed
 * is within streaming_lba_count.
 */
VbError_t VbExStreamOpen(VbExDiskHandle_t handle, uint64_t lba_start,
			 uint64_t lba_count, VbExStream_t *stream_ptr);

/**
 * Read from a stream on a disk
 *
 * @param stream	Stream to read from
 * @param bytes		Number of bytes to read
 * @param buffer	Destination to read into
 *
 * @return Error code, or VBERROR_SUCCESS. Failure to read as much data as
 * requested is an error.
 *
 * This is used for access to the contents of the actual partitions on the
 * device. It is not used to access the GPT.
 */
VbError_t VbExStreamRead(VbExStream_t stream, uint32_t bytes, void *buffer);

/**
 * Close a stream
 *
 * @param stream	Stream to close
 */
void VbExStreamClose(VbExStream_t stream);


/*****************************************************************************/
/* Display */

/* Predefined (default) screens for VbExDisplayScreen(). */
enum VbScreenType_t {
	/* Blank (clear) screen */
	VB_SCREEN_BLANK = 0,
	/* Developer - warning */
	VB_SCREEN_DEVELOPER_WARNING = 0x101,
	/* REMOVED: Developer - easter egg (0x102) */
	/* REMOVED: Recovery - remove inserted devices (0x201) */
	/* Recovery - insert recovery image */
	VB_SCREEN_RECOVERY_INSERT   = 0x202,
	/* Recovery - inserted image invalid */
	VB_SCREEN_RECOVERY_NO_GOOD  = 0x203,
	/* Recovery - confirm dev mode */
	VB_SCREEN_RECOVERY_TO_DEV   = 0x204,
	/* Developer - confirm normal mode */
	VB_SCREEN_DEVELOPER_TO_NORM = 0x205,
	/* Please wait - programming EC */
	VB_SCREEN_WAIT              = 0x206,
	/* Confirm after DEVELOPER_TO_NORM */
	VB_SCREEN_TO_NORM_CONFIRMED = 0x207,
	/* Broken screen shown after verification failure */
	VB_SCREEN_OS_BROKEN         = 0x208,
	/* REMOVED: Display base screen (no icons, no text) (0x209) */
	/* Detachable Menu - Developer Warning */
	VB_SCREEN_DEVELOPER_WARNING_MENU = 0x20a,
	/* Detachable Menu - Developer Boot */
	VB_SCREEN_DEVELOPER_MENU = 0x20b,
	/* REMOVED: Detachable Menu - Recovery (0x20c) */
	/* Detachable Menu - Confirm Dev Mode */
	VB_SCREEN_RECOVERY_TO_DEV_MENU = 0x20d,
	/* Detachable Menu - Confirm Normal Mode */
	VB_SCREEN_DEVELOPER_TO_NORM_MENU = 0x20e,
	/* Detachable Menu - Languages */
	VB_SCREEN_LANGUAGES_MENU = 0x20f,
	/* Detachable Menu - Options */
	VB_SCREEN_OPTIONS_MENU = 0x210,
};

/**
 * Display a predefined screen; see VB_SCREEN_* for valid screens.
 *
 * This is a backup method of screen display, intended for use if the GBB does
 * not contain a full set of bitmaps.  It is acceptable for the backup screen
 * to be simple ASCII text such as "NO GOOD" or "INSERT"; these screens should
 * only be seen during development.
 */
VbError_t VbExDisplayScreen(uint32_t screen_type, uint32_t locale);

/**
 * Display a predefined menu screen; see VB_SCREEN_* for valid screens.
 *
 * @param screen_type       ID of screen to draw
 * @param locale            language to display
 * @param selected_index    Index of menu item that is currently selected.
 * @param disabled_idx_mask Bitmap for enabling/disabling certain menu items.
 *                          each bit corresponds to the menu item's index.
 * @param redraw_base       Setting 1 will force a full redraw of the screen
 *
 * @return VBERROR_SUCCESS or error code on error.
 */
VbError_t VbExDisplayMenu(uint32_t screen_type, uint32_t locale,
			  uint32_t selected_index, uint32_t disabled_idx_mask,
			  uint32_t redraw_base);

/**
 * Display a string containing debug information on the screen, rendered in a
 * platform-dependent font.  Should be able to handle newlines '\n' in the
 * string.  Firmware must support displaying at least 20 lines of text, where
 * each line may be at least 80 characters long.  If the firmware has its own
 * debug state, it may display it to the screen below this information.
 *
 * NOTE: This is what we currently display when TAB is pressed.  Some
 * information (HWID, recovery reason) is ours; some (CMOS breadcrumbs) is
 * platform-specific.  If we decide to soft-render the HWID string
 * (chrome-os-partner:3693), we'll need to maintain our own fonts, so we'll
 * likely display it via VbExDisplayImage() above.
 */
VbError_t VbExDisplayDebugInfo(const char *info_str);

/*****************************************************************************/
/* Keyboard and switches */

/* Key codes for required non-printable-ASCII characters. */
enum VbKeyCode_t {
	VB_KEY_UP = 0x100,
	VB_KEY_DOWN = 0x101,
	VB_KEY_LEFT = 0x102,
	VB_KEY_RIGHT = 0x103,
	VB_KEY_CTRL_ENTER = 0x104
};

/*
 * WARNING!!! Before updating the codes in enum VbButtonCode_t, ensure that the
 * code does not overlap the values in VbKeyCode_t unless the button action is
 * the same as key action.
 */
enum VbButtonCode_t {
	/* Volume up/down short press match the values in 8042 driver. */
	VB_BUTTON_VOL_UP_SHORT_PRESS = 0x62,
	VB_BUTTON_VOL_DOWN_SHORT_PRESS = 0x63,
	/* Dummy values used below. */
	VB_BUTTON_POWER_SHORT_PRESS = 0x90,
	VB_BUTTON_VOL_UP_LONG_PRESS = 0x91,
	VB_BUTTON_VOL_DOWN_LONG_PRESS = 0x92,
	VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS = 0x93,
};

/* Flags for additional information.
 * TODO(semenzato): consider adding flags for modifiers instead of
 * making up some of the key codes above.
 */
enum VbKeyFlags_t {
	VB_KEY_FLAG_TRUSTED_KEYBOARD = 1 << 0,
};

/**
 * Read the next keypress from the keyboard buffer.
 *
 * Returns the keypress, or zero if no keypress is pending or error.
 *
 * The following keys must be returned as ASCII character codes:
 *    0x08          Backspace
 *    0x09          Tab
 *    0x0D          Enter (carriage return)
 *    0x01 - 0x1A   Ctrl+A - Ctrl+Z (yes, those alias with backspace/tab/enter)
 *    0x1B          Esc
 *    0x20          Space
 *    0x30 - 0x39   '0' - '9'
 *    0x60 - 0x7A   'a' - 'z'
 *
 * Some extended keys must also be supported; see the VB_KEY_* defines above.
 *
 * Keys ('/') or key-chords (Fn+Q) not defined above may be handled in any of
 * the following ways:
 *    1. Filter (don't report anything if one of these keys is pressed).
 *    2. Report as ASCII (if a well-defined ASCII value exists for the key).
 *    3. Report as any other value in the range 0x200 - 0x2FF.
 * It is not permitted to report a key as a multi-byte code (for example,
 * sending an arrow key as the sequence of keys '\x1b', '[', '1', 'A'). */
uint32_t VbExKeyboardRead(void);

/**
 * Same as VbExKeyboardRead(), but return extra information.
 */
uint32_t VbExKeyboardReadWithFlags(uint32_t *flags_ptr);

/**
 * Return the current state of the switches specified in request_mask
 */
uint32_t VbExGetSwitches(uint32_t request_mask);

/*****************************************************************************/
/* Embedded controller (EC) */

/*
 * All these functions take a devidx parameter, which indicates which embedded
 * processor the call applies to.  At present, only devidx=0 is valid, but
 * upcoming CLs will add support for multiple devices.
 */

/**
 * This is called only if the system implements a keyboard-based (virtual)
 * developer switch. It must return true only if the system has an embedded
 * controller which is provably running in its RO firmware at the time the
 * function is called.
 */
int VbExTrustEC(int devidx);

/**
 * Check if the EC is currently running rewritable code.
 *
 * If the EC is in RO code, sets *in_rw=0.
 * If the EC is in RW code, sets *in_rw non-zero.
 * If the current EC image is unknown, returns error. */
VbError_t VbExEcRunningRW(int devidx, int *in_rw);

/**
 * Request the EC jump to its rewritable code.  If successful, returns when the
 * EC has booting its RW code far enough to respond to subsequent commands.
 * Does nothing if the EC is already in its rewritable code.
 */
VbError_t VbExEcJumpToRW(int devidx);

/**
 * Tell the EC to refuse another jump until it reboots. Subsequent calls to
 * VbExEcJumpToRW() in this boot will fail.
 */
VbError_t VbExEcDisableJump(int devidx);

/**
 * Read the SHA-256 hash of the selected EC image.
 *
 * @param devidx    Device index. 0: EC, 1: PD.
 * @param select    Image to get hash of. RO or RW.
 * @param hash      Pointer to the hash.
 * @param hash_size Pointer to the hash size.
 * @return          VBERROR_... error, VBERROR_SUCCESS on success.
 */
VbError_t VbExEcHashImage(int devidx, enum VbSelectFirmware_t select,
			  const uint8_t **hash, int *hash_size);

/**
 * Get the expected contents of the EC image associated with the main firmware
 * specified by the "select" argument.
 */
VbError_t VbExEcGetExpectedImage(int devidx, enum VbSelectFirmware_t select,
				 const uint8_t **image, int *image_size);

/**
 * Read the SHA-256 hash of the expected contents of the EC image associated
 * with the main firmware specified by the "select" argument.
 */
VbError_t VbExEcGetExpectedImageHash(int devidx, enum VbSelectFirmware_t select,
				     const uint8_t **hash, int *hash_size);

/**
 * Update the selected EC image.
 */
VbError_t VbExEcUpdateImage(int devidx, enum VbSelectFirmware_t select,
			    const uint8_t *image, int image_size);

/**
 * Lock the selected EC code to prevent updates until the EC is rebooted.
 * Subsequent calls to VbExEcUpdateImage() with the same region this boot will
 * fail.
 */
VbError_t VbExEcProtect(int devidx, enum VbSelectFirmware_t select);

/**
 * Info the EC of the boot mode selected by the AP.
 * mode: Normal, Developer, or Recovery
 */
enum VbEcBootMode_t {VB_EC_NORMAL, VB_EC_DEVELOPER, VB_EC_RECOVERY };
VbError_t VbExEcEnteringMode(int devidx, enum VbEcBootMode_t mode);

/**
 * Perform EC post-verification / updating / jumping actions.
 *
 * This routine is called to perform certain actions that must wait until
 * after the EC resides in its `final` image (the image the EC will
 * run for the duration of boot). These actions include verifying that
 * enough power is available to continue with boot.
 *
 * @param in_recovery	1 if recovery mode is selected by the AP, 0 otherwise.
 * @return VBERROR_... error, VBERROR_SUCCESS on success.
 */
VbError_t VbExEcVbootDone(int in_recovery);

/**
 * Request EC to stop discharging and cut-off battery.
 */
VbError_t VbExEcBatteryCutOff(void);

/*
 * severity levels for an auxiliary firmware update request
 */
typedef enum {
	/* no update needed */
	VB_AUX_FW_NO_UPDATE = 0,
	/* update needed, can be done quickly */
	VB_AUX_FW_FAST_UPDATE = 1,
	/* update needed, "this would take a while..." */
	VB_AUX_FW_SLOW_UPDATE = 2,
} VbAuxFwUpdateSeverity_t;

/**
 * Perform auxiliary firmware checks.
 *
 * This is called after the EC has been updated and is intended to
 * version-check additional firmware blobs such as TCPCs.
 *
 * @param severity	return parameter for health of auxiliary firmware
 *			0: no update needed
 *			1: fast update needed
 *			2: slow update needed
 * @return VBERROR_... error, VBERROR_SUCCESS on success.
 */

VbError_t VbExCheckAuxFw(VbAuxFwUpdateSeverity_t *severity);

/**
 * Perform auxiliary firmware update(s).
 *
 * This is called after the EC has been updated and is intended to
 * update additional firmware blobs such as TCPCs.
 *
 * @return VBERROR_... error, VBERROR_SUCCESS on success.
 */

VbError_t VbExUpdateAuxFw(void);

/*****************************************************************************/
/* Misc */

/**
 * Check if the firmware needs to shut down the system.
 *
 * Returns a non-zero VB_SHUTDOWN_REQUEST mask indicating the reason(s) for
 * shutdown if a shutdown is being requested (see VB_SHUTDOWN_REQUEST_*), or 0
 * if a shutdown is not being requested.
 *
 * NOTE: When we're displaying a screen, pressing the power button should shut
 * down the computer.  We need a way to break out of our control loop so this
 * can occur cleanly.
 */
uint32_t VbExIsShutdownRequested(void);

/*
 * Shutdown requested for a reason which is not defined among other
 * VB_SHUTDOWN_REQUEST_* values. This must be defined as 1 for backward
 * compatibility with old versions of the API.
 */
#define VB_SHUTDOWN_REQUEST_OTHER		0x00000001
/* Shutdown requested due to a lid switch being closed. */
#define VB_SHUTDOWN_REQUEST_LID_CLOSED		0x00000002
/* Shutdown requested due to a power button being pressed. */
#define VB_SHUTDOWN_REQUEST_POWER_BUTTON	0x00000004

/* Constants for compression_type */
enum {
	COMPRESS_NONE = 0,
	COMPRESS_EFIv1,           /* The x86 BIOS only supports this */
	COMPRESS_LZMA1,           /* The ARM BIOS supports LZMA1 */
	MAX_COMPRESS,
};

/**
 * Execute legacy boot option.
 */
int VbExLegacy(void);

/* Regions for VbExRegionRead() */
enum vb_firmware_region {
	VB_REGION_GBB,	/* Google Binary Block - see gbbheader.h */

	VB_REGION_COUNT,
};

/**
 * Check if the firmware wants to override GPT entry priority.
 *
 * In case of kernel entry, check if there is an override of priority
 * available. This is used to select a particular partition to boot in the
 * current boot cycle. Rollback protection, image verification and all other
 * checks in LoadKernel still remain the same.
 *
 * @param e	Gpt Entry to check for priority override.
 * @return 0 if no override, 1-15 for override priority.
 */
uint8_t VbExOverrideGptEntryPriority(const GptEntry *e);

/**
 * Return number of locales supported
 *
 * @param count		Pointer to the number of locales.
 * @return VBERROR_... error, VBERROR_SUCCESS on success.
 */
VbError_t VbExGetLocalizationCount(uint32_t *count);
#endif  /* VBOOT_REFERENCE_VBOOT_API_H_ */
