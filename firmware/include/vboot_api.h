/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* APIs provided by firmware to vboot_reference. */

/* General notes:
 *
 * All verified boot functions now start with "Vb" for namespace
 * clarity.  This fixes the problem where uboot and vboot both defined
 * assert().
 *
 * Verified boot APIs to be implemented by the calling firmware and
 * exported to vboot_reference start with "VbEx".
 */
/* TODO: split this file into a vboot_entry_points.h file which
 * contains the entry points for the firmware to call vboot_reference,
 * and a vboot_firmware_exports.h which contains the APIs to be
 * implemented by the calling firmware and exported to
 * vboot_reference. */

#ifndef VBOOT_REFERENCE_VBOOT_API_H_
#define VBOOT_REFERENCE_VBOOT_API_H_

#include "sysincludes.h"
#include "bmpblk_header.h"


/*****************************************************************************/
/* Error codes */

/* Functions which return an error all return this type.  This is a
* 32-bit value rather than an int so it's consistent across UEFI,
* which is 32-bit during PEI and 64-bit during DXE/BDS. */
typedef uint32_t VbError_t;

/* No error; function completed successfully. */
#define VBERROR_SUCCESS 0
/* Errors are non-zero, but differ between functions.  For example,
 * the TPM functions may pass through TPM error codes, some of which
 * may be recoverable. */


/*****************************************************************************/
/* Main entry points from firmware into vboot_reference */

/* Minimum and recommended size of shared_data_blob in bytes.  Shared
 * data blob is used to communicate data between calls to VbInit(),
 * VbSelectFirmware(), the OS.  Minimum size is enough to hold all
 * required data for verified boot but may not be able to hold debug
 * output. */
#define VB_SHARED_DATA_MIN_SIZE 3072
#define VB_SHARED_DATA_REC_SIZE 16384

/* Data passed by firmware to VbInit(), VbSelectFirmware() and
 * VbSelectAndLoadKernel(). */
/* Note that in UEFI, these are called by different phases in
 * different processor modes (VbInit() and VbSelectFirmware() = 32-bit PEI,
 * VbSelectAndLoadKernel() = 64-bit BDS), so the data may be at a different
 * location between calls. */
typedef struct VbCommonParams {
  void* gbb_data;                /* Pointer to GBB data */
  uint32_t gbb_size;             /* Size of GBB data in bytes */

  /* Shared data blob for data shared between verified boot entry
   * points.  This should be at least VB_SHARED_DATA_MIN_SIZE bytes
   * long, and ideally is VB_SHARED_DATA_REC_SIZE bytes long. */
  void* shared_data_blob;        /* Pointer to shared data blob buffer */
  uint32_t shared_data_size;     /* On input, set to size of shared data blob
                                  * buffer, in bytes.  On output, this will
                                  * contain the actual data size placed into
                                  * the buffer. */

  /* Internal context/data for verified boot, to maintain state during
   * calls to other API functions such as VbExHashFirmwareBody().
   * Allocated and freed inside the entry point; firmware should not
   * look at this. */
  void* vboot_context;

  /* Internal context/data for firmware / VbExHashFirmwareBody().
   * Needed because the PEI phase of UEFI boot runs out of ROM and
   * thus can't modify global variables; everything needs to get
   * passed around on the stack. */
  void* caller_context;
} VbCommonParams;


/* Flags for VbInitParams.flags */
/* Developer switch was on at boot time. */
#define VB_INIT_FLAG_DEV_SWITCH_ON       0x00000001
/* Recovery button was pressed at boot time. */
#define VB_INIT_FLAG_REC_BUTTON_PRESSED  0x00000002
/* Hardware write protect was enabled at boot time. */
#define VB_INIT_FLAG_WP_ENABLED          0x00000004
/* This is a S3 resume, not a normal boot. */
#define VB_INIT_FLAG_S3_RESUME           0x00000008


/* Output flags for VbInitParams.out_flags.  Used to indicate
 * potential boot paths and configuration to the calling firmware
 * early in the boot process, so that it can properly configure itself
 * for the capabilities subsequently required by VbSelectFirmware()
 * and VbSelectAndLoadKernel(). */
/* Enable recovery path.  Do not rely on any rewritable data (cached
 * RAM timings, etc.).  Reliable operation is more important than boot
 * speed. */
#define VB_INIT_OUT_ENABLE_RECOVERY      0x00000001
/* RAM must be cleared before calling VbSelectFirmware(). */
#define VB_INIT_OUT_CLEAR_RAM            0x00000002
/* Load display drivers; VbExDisplay*() functions may be called.  If this flag
 * is not present, VbExDisplay*() functions will not be called this boot. */
#define VB_INIT_OUT_ENABLE_DISPLAY       0x00000004
/* Load USB storage drivers; VbExDisk*() functions may be called with the
 * VB_DISK_FLAG_REMOVABLE flag.  If this flag is not present, VbExDisk*()
 * functions will only be called for fixed disks. */
#define VB_INIT_OUT_ENABLE_USB_STORAGE   0x00000008
/* If this is a S3 resume, do a debug reset boot instead */
#define VB_INIT_OUT_S3_DEBUG_BOOT        0x00000010


/* Data only used by VbInit() */
typedef struct VbInitParams {
  /* Inputs to VbInit() */
  uint32_t flags;                /* Flags (see VB_INIT_FLAG_*) */

  /* Outputs from VbInit(); valid only if it returns success. */
  uint32_t out_flags;            /* Output flags for firmware; see
                                  * VB_INIT_OUT_*) */
} VbInitParams;


/* Firmware types for VbHashFirmwareBody() and
 * VbSelectFirmwareParams.selected_firmware.  Note that we store these
 * in a uint32_t because enum maps to int, which isn't fixed-size. */
enum VbSelectFirmware_t {
  VB_SELECT_FIRMWARE_RECOVERY = 0,
  VB_SELECT_FIRMWARE_A = 1,
  VB_SELECT_FIRMWARE_B = 2
};


/* Data only used by VbSelectFirmware() */
typedef struct VbSelectFirmwareParams {
  /* Inputs to VbSelectFirmware() */
  void* verification_block_A;    /* Key block + preamble for firmware A */
  void* verification_block_B;    /* Key block + preamble for firmware B */
  uint32_t verification_size_A;  /* Verification block A size in bytes */
  uint32_t verification_size_B;  /* Verification block B size in bytes */

  /* Outputs from VbSelectFirmware(); valid only if it returns success. */
  uint32_t selected_firmware;    /* Main firmware to run; see VB_SELECT_*. */
} VbSelectFirmwareParams;


/* We use disk handles rather than indices.  Using indices causes problems if
 * a disk is removed/inserted in the middle of processing. */
typedef void* VbExDiskHandle_t;


/* Data used only by VbSelectAndLoadKernel() */
typedef struct VbSelectAndLoadKernelParams {
  /* Inputs to VbSelectAndLoadKernel() */
  void* kernel_buffer;           /* Destination buffer for kernel
                                  * (normally at 0x100000 on x86) */
  uint32_t kernel_buffer_size;   /* Size of kernel buffer in bytes */

  /* Outputs from VbSelectAndLoadKernel(); valid only if it returns success. */
  VbExDiskHandle_t disk_handle;  /* Handle of disk containing loaded kernel */
  uint32_t partition_number;     /* Partition number on disk to boot (1...M) */
  uint64_t bootloader_address;   /* Address of bootloader image in RAM */
  uint32_t bootloader_size;      /* Size of bootloader image in bytes */
  uint8_t partition_guid[16];    /* UniquePartitionGuid for boot partition */
  /* TODO: in H2C, all that pretty much just gets passed to the bootloader
   * as KernelBootloaderOptions, though the disk handle is passed as an index
   * instead of a handle.  Is that used anymore now that we're passing
   * partition_guid? */
} VbSelectAndLoadKernelParams;


/* Initialize the verified boot library.
 *
 * Returns VBERROR_SUCCESS if success, non-zero if error; on error,
 * caller should reboot. */
VbError_t VbInit(VbCommonParams* cparams, VbInitParams* iparams);


/* Select the main firmware.
 *
 * Returns VBERROR_SUCCESS if success, non-zero if error; on error,
 * caller should reboot. */
/* NOTE: This is now called in all modes, including recovery.
 * Previously, LoadFirmware() was not called in recovery mode, which
 * meant that LoadKernel() needed to duplicate the TPM and
 * VbSharedData initialization code. */
VbError_t VbSelectFirmware(VbCommonParams* cparams,
                           VbSelectFirmwareParams* fparams);

/* Update the data hash for the current firmware image, extending it
 * by [size] bytes stored in [*data].  This function must only be
 * called inside VbExHashFirmwareBody(), which is in turn called by
 * VbSelectFirmware().  */
void VbUpdateFirmwareBodyHash(VbCommonParams* cparams,
                              uint8_t* data, uint32_t size);

/* Select and loads the kernel.
 *
 * Returns VBERROR_SUCCESS if success, non-zero if error; on error,
 * caller should reboot. */
VbError_t VbSelectAndLoadKernel(VbCommonParams* cparams,
                                VbSelectAndLoadKernelParams* kparams);

/* S3 resume handler.  This only needs to be called if the hardware
 * does not maintain power to the TPM when in S3 (suspend-to-RAM).
 *
 * Returns VBERROR_SUCCESS if success, non-zero if error; on error,
 * caller should reboot. */
VbError_t VbS3Resume(void);


/*****************************************************************************/
/* Debug output (from utility.h) */

/* Output an error message and quit.  Does not return.  Supports
 * printf()-style formatting. */
void VbExError(const char* format, ...);

/* Output a debug message.  Supports printf()-style formatting. */
void VbExDebug(const char* format, ...);


/*****************************************************************************/
/* Memory (from utility.h) */

/* Allocate [size] bytes and return a pointer to the allocated memory. Abort
 * on error; this always either returns a good pointer or never returns.
 *
 * If any of the firmware API implementations require aligned data
 * (for example, disk access on ARM), all pointers returned by
 * VbExMalloc() must also be aligned.  */
void* VbExMalloc(size_t size);

/* Free memory pointed to by [ptr] previously allocated by VbExMalloc(). */
void VbExFree(void* ptr);


/*****************************************************************************/
/* Timer and delay (first two from utility.h) */

/* Read a high-resolution timer.  Returns the current timer value in
 * arbitrary units.
 *
 * This is intended for benchmarking, so this call MUST be fast.  The
 * timer frequency must be >1 KHz (preferably >1 MHz), and the timer
 * must not wrap around for at least 10 minutes.  It is preferable
 * (but not required) that the timer be initialized to 0 at boot.
 *
 * It is assumed that the firmware has some other way of communicating
 * the timer frequency to the OS.  For example, on x86 we use TSC, and
 * the OS kernel reports the initial TSC value at kernel-start and
 * calculates the frequency. */
uint64_t VbExGetTimer(void);

/* Delay for at least the specified number of milliseconds.  Should be
 * accurate to within 10% (a requested delay of 1000 ms should
 * result in an actual delay of between 1000 - 1100 ms). */
void VbExSleepMs(uint32_t msec);

/* Play a beep tone of the specified frequency in Hz and duration in msec.
 * This is effectively a VbSleep() variant that makes noise.
 *
 * The implementation should do the best it can if it cannot fully
 * support this interface - for example, beeping at a fixed frequency
 * if frequency support is not available.  At a minimum, it must delay for
 * the specified duration. */
void VbExBeep(uint32_t msec, uint32_t frequency);


/*****************************************************************************/
/* TPM (from tlcl_stub.h) */

/* Initialize the stub library. */
VbError_t VbExTpmInit(void);

/* Close and open the device.  This is needed for running more complex commands
 * at user level, such as TPM_TakeOwnership, since the TPM device can be opened
 * only by one process at a time. */
VbError_t VbExTpmClose(void);
VbError_t VbExTpmOpen(void);

/* Send a request_length-byte request to the TPM and receive a
 * response.  On input, response_length is the size of the response
 * buffer in bytes.  On exit, response_length is set to the actual
 * received response length in bytes. */
VbError_t VbExTpmSendReceive(const uint8_t* request, uint32_t request_length,
                             uint8_t* response, uint32_t* response_length);


/*****************************************************************************/
/* Non-volatile storage */

#define VBNV_BLOCK_SIZE 16  /* Size of NV storage block in bytes */

/* Read the VBNV_BLOCK_SIZE-byte non-volatile storage into buf. */
VbError_t VbExNvStorageRead(uint8_t* buf);

/* Write the VBNV_BLOCK_SIZE-byte non-volatile storage from buf. */
VbError_t VbExNvStorageWrite(const uint8_t* buf);


/*****************************************************************************/
/* Firmware / EEPROM access (previously in load_firmware_fw.h) */

/* Calculate the hash of the firmware body data for [firmware_index],
 * which is either VB_SELECT_FIRMWARE_A or VB_SELECT_FIRMWARE B.
 *
 * This function must call VbUpdateFirmwareBodyHash() before
 * returning, to update the secure hash for the firmware image.  For
 * best performance, the implementation should call
 * VbUpdateFirmwareBodyHash() periodically during the read, so that
 * updating the hash can be pipelined with the read.  If the reader
 * cannot update the hash during the read process, it should call
 * VbUpdateFirmwareBodyHash() on the entire firmware data after the
 * read, before returning.
 *
 * It is recommended that the firmware use this call to copy the
 * requested firmware body from EEPROM into RAM, so that it doesn't
 * need to do a second slow copy from EEPROM to RAM if this firmware
 * body is selected.
 *
 * Note this function doesn't actually pass the firmware body data to
 * verified boot, because verified boot doesn't actually need the
 * firmware body, just its hash.  This is important on x86, where the
 * firmware is stored compressed.  We hash the compressed data, but
 * the BIOS decompresses it during read.  Simply updating a hash is
 * compatible with the x86 read-and-decompress pipeline. */
VbError_t VbExHashFirmwareBody(VbCommonParams* cparams,
                               uint32_t firmware_index);

/*****************************************************************************/
/* Disk access (previously in boot_device.h) */

/* Flags for VbDisk APIs */
/* Disk is removable.  Example removable disks: SD cards, USB keys.  */
#define VB_DISK_FLAG_REMOVABLE 0x00000001
/* Disk is fixed.  If this flag is present, disk is internal to the
 * system and not removable.  Example fixed disks: internal SATA SSD, eMMC. */
#define VB_DISK_FLAG_FIXED     0x00000002
/* Note that VB_DISK_FLAG_REMOVABLE and VB_DISK_FLAG_FIXED are
 * mutually-exclusive for a single disk.  VbExDiskGetInfo() may specify
 * both flags to request disks of both types in a single call. */
/* At some point we could specify additional flags, but we don't currently
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
 **/

/* Information on a single disk */
typedef struct VbDiskInfo {
  VbExDiskHandle_t handle;  /* Disk handle */
  uint64_t bytes_per_lba;   /* Size of a LBA sector in bytes */
  uint64_t lba_count;       /* Number of LBA sectors on the device */
  uint32_t flags;           /* Flags (see VB_DISK_FLAG_* constants) */
  const char* name;         /* Optional name string, for use in debugging.
                             * May be empty or null if not available. */
} VbDiskInfo;

/* Store information into [info] for all disks (storage devices)
 * attached to the system which match all of the disk_flags.
 *
 * On output, count indicates how many disks are present, and
 * [infos_ptr] points to a [count]-sized array of VbDiskInfo structs
 * with the information on those disks; this pointer must be freed by
 * calling VbExDiskFreeInfo().  If count=0, infos_ptr may point to
 * NULL.  If [infos_ptr] points to NULL because count=0 or error, it
 * is not necessary to call VbExDiskFreeInfo().
 *
 * A multi-function device (such as a 4-in-1 card reader) should provide
 * multiple disk handles.
 *
 * The firmware must not alter or free the list pointed to by
 * [infos_ptr] until VbExDiskFreeInfo() is called. */
VbError_t VbExDiskGetInfo(VbDiskInfo** infos_ptr, uint32_t* count,
                          uint32_t disk_flags);

/* Free a disk information list [infos] previously returned by
 * VbExDiskGetInfo().  If [preserve_handle] != NULL, the firmware must
 * ensure that handle remains valid after this call; all other handles
 * from the info list need not remain valid after this call. */
VbError_t VbExDiskFreeInfo(VbDiskInfo* infos,
                           VbExDiskHandle_t preserve_handle);

/* Read lba_count LBA sectors, starting at sector lba_start, from the disk,
 * into the buffer.
 *
 * If the disk handle is invalid (for example, the handle refers to a
 * disk which as been removed), the function must return error but
 * must not crash. */
VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void* buffer);

/* Write lba_count LBA sectors, starting at sector lba_start, to the
 * disk, from the buffer.
 *
 * If the disk handle is invalid (for example, the handle refers to a
 * disk which as been removed), the function must return error but
 * must not crash. */
VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
                        uint64_t lba_count, const void* buffer);


/*****************************************************************************/
/* Display */

/* Predefined (default) screens for VbExDisplayScreen(). */
enum VbScreenType_t {
  VB_SCREEN_BLANK = 0,                 /* Blank (clear) screen */
  VB_SCREEN_DEVELOPER_WARNING = 0x101, /* Developer - warning */
  VB_SCREEN_DEVELOPER_EGG     = 0x102, /* Developer - easter egg */
  VB_SCREEN_RECOVERY_REMOVE   = 0x201, /* Recovery - remove inserted devices */
  VB_SCREEN_RECOVERY_INSERT   = 0x202, /* Recovery - insert recovery image */
  VB_SCREEN_RECOVERY_NO_GOOD  = 0x203, /* Recovery - inserted image invalid */
};

/* Initialize and clear the display.  Set width and height to the screen
 * dimensions in pixels. */
VbError_t VbExDisplayInit(uint32_t* width, uint32_t* height);


/* Enable (enable!=0) or disable (enable=0) the display backlight. */
VbError_t VbExDisplayBacklight(uint8_t enable);


/* Display a predefined screen; see VB_SCREEN_* for valid screens.
 * This is a backup method of screen display, intended for use if the
 * GBB does not contain a full set of bitmaps.  It is acceptable for
 * the backup screen to be simple ASCII text such as "NO GOOD" or
 * "INSERT"; these screens should only be seen during development. */
VbError_t VbExDisplayScreen(uint32_t screen_type);


/* Write an image to the display, with the upper left corner at the
 * specified pixel coordinates.  The bitmap buffer is a
 * platform-dependent binary blob with size specified by
 * info->compressed_size. */
// TODO: I don't like passing an ImageInfo directly to the underlying firmware;
// should pass a struct defined in THIS header file, or individual params.
VbError_t VbExDisplayImage(uint32_t x, uint32_t y, const ImageInfo* info,
                           const void* buffer);

/* Display a string containing debug information on the screen,
 * rendered in a platform-dependent font.  Should be able to handle
 * newlines '\n' in the string.  Firmware must support displaying at
 * least 20 lines of text, where each line may be at least 80
 * characters long.  If the firmware has its own debug state, it may
 * display it to the screen below this information. */
VbError_t VbExDisplayDebugInfo(const char* info_str);
/* NOTE: This is what we currently display on ZGB/Alex when TAB is
 * pressed.  Some information (HWID, recovery reason) is ours; some
 * (CMOS breadcrumbs) is platform-specific.  If we decide to
 * soft-render the HWID string (chrome-os-partner:3693), we'll need to
 * maintain our own fonts, so we'll likely display it via
 * VbExDisplayBitmap() above. */


/*****************************************************************************/
/* Keyboard and switches */

/* Key codes for required non-printable-ASCII characters. */
enum VbKeyCode_t {
  VB_KEY_UP = 0x100,
  VB_KEY_DOWN = 0x101,
  VB_KEY_LEFT = 0x102,
  VB_KEY_RIGHT = 0x103
};

/* Read the next keypress from the keyboard buffer.
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


/*****************************************************************************/
/* Misc */

/* Checks if the firmware needs to shut down the system.
 *
 * Returns 1 if a shutdown is being requested (for example, the user has
 * pressed the power button or closed the lid), or 0 if a shutdown is not
 * being requested. */
/* NOTE: When we're displaying a screen, pressing the power button
 * should shut down the computer.  We need a way to break out of our
 * control loop so this can occur cleanly. */
uint32_t VbExIsShutdownRequested(void);

#endif  /* VBOOT_REFERENCE_VBOOT_API_H_ */
