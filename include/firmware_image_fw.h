/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure and API definitions for a verified boot firmware image.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_IMAGE_FW_H_
#define VBOOT_REFERENCE_FIRMWARE_IMAGE_FW_H_

#include <stdint.h>
#include "rsa.h"
#include "sha.h"

#define FIRMWARE_MAGIC "CHROMEOS"
#define FIRMWARE_MAGIC_SIZE 8
#define FIRMWARE_PREAMBLE_SIZE 8

/* RSA 8192 and SHA-512. */
#define ROOT_SIGNATURE_ALGORITHM 11
#define ROOT_SIGNATURE_ALGORITHM_STRING "11"

typedef struct FirmwareImage {
  uint8_t magic[FIRMWARE_MAGIC_SIZE];
  /* Key Header */
  uint16_t header_len;  /* Length of the header. */
  uint16_t firmware_sign_algorithm;  /* Signature algorithm used by the signing
                                      * key. */
  uint16_t firmware_key_version;  /* Key Version# for preventing rollbacks. */
  uint8_t* firmware_sign_key;  /* Pre-processed public half of signing key. */
  uint8_t header_checksum[SHA512_DIGEST_SIZE];  /* SHA-512 hash of the header.*/

  uint8_t firmware_key_signature[RSA8192NUMBYTES];  /* Signature of the header
                                                     * above. */

  /* Firmware Preamble. */
  uint16_t firmware_version;  /* Firmware Version# for preventing rollbacks.*/
  uint64_t firmware_len;  /* Length of the rest of the R/W firmware data. */
  uint8_t preamble[FIRMWARE_PREAMBLE_SIZE];  /* Remaining preamble data.*/

  uint8_t* preamble_signature;  /* Signature over the preamble. */

  /* The firmware signature comes first as it may allow us to parallelize
   * the firmware data fetch and RSA public operation.
   */
  uint8_t* firmware_signature;  /* Signature on the Preamble +
                                   [firmware_data]. */
  uint8_t* firmware_data;  /* Rest of firmware data */

} FirmwareImage;


/* Error Codes for VerifyFirmware* family of functions. */
#define VERIFY_FIRMWARE_SUCCESS 0
#define VERIFY_FIRMWARE_INVALID_IMAGE 1
#define VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED 2
#define VERIFY_FIRMWARE_INVALID_ALGORITHM 3
#define VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED 4
#define VERIFY_FIRMWARE_SIGNATURE_FAILED 5
#define VERIFY_FIRMWARE_WRONG_MAGIC 6
#define VERIFY_FIRMWARE_WRONG_HEADER_CHECKSUM 7
#define VERIFY_FIRMWARE_KEY_ROLLBACK 8
#define VERIFY_FIRMWARE_VERSION_ROLLBACK 9
#define VERIFY_FIRMWARE_MAX 10  /* Total number of error codes. */

extern char* kVerifyFirmwareErrors[VERIFY_FIRMWARE_MAX];

/* Checks for the sanity of the firmware header pointed by [header_blob].
 *
 * On success, put signature algorithm in [algorithm], header length
 * in [header_len], and return 0.
 * Else, return error code on failure.
 */
int VerifyFirmwareHeader(const uint8_t* root_key_blob,
                         const uint8_t* header_blob,
                         int* algorithm,
                         int* header_len);

/* Checks the preamble signature on firmware preamble pointed by
 * [preamble_blob] using the signing key [sign_key].
 *
 * On success, put firmware length into [firmware_len], and return 0.
 * Else, return error code on failure.
 */
int VerifyFirmwarePreamble(RSAPublicKey* sign_key,
                           const uint8_t* preamble_blob,
                           int algorithm,
                           uint64_t* firmware_len);

/* Checks the signature on the preamble + firmware data at
 * [preamble_start] and [firmware_data_start].
 * The length of the actual firmware data is firmware_len and it is assumed to
 * be prepended with the signature whose size depends on the signature_algorithm
 * [algorithm]. This signature also covers the preamble data (but not the
 * preamble signature itself).
 *
 * Return 0 on success, error code on failure.
 */
int VerifyFirmwareData(RSAPublicKey* sign_key,
                       const uint8_t* preamble_start,
                       const uint8_t* firmware_data_start,
                       uint64_t firmware_len,
                       int algorithm);

/* Performs a chained verify of the firmware blob [firmware_blob].
 *
 * Returns 0 on success, error code on failure.
 *
 * NOTE: The length of the firmware blob is derived from reading the fields
 * in the first few bytes of the buffer. This might look risky but in firmware
 * land, the start address of the firmware_blob will always be fixed depending
 * on the memory map on the particular platform. In addition, the signature on
 * length itself is checked early in the verification process for extra safety.
 */
int VerifyFirmware(const uint8_t* root_key_blob,
                   const uint8_t* firmware_blob);

/* Returns the logical version of a firmware blob which is calculated as
 * (firmware_key_version << 16 | firmware_version). */
uint32_t GetLogicalFirmwareVersion(uint8_t* firmware_blob);

#define  BOOT_FIRMWARE_A_CONTINUE 1
#define  BOOT_FIRMWARE_B_CONTINUE 2
#define  BOOT_FIRMWARE_RECOVERY_CONTINUE 3

/* This function is the driver used by the RO firmware to
 * determine which copy of the firmware to boot from. It performs
 * the requisite rollback index checking, including updating them,
 * if required.
 *
 * Returns the code path to follow. It is one of:
 * BOOT_FIRMWARE_A_CONTINUE          Boot from Firmware A
 * BOOT_FIRMWARE_B_CONTINUE          Boot from Firmware B
 * BOOT_FIRMWARE_RECOVERY_CONTINUE   Jump to recovery mode
 */
int VerifyFirmwareDriver_f(uint8_t* root_key_blob,
                           uint8_t* firmwareA,
                           uint8_t* firmwareB);


#endif  /* VBOOT_REFERENCE_FIRMWARE_IMAGE_FW_H_ */
