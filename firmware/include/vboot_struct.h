/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure definitions for verified boot, for on-disk / in-eeprom
 * data.
 */

#ifndef VBOOT_REFERENCE_VBOOT_STRUCT_H_
#define VBOOT_REFERENCE_VBOOT_STRUCT_H_

#include "sysincludes.h"

__pragma(pack(push, 1)) /* Support packing for MSVC. */

/* Public key data */
typedef struct VbPublicKey {
  uint64_t key_offset;     /* Offset of key data from start of this struct */
  uint64_t key_size;       /* Size of key data in bytes (NOT strength of key
                           * in bits) */
  uint64_t algorithm;      /* Signature algorithm used by the key */
  uint64_t key_version;    /* Key version */
} __attribute__((packed)) VbPublicKey;

#define EXPECTED_VBPUBLICKEY_SIZE 32


/* Signature data (a secure hash, possibly signed) */
typedef struct VbSignature {
  uint64_t sig_offset;  /* Offset of signature data from start of this
                         * struct */
  uint64_t sig_size;    /* Size of signature data in bytes */
  uint64_t data_size;   /* Size of the data block which was signed in bytes */
} __attribute__((packed)) VbSignature;

#define EXPECTED_VBSIGNATURE_SIZE 24


#define KEY_BLOCK_MAGIC "CHROMEOS"
#define KEY_BLOCK_MAGIC_SIZE 8

#define KEY_BLOCK_HEADER_VERSION_MAJOR 2
#define KEY_BLOCK_HEADER_VERSION_MINOR 1

/* Flags for key_block_flags */
/* The following flags set where the key is valid */
#define KEY_BLOCK_FLAG_DEVELOPER_0  UINT64_C(0x01)  /* Developer switch off */
#define KEY_BLOCK_FLAG_DEVELOPER_1  UINT64_C(0x02)  /* Developer switch on */
#define KEY_BLOCK_FLAG_RECOVERY_0   UINT64_C(0x04)  /* Not recovery mode */
#define KEY_BLOCK_FLAG_RECOVERY_1   UINT64_C(0x08)  /* Recovery mode */

/* Key block, containing the public key used to sign some other chunk
 * of data. */
typedef struct VbKeyBlockHeader {
  uint8_t magic[KEY_BLOCK_MAGIC_SIZE];  /* Magic number */
  uint32_t header_version_major;     /* Version of this header format */
  uint32_t header_version_minor;     /* Version of this header format */
  uint64_t key_block_size;           /* Length of this entire key block,
                                      * including keys, signatures, and
                                      * padding, in bytes */
  VbSignature key_block_signature;   /* Signature for this key block
                                      * (header + data pointed to by data_key)
                                      * For use with signed data keys*/
  VbSignature key_block_checksum;    /* SHA-512 checksum for this key block
                                      * (header + data pointed to by data_key)
                                      * For use with unsigned data keys */
  uint64_t key_block_flags;          /* Flags for key (KEY_BLOCK_FLAG_*) */
  VbPublicKey data_key;              /* Key to verify the chunk of data */
} __attribute__((packed)) VbKeyBlockHeader;
/* This should be followed by:
 *   1) The data_key key data, pointed to by data_key.key_offset.
 *   2) The checksum data for (VBKeyBlockHeader + data_key data), pointed to
 *      by key_block_checksum.sig_offset.
 *   3) The signature data for (VBKeyBlockHeader + data_key data), pointed to
 *      by key_block_signature.sig_offset. */

#define EXPECTED_VBKEYBLOCKHEADER_SIZE 112


#define FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR 2
#define FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR 0

/* Preamble block for rewritable firmware */
typedef struct VbFirmwarePreambleHeader {
  uint64_t preamble_size;            /* Size of this preamble, including keys,
                                      * signatures, and padding, in bytes */
  VbSignature preamble_signature;    /* Signature for this preamble
                                      * (header + kernel subkey +
                                      * body signature) */
  uint32_t header_version_major;     /* Version of this header format */
  uint32_t header_version_minor;     /* Version of this header format */

  uint64_t firmware_version;         /* Firmware version */
  VbPublicKey kernel_subkey;         /* Key to verify kernel key block */
  VbSignature body_signature;        /* Signature for the firmware body */
} __attribute__((packed)) VbFirmwarePreambleHeader;
/* This should be followed by:
 *   1) The kernel_subkey key data, pointed to by kernel_subkey.key_offset.
 *   2) The signature data for the firmware body, pointed to by
 *      body_signature.sig_offset.
 *   3) The signature data for (VBFirmwarePreambleHeader + kernel_subkey data
 *      + body signature data), pointed to by
 *      preamble_signature.sig_offset. */

#define EXPECTED_VBFIRMWAREPREAMBLEHEADER_SIZE 104

#define KERNEL_PREAMBLE_HEADER_VERSION_MAJOR 2
#define KERNEL_PREAMBLE_HEADER_VERSION_MINOR 0

/* Preamble block for kernel */
typedef struct VbKernelPreambleHeader {
  uint64_t preamble_size;            /* Size of this preamble, including keys,
                                      * signatures, and padding, in bytes */
  VbSignature preamble_signature;    /* Signature for this preamble
                                      * (header + body signature) */
  uint32_t header_version_major;     /* Version of this header format */
  uint32_t header_version_minor;     /* Version of this header format */

  uint64_t kernel_version;           /* Kernel version */
  uint64_t body_load_address;        /* Load address for kernel body */
  uint64_t bootloader_address;       /* Address of bootloader, after body is
                                      * loaded at body_load_address */
  uint64_t bootloader_size;          /* Size of bootloader in bytes */
  VbSignature body_signature;        /* Signature for the kernel body */
} __attribute__((packed)) VbKernelPreambleHeader;
/* This should be followed by:
 *   2) The signature data for the kernel body, pointed to by
 *      body_signature.sig_offset.
 *   3) The signature data for (VBFirmwarePreambleHeader + body signature
 *      data), pointed to by preamble_signature.sig_offset. */

#define EXPECTED_VBKERNELPREAMBLEHEADER_SIZE 96

/* Minimum and recommended size of shared_data_blob in bytes. */
#define VB_SHARED_DATA_MIN_SIZE 3072
#define VB_SHARED_DATA_REC_SIZE 16384

/* Data shared between LoadFirmware(), LoadKernel(), and OS.
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
 *      For example, via ACPI or ATAGs. */
typedef struct VbSharedDataHeader {
  /* Fields present in version 1 */
  uint32_t struct_version;            /* Version of this structure */
  uint64_t struct_size;               /* Size of this structure in bytes */
  uint64_t data_size;                 /* Size of shared data buffer in bytes */
  uint64_t data_used;                 /* Amount of shared data used so far */

  VbPublicKey kernel_subkey;          /* Kernel subkey, from firmware */
  uint64_t kernel_subkey_data_offset; /* Offset of kernel subkey data from
                                       * start of this struct */
  uint64_t kernel_subkey_data_size;   /* Offset of kernel subkey data */

  uint64_t flags;                     /* Flags */

  /* After read-only firmware which uses version 1 is released, any additional
   * fields must be added below, and the struct version must be increased.
   * Before reading/writing those fields, make sure that the struct being
   * accessed is at least version 2.
   *
   * It's always ok for an older firmware to access a newer struct, since all
   * the fields it knows about are present.  Newer firmware needs to use
   * reasonable defaults when accessing older structs. */

} __attribute__((packed)) VbSharedDataHeader;

#define VB_SHARED_DATA_VERSION 1      /* Version for struct_version */

__pragma(pack(pop)) /* Support packing for MSVC. */

#endif  /* VBOOT_REFERENCE_VBOOT_STRUCT_H_ */
