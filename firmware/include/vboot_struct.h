/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure definitions for verified boot, for on-disk / in-eeprom
 * data.
 */

#ifndef VBOOT_REFERENCE_VBOOT_STRUCT_H_
#define VBOOT_REFERENCE_VBOOT_STRUCT_H_
#include <stdint.h>

/* Public key data */
typedef struct VbPublicKey {
	/* Offset of key data from start of this struct */
	uint64_t key_offset;
	/* Size of key data in bytes (NOT strength of key in bits) */
	uint64_t key_size;
	/* Signature algorithm used by the key */
	uint64_t algorithm;
	/* Key version */
	uint64_t key_version;
} __attribute__((packed)) VbPublicKey;

#define EXPECTED_VBPUBLICKEY_SIZE 32

/* Signature data (a secure hash, possibly signed) */
typedef struct VbSignature {
	/* Offset of signature data from start of this struct */
	uint64_t sig_offset;
	/* Size of signature data in bytes */
	uint64_t sig_size;
	/* Size of the data block which was signed in bytes */
	uint64_t data_size;
} __attribute__((packed)) VbSignature;

#define EXPECTED_VBSIGNATURE_SIZE 24

#define KEY_BLOCK_MAGIC "CHROMEOS"
#define KEY_BLOCK_MAGIC_SIZE 8

#define KEY_BLOCK_HEADER_VERSION_MAJOR 2
#define KEY_BLOCK_HEADER_VERSION_MINOR 1

/* Flags for key_block_flags */
/* The following flags set where the key is valid */
#define KEY_BLOCK_FLAG_DEVELOPER_0  (0x01ULL) /* Developer switch off */
#define KEY_BLOCK_FLAG_DEVELOPER_1  (0x02ULL) /* Developer switch on */
#define KEY_BLOCK_FLAG_RECOVERY_0   (0x04ULL) /* Not recovery mode */
#define KEY_BLOCK_FLAG_RECOVERY_1   (0x08ULL) /* Recovery mode */

/*
 * Key block, containing the public key used to sign some other chunk of data.
 *
 * This should be followed by:
 *   1) The data_key key data, pointed to by data_key.key_offset.
 *   2) The checksum data for (VBKeyBlockHeader + data_key data), pointed to
 *      by key_block_checksum.sig_offset.
 *   3) The signature data for (VBKeyBlockHeader + data_key data), pointed to
 *      by key_block_signature.sig_offset.
 */
typedef struct VbKeyBlockHeader {
	/* Magic number */
	uint8_t magic[KEY_BLOCK_MAGIC_SIZE];
	/* Version of this header format */
	uint32_t header_version_major;
	/* Version of this header format */
	uint32_t header_version_minor;
	/*
	 * Length of this entire key block, including keys, signatures, and
	 * padding, in bytes
	 */
	uint64_t key_block_size;
	/*
	 * Signature for this key block (header + data pointed to by data_key)
	 * For use with signed data keys
	 */
	VbSignature key_block_signature;
	/*
	 * SHA-512 checksum for this key block (header + data pointed to by
	 * data_key) For use with unsigned data keys
	 */
	VbSignature key_block_checksum;
	/* Flags for key (KEY_BLOCK_FLAG_*) */
	uint64_t key_block_flags;
	/* Key to verify the chunk of data */
	VbPublicKey data_key;
} __attribute__((packed)) VbKeyBlockHeader;

#define EXPECTED_VBKEYBLOCKHEADER_SIZE 112

/****************************************************************************/

#define FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR 2
#define FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR 1

/*
 * Preamble block for rewritable firmware, version 2.0.  All 2.x versions of
 * this struct must start with the same data, to be compatible with version 2.0
 * readers.
 */
typedef struct VbFirmwarePreambleHeader2_0 {
	/*
	 * Size of this preamble, including keys, signatures, and padding, in
	 * bytes
	 */
	uint64_t preamble_size;
	/*
	 * Signature for this preamble (header + kernel subkey + body
	 * signature)
	 */
	VbSignature preamble_signature;
	/* Version of this header format (= 2) */
	uint32_t header_version_major;
	/* Version of this header format (= 0) */
	uint32_t header_version_minor;

	/* Firmware version */
	uint64_t firmware_version;
	/* Key to verify kernel key block */
	VbPublicKey kernel_subkey;
	/* Signature for the firmware body */
	VbSignature body_signature;
} __attribute__((packed)) VbFirmwarePreambleHeader2_0;

#define EXPECTED_VBFIRMWAREPREAMBLEHEADER2_0_SIZE 104

/* Flags for VbFirmwarePreambleHeader.flags */
/*
 * Use the normal/dev boot path from the read-only firmware, instead of
 * verifying the body signature.
 */
#define VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL 0x00000001

/* Premable block for rewritable firmware, version 2.1.
 *
 * The firmware preamble header should be followed by:
 *   1) The kernel_subkey key data, pointed to by kernel_subkey.key_offset.
 *   2) The signature data for the firmware body, pointed to by
 *      body_signature.sig_offset.
 *   3) The signature data for (header + kernel_subkey data + body signature
 *      data), pointed to by preamble_signature.sig_offset.
 */
typedef struct VbFirmwarePreambleHeader {
	/*
	 * Size of this preamble, including keys, signatures, and padding, in
	 * bytes
	 */
	uint64_t preamble_size;
	/*
	 * Signature for this preamble (header + kernel subkey + body
	 * signature)
	 */
	VbSignature preamble_signature;
	/* Version of this header format */
	uint32_t header_version_major;
	/* Version of this header format */
	uint32_t header_version_minor;

	/* Firmware version */
	uint64_t firmware_version;
	/* Key to verify kernel key block */
	VbPublicKey kernel_subkey;
	/* Signature for the firmware body */
	VbSignature body_signature;

	/*
	 * Fields added in header version 2.1.  You must verify the header
	 * version before reading these fields!
	 */
	/*
	 * Flags; see VB_FIRMWARE_PREAMBLE_*.  Readers should return 0 for
	 * header version < 2.1.
	 */
	uint32_t flags;
} __attribute__((packed)) VbFirmwarePreambleHeader;

#define EXPECTED_VBFIRMWAREPREAMBLEHEADER2_1_SIZE 108

/****************************************************************************/

#define KERNEL_PREAMBLE_HEADER_VERSION_MAJOR 2
#define KERNEL_PREAMBLE_HEADER_VERSION_MINOR 2

/* Preamble block for kernel, version 2.0
 *
 * This should be followed by:
 *   1) The signature data for the kernel body, pointed to by
 *      body_signature.sig_offset.
 *   2) The signature data for (VBFirmwarePreambleHeader + body signature
 *      data), pointed to by preamble_signature.sig_offset.
 */
typedef struct VbKernelPreambleHeader2_0 {
	/*
	 * Size of this preamble, including keys, signatures, and padding, in
	 * bytes
	 */
	uint64_t preamble_size;
	/* Signature for this preamble (header + body signature) */
	VbSignature preamble_signature;
	/* Version of this header format */
	uint32_t header_version_major;
	/* Version of this header format */
	uint32_t header_version_minor;

	/* Kernel version */
	uint64_t kernel_version;
	/* Load address for kernel body */
	uint64_t body_load_address;
	/* Address of bootloader, after body is loaded at body_load_address */
	uint64_t bootloader_address;
	/* Size of bootloader in bytes */
	uint64_t bootloader_size;
	/* Signature for the kernel body */
	VbSignature body_signature;
} __attribute__((packed)) VbKernelPreambleHeader2_0;

#define EXPECTED_VBKERNELPREAMBLEHEADER2_0_SIZE 96

/* Preamble block for kernel, version 2.1
 *
 * This should be followed by:
 *   1) The signature data for the kernel body, pointed to by
 *      body_signature.sig_offset.
 *   2) The signature data for (VBFirmwarePreambleHeader + body signature
 *      data), pointed to by preamble_signature.sig_offset.
 *   3) The 16-bit vmlinuz header, which is used for reconstruction of
 *      vmlinuz image.
 */
typedef struct VbKernelPreambleHeader {
	/*
	 * Size of this preamble, including keys, signatures, vmlinuz header,
	 * and padding, in bytes
	 */
	uint64_t preamble_size;
	/* Signature for this preamble (header + body signature) */
	VbSignature preamble_signature;
	/* Version of this header format */
	uint32_t header_version_major;
	/* Version of this header format */
	uint32_t header_version_minor;

	/* Kernel version */
	uint64_t kernel_version;
	/* Load address for kernel body */
	uint64_t body_load_address;
	/* Address of bootloader, after body is loaded at body_load_address */
	uint64_t bootloader_address;
	/* Size of bootloader in bytes */
	uint64_t bootloader_size;
	/* Signature for the kernel body */
	VbSignature body_signature;
	/*
	 * Fields added in header version 2.1.  You must verify the header
	 * version before reading these fields!
	 */
	/* Address of 16-bit header for vmlinuz reassembly.  Readers should
	   return 0 for header version < 2.1 */
	uint64_t vmlinuz_header_address;
	/* Size of 16-bit header for vmlinuz in bytes.  Readers should return 0
	   for header version < 2.1 */
	uint64_t vmlinuz_header_size;
	/*
	 * Flags passed in by the signer. Readers should return 0 for header
	 * version < 2.2. Flags field is currently defined as:
	 * [31:2] - Reserved (for future use)
	 * [1:0]  - Kernel image type (0b00 - CrOS, 0b01 - bootimg)
	 */
	uint32_t flags;
} __attribute__((packed)) VbKernelPreambleHeader;

#define EXPECTED_VBKERNELPREAMBLEHEADER2_1_SIZE 112
#define EXPECTED_VBKERNELPREAMBLEHEADER2_2_SIZE 116

/****************************************************************************/

/* Constants and sub-structures for VbSharedDataHeader */

/* Magic number for recognizing VbSharedDataHeader ("VbSD") */
#define VB_SHARED_DATA_MAGIC 0x44536256

/* Minimum and recommended size of shared_data_blob in bytes. */
#define VB_SHARED_DATA_MIN_SIZE 3072
#define VB_SHARED_DATA_REC_SIZE 16384

/* Flags for VbSharedDataHeader */
/* LoadFirmware() tried firmware B because of VbNvStorage firmware B tries */
#define VBSD_FWB_TRIED                  0x00000001
/*
 * LoadKernel() verified the good kernel keyblock using the kernel subkey from
 * the firmware.  If this flag is not present, it just used the hash of the
 * kernel keyblock.
 */
#define VBSD_KERNEL_KEY_VERIFIED         0x00000002
/* LoadFirmware() was told the developer switch was on */
#define VBSD_LF_DEV_SWITCH_ON            0x00000004
/* LoadFirmware() is requesting the read only normal/dev code path */
#define VBSD_LF_USE_RO_NORMAL            0x00000008
/* Developer switch was enabled at boot time */
#define VBSD_BOOT_DEV_SWITCH_ON          0x00000010
/* Recovery switch was enabled at boot time */
#define VBSD_BOOT_REC_SWITCH_ON          0x00000020
/* Firmware write protect was enabled at boot time */
#define VBSD_BOOT_FIRMWARE_WP_ENABLED    0x00000040
/* Boot is a S3->S0 resume, not a S5->S0 normal boot */
#define VBSD_BOOT_S3_RESUME              0x00000100
/* Read-only firmware supports the normal/developer code path */
#define VBSD_BOOT_RO_NORMAL_SUPPORT      0x00000200
/* VbInit() was told that the system has a virtual dev-switch */
#define VBSD_HONOR_VIRT_DEV_SWITCH       0x00000400
/* VbInit() was told the system supports EC software sync */
#define VBSD_EC_SOFTWARE_SYNC            0x00000800
/* VbInit() was told that the EC firmware is slow to update */
#define VBSD_EC_SLOW_UPDATE              0x00001000
/* Firmware software write protect was enabled at boot time */
#define VBSD_BOOT_FIRMWARE_SW_WP_ENABLED 0x00002000
/* VbInit() was told that the recovery button is a virtual one */
#define VBSD_BOOT_REC_SWITCH_VIRTUAL     0x00004000
/* Firmware used vboot2 for firmware selection */
#define VBSD_BOOT_FIRMWARE_VBOOT2        0x00008000
/* Firmware needs VGA Option ROM to display screens */
#define VBSD_OPROM_MATTERS               0x00010000
/* Firmware has loaded the VGA Option ROM */
#define VBSD_OPROM_LOADED                0x00020000

/*
 * Supported flags by header version.  It's ok to add new flags while keeping
 * struct version 2 as long as flag-NOT-present is the correct value for
 * existing hardware (Stumpy/Lumpy).
 */
#define VBSD_FLAGS_VERSION_1            0x00000007  /* Alex, ZGB */
#define VBSD_FLAGS_VERSION_2            0x00000F7F

/* Result codes for VbSharedDataHeader.check_fw_a_result (and b_result) */
#define VBSD_LF_CHECK_NOT_DONE          0
#define VBSD_LF_CHECK_DEV_MISMATCH      1
#define VBSD_LF_CHECK_REC_MISMATCH      2
#define VBSD_LF_CHECK_VERIFY_KEYBLOCK   3
#define VBSD_LF_CHECK_KEY_ROLLBACK      4
#define VBSD_LF_CHECK_DATA_KEY_PARSE    5
#define VBSD_LF_CHECK_VERIFY_PREAMBLE   6
#define VBSD_LF_CHECK_FW_ROLLBACK       7
#define VBSD_LF_CHECK_HEADER_VALID      8
#define VBSD_LF_CHECK_GET_FW_BODY       9
#define VBSD_LF_CHECK_HASH_WRONG_SIZE   10
#define VBSD_LF_CHECK_VERIFY_BODY       11
#define VBSD_LF_CHECK_VALID             12
/*
 * Read-only normal path requested by firmware preamble, but unsupported by
 * firmware.
 */
#define VBSD_LF_CHECK_NO_RO_NORMAL      13

/* Boot mode for VbSharedDataHeader.lk_boot_mode */
#define VBSD_LK_BOOT_MODE_RECOVERY      0
#define VBSD_LK_BOOT_MODE_NORMAL        1
#define VBSD_LK_BOOT_MODE_DEVELOPER     2

/* Flags for VbSharedDataKernelPart.flags */
#define VBSD_LKP_FLAG_KEY_BLOCK_VALID   0x01

/* Result codes for VbSharedDataKernelPart.check_result */
#define VBSD_LKP_CHECK_NOT_DONE           0
#define VBSD_LKP_CHECK_TOO_SMALL          1
#define VBSD_LKP_CHECK_READ_START         2
#define VBSD_LKP_CHECK_KEY_BLOCK_SIG      3
#define VBSD_LKP_CHECK_KEY_BLOCK_HASH     4
#define VBSD_LKP_CHECK_DEV_MISMATCH       5
#define VBSD_LKP_CHECK_REC_MISMATCH       6
#define VBSD_LKP_CHECK_KEY_ROLLBACK       7
#define VBSD_LKP_CHECK_DATA_KEY_PARSE     8
#define VBSD_LKP_CHECK_VERIFY_PREAMBLE    9
#define VBSD_LKP_CHECK_KERNEL_ROLLBACK    10
#define VBSD_LKP_CHECK_PREAMBLE_VALID     11
/*
 * Body load address check is omitted; this result code is deprecated and not
 * used anywhere in the codebase.
 */
#define VBSD_LKP_CHECK_BODY_ADDRESS       12
#define VBSD_LKP_CHECK_BODY_OFFSET        13
#define VBSD_LKP_CHECK_SELF_SIGNED        14
#define VBSD_LKP_CHECK_BODY_EXCEEDS_MEM   15
#define VBSD_LKP_CHECK_BODY_EXCEEDS_PART  16
#define VBSD_LKP_CHECK_READ_DATA          17
#define VBSD_LKP_CHECK_VERIFY_DATA        18
#define VBSD_LKP_CHECK_KERNEL_GOOD        19

/* Information about a single kernel partition check in LoadKernel() */
typedef struct VbSharedDataKernelPart {
	uint64_t sector_start;     /* Start sector of partition */
	uint64_t sector_count;     /* Sector count of partition */
	uint32_t combined_version; /* Combined key+kernel version */
	uint8_t gpt_index;         /* Index of partition in GPT */
	uint8_t check_result;      /* Check result; see VBSD_LKP_CHECK_* */
	uint8_t flags;             /* Flags (see VBSD_LKP_FLAG_* */
	uint8_t reserved0;         /* Reserved for padding */
} VbSharedDataKernelPart;

/* Number of kernel partitions to track per call.  Must be power of 2. */
#define VBSD_MAX_KERNEL_PARTS 8

/* Flags for VbSharedDataKernelCall.flags */
/* Error initializing TPM in recovery mode */
#define VBSD_LK_FLAG_REC_TPM_INIT_ERROR 0x00000001

/* Result codes for VbSharedDataKernelCall.check_result */
#define VBSD_LKC_CHECK_NOT_DONE            0
#define VBSD_LKC_CHECK_DEV_SWITCH_MISMATCH 1
#define VBSD_LKC_CHECK_GPT_READ_ERROR      2
#define VBSD_LKC_CHECK_GPT_PARSE_ERROR     3
#define VBSD_LKC_CHECK_GOOD_PARTITION      4
#define VBSD_LKC_CHECK_INVALID_PARTITIONS  5
#define VBSD_LKC_CHECK_NO_PARTITIONS       6

/* Information about a single call to LoadKernel() */
typedef struct VbSharedDataKernelCall {
	/* Bottom 32 bits of flags passed in LoadKernelParams.boot_flags */
	uint32_t boot_flags;
	/* Debug flags; see VBSD_LK_FLAG_* */
	uint32_t flags;
	/* Number of sectors on drive */
	uint64_t sector_count;
	/* Sector size in bytes */
	uint32_t sector_size;
	/* Check result; see VBSD_LKC_CHECK_* */
	uint8_t check_result;
	/* Boot mode for LoadKernel(); see VBSD_LK_BOOT_MODE_* constants */
	uint8_t boot_mode;
	/* Test error number, if non-zero */
	uint8_t test_error_num;
	/* Return code from LoadKernel() */
	uint8_t return_code;
	/* Number of kernel partitions found */
	uint8_t kernel_parts_found;
	/* Reserved for padding */
	uint8_t reserved0[7];
	/* Data on kernels */
	VbSharedDataKernelPart parts[VBSD_MAX_KERNEL_PARTS];
} VbSharedDataKernelCall;

/* Number of kernel calls to track.  Must be power of 2. */
#define VBSD_MAX_KERNEL_CALLS 4

/*
 * Data shared between LoadFirmware(), LoadKernel(), and OS.
 *
 * The boot process is:
 *   1) Caller allocates buffer, at least VB_SHARED_DATA_MIN bytes, ideally
 *      VB_SHARED_DATA_REC_SIZE bytes.
 *   2) If non-recovery boot, this is passed to LoadFirmware(), which
 *      initializes the buffer, adding this header and some data.
 *   3) Buffer is passed to LoadKernel().  If this is a recovery boot,
 *      LoadKernel() initializes the buffer, adding this header.  Regardless
 *      of boot type, LoadKernel() adds some data to the buffer.
 *   4) Caller makes data available to the OS in a platform-dependent manner.
 *      For example, via ACPI or ATAGs.
 */
typedef struct VbSharedDataHeader {
	/* Fields present in version 1 */
	/* Magic number for struct (VB_SHARED_DATA_MAGIC) */
	uint32_t magic;
	/* Version of this structure */
	uint32_t struct_version;
	/* Size of this structure in bytes */
	uint64_t struct_size;
	/* Size of shared data buffer in bytes */
	uint64_t data_size;
	/* Amount of shared data used so far */
	uint64_t data_used;
	/* Flags */
	uint32_t flags;
	/* Reserved for padding */
	uint32_t reserved0;
	/* Kernel subkey, from firmware */
	VbPublicKey kernel_subkey;
	/* Offset of kernel subkey data from start of this struct */
	uint64_t kernel_subkey_data_offset;
	/* Size of kernel subkey data */
	uint64_t kernel_subkey_data_size;

	/*
	 * Timer values from VbExGetTimer().  Unused values are set to 0.  Note
	 * that these are now the enter/exit times for the wrapper API entry
	 * points; see crosbug.com/17018. */
	/* VbInit() enter/exit */
	uint64_t timer_vb_init_enter;
	uint64_t timer_vb_init_exit;
	/* VbSelectFirmware() enter/exit */
	uint64_t timer_vb_select_firmware_enter;
	uint64_t timer_vb_select_firmware_exit;
	/* VbSelectAndLoadKernel() enter/exit */
	uint64_t timer_vb_select_and_load_kernel_enter;
	uint64_t timer_vb_select_and_load_kernel_exit;

	/* Information stored in TPM, as retrieved by firmware */
	/* Current firmware version in TPM */
	uint32_t fw_version_tpm;
	/* Current kernel version in TPM */
	uint32_t kernel_version_tpm;

	/* Debugging information from LoadFirmware() */
	/* Result of checking RW firmware A and B */
	uint8_t check_fw_a_result;
	uint8_t check_fw_b_result;
	/* Firmware index returned by LoadFirmware() or 0xFF if failure */
	uint8_t firmware_index;
	/* Reserved for padding */
	uint8_t reserved1;
	/* Firmware TPM version at start of VbSelectFirmware() */
	uint32_t fw_version_tpm_start;
	/* Firmware lowest version found */
	uint32_t fw_version_lowest;

	/* Debugging information from LoadKernel() */
	/* Number of times LoadKernel() called */
	uint32_t lk_call_count;
	/* Info on calls */
	VbSharedDataKernelCall lk_calls[VBSD_MAX_KERNEL_CALLS];

	/*
	 * Offset and size of supplemental kernel data.  Reserve space for
	 * these fields now, so that future LoadKernel() versions can store
	 * information there without needing to shift down whatever data the
	 * original LoadFirmware() might have put immediately following its
	 * VbSharedDataHeader.
	 */
	uint64_t kernel_supplemental_offset;
	uint64_t kernel_supplemental_size;

	/*
	 * Fields added in version 2.  Before accessing, make sure that
	 * struct_version >= 2
	 */
	/* Recovery reason for current boot */
	uint8_t recovery_reason;
	/* Reserved for padding */
	uint8_t reserved2[7];
	/* Flags from firmware keyblock */
	uint64_t fw_keyblock_flags;
	/* Kernel TPM version at start of VbSelectAndLoadKernel() */
	uint32_t kernel_version_tpm_start;
	/* Kernel lowest version found */
	uint32_t kernel_version_lowest;

	/*
	 * After read-only firmware which uses version 2 is released, any
	 * additional fields must be added below, and the struct version must
	 * be increased.  Before reading/writing those fields, make sure that
	 * the struct being accessed is at least version 3.
	 *
	 * It's always ok for an older firmware to access a newer struct, since
	 * all the fields it knows about are present.  Newer firmware needs to
	 * use reasonable defaults when accessing older structs.
	 */
} __attribute__((packed)) VbSharedDataHeader;

/*
 * Size of VbSharedDataheader for each version
 *
 * TODO: crossystem needs not to fail if called on a v1 system where
 * sizeof(VbSharedDataHeader) was smaller
 */
#define VB_SHARED_DATA_HEADER_SIZE_V1 1072
#define VB_SHARED_DATA_HEADER_SIZE_V2 1096

#define VB_SHARED_DATA_VERSION 2      /* Version for struct_version */

#endif  /* VBOOT_REFERENCE_VBOOT_STRUCT_H_ */
