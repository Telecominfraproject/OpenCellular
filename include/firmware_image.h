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

#define ROOT_SIGNATURE_ALGORITHM 11 /* RSA 8192 and SHA-512. */
#define ROOT_SIGNATURE_ALGORITHM_STRING "11"

typedef struct FirmwareImage {
  uint8_t magic[FIRMWARE_MAGIC_SIZE];
  /* Key Header */
  uint16_t header_len;  /* Length of the header. */
  uint16_t sign_algorithm;  /* Signature algorithm used by the signing key. */
  uint8_t* sign_key;  /* Pre-processed public half of signing key. */
  uint16_t key_version;  /* Key Version# for preventing rollbacks. */
  uint8_t header_hash[SHA512_DIGEST_SIZE];  /* SHA-512 hash of the header.*/

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
FirmwareImage* ReadFirmware(const char* input_file,
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
FirmwareImage* WriteFirmware(const char* input_file,
                             FirmwareImage* image);

/* Pretty print the contents of [image]. Only headers and metadata information
 * is printed.
 */
void PrintFirmware(const FirmwareImage* image);

/* Performs a chained verify of the firmware [image]. If [dev_mode] is
 * 0 (inactive), then the [root_key] is used to verify the signature of
 * the signing key, else the check is skipped.
 *
 * Returns 0 on success, error code on failure.
 */
int VerifyFirmware(const RSAPublicKey* root_key,
                   const FirmwareImage* image,
                   const int dev_mode);

/* Error Codes for VerifyFirmware. */
#define VERIFY_SUCCESS 0
#define VERIFY_INVALID_IMAGE 1
#define VERIFY_ROOT_SIGNATURE_FAILED 2
#define VERIFY_INVALID_ALGORITHM 3
#define VERIFY_PREAMBLE_SIGNATURE_FAILED 4
#define VERIFY_FIRMWARE_SIGNATURE_FAILED 5
#define VERIFY_MAX 6  /* Generic catch-all. */

char* kVerifyFirmwareErrors[VERIFY_MAX];

/* Maps error codes from VerifyFirmware() to error description. */
char* VerifyErrorString(int error);


/* Helper function to invoke external program to calculate signature on
 * [input_file] using private key [key_file] and signature algorithm
 * [algorithm].
 *
 * Returns the signature. Caller owns the buffer and must Free() it.
 */
uint8_t* SignatureFile(char* input_fie, char* key_file, int algorithm);

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
