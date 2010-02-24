/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure and API definitions for a verified boot firmware image.
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_IMAGE_H_
#define VBOOT_REFERENCE_FIRMWARE_IMAGE_H_

#include <inttypes.h>

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
  uint16_t sign_algorithm;  /* Signature algorithm used by the signing key. */
  uint8_t* sign_key;  /* Pre-processed public half of signing key. */
  uint16_t key_version;  /* Key Version# for preventing rollbacks. */
  uint8_t header_checksum[SHA512_DIGEST_SIZE];  /* SHA-512 hash of the header.*/

  uint8_t key_signature[RSA8192NUMBYTES];   /* Signature of the header above. */

  /* Firmware Preamble. */
  uint16_t firmware_version;  /* Firmware Version# for preventing rollbacks.*/
  uint32_t firmware_len;  /* Length of the rest of the R/W firmware data. */
  uint8_t preamble[FIRMWARE_PREAMBLE_SIZE];  /* Remaining preamble data.*/

  uint8_t* preamble_signature;  /* Signature over the preamble. */

  /* The firmware signature comes first as it may allow us to parallelize
   * the firmware data fetch and RSA public operation.
   */
  uint8_t* firmware_signature;  /* Signature on [firmware_data]. */
  uint8_t* firmware_data;  /* Rest of firmware data */

} FirmwareImage;

/* Allocate and return a new FirmwareImage structure. */
FirmwareImage* FirmwareImageNew(void);

/* Deep free the contents of [fw]. */
void FirmwareImageFree(FirmwareImage* fw);

/* Read firmware data from file named [input_file] into [image].
 *
 * Returns a filled up FirmwareImage on success, NULL on error.
 */
FirmwareImage* ReadFirmwareImage(const char* input_file,
                                 FirmwareImage* image);

/* Write firmware header from [image] to an open file pointed by the
 * file descriptor [fd].
 */
void WriteFirmwareHeader(int fd, FirmwareImage* image);

/* Write firmware preamble from [image] to an open file pointed by the
 * file descriptor [fd].
 */
void WriteFirmwarePreamble(int fd, FirmwareImage* image);


/* Write firmware data from [image] into a file named [input_file].
 *
 * Return [image] on success, NULL on error.
 */
FirmwareImage* WriteFirmwareImage(const char* input_file,
                                  FirmwareImage* image);

/* Pretty print the contents of [image]. Only headers and metadata information
 * is printed.
 */
void PrintFirmwareImage(const FirmwareImage* image);

/* Error Codes for VerifyFirmware* family of functions. */
#define VERIFY_FIRMWARE_SUCCESS 0
#define VERIFY_FIRMWARE_INVALID_IMAGE 1
#define VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED 2
#define VERIFY_FIRMWARE_INVALID_ALGORITHM 3
#define VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED 4
#define VERIFY_FIRMWARE_SIGNATURE_FAILED 5
#define VERIFY_FIRMWARE_WRONG_MAGIC 6
#define VERIFY_FIRMWARE_MAX 7  /* Generic catch-all. */

char* kVerifyFirmwareErrors[VERIFY_FIRMWARE_MAX];

/* Checks for the sanity of the firmware header pointed by [header_blob].
 * If [dev_mode] is enabled, also checks the root key signature using the
 * pre-processed public root key [root_key_blob].
 *
 * On success, put signature algorithm in [algorithm], header length
 * in [header_len], and return 0.
 * Else, return error code on failure.
 */
int VerifyFirmwareHeader(const uint8_t* root_key_blob,
                         const uint8_t* header_blob,
                         const int dev_mode,
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
                           int* firmware_len);

/* Checks the signature on the firmware data at location [firmware_data_start].
 * The length of the actual firmware data is firmware_len and it is assumed to
 * be prepended with the signature whose size depends on the signature_algorithm
 * [algorithm].
 *
 * Return 0 on success, error code on failure.
 */
int VerifyFirmwareData(RSAPublicKey* sign_key,
                       const uint8_t* firmware_data_start,
                       int firmware_len,
                       int algorithm);

/* Performs a chained verify of the firmware blob [firmware_blob]. If
 * [dev_mode] is 0 [inactive], then the pre-processed public root key
 * [root_key_blob] is used the verify the signature of the signing key,
 * else the check is skipped.
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
                   const uint8_t* firmware_blob,
                   const int dev_mode);

/* Performs a chained verify of the firmware [image]. If [dev_mode] is
 * 0 (inactive), then the [root_key] is used to verify the signature of
 * the signing key, else the check is skipped.
 *
 * Returns 0 on success, error code on failure.
 */
int VerifyFirmwareImage(const RSAPublicKey* root_key,
                        const FirmwareImage* image,
                        const int dev_mode);

/* Maps error codes from VerifyFirmware() to error description. */
char* VerifyErrorString(int error);

/* Add a root key signature to the key header to a firmware image [image]
 * using the private root key in file [root_key_file].
 *
 * Return 1 on success, 0 on failure.
 */
int AddKeySignature(FirmwareImage* image, char* root_key_file);

/* Add firmware and preamble signature to a firmware image [image]
 * using the private signing key in file [signing_key_file].
 *
 * Return 1 on success, 0 on failure.
 */
int AddFirmwareSignature(FirmwareImage* image, char* signing_key_file,
                         int algorithm);

#endif  /* VBOOT_REFERENCE_FIRMWARE_IMAGE_H_ */
