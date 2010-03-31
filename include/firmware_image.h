/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * API definitions for a verified boot firmware image.
 * (Userland Portion)
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_IMAGE_H_
#define VBOOT_REFERENCE_FIRMWARE_IMAGE_H_

#include "firmware_image_fw.h"

/* Allocate and return a new FirmwareImage structure. */
FirmwareImage* FirmwareImageNew(void);

/* Deep free the contents of [fw]. */
void FirmwareImageFree(FirmwareImage* fw);

/* Read firmware data from file named [input_file].
 *
 * Returns a filled up FirmwareImage structure on success, NULL on error.
 */
FirmwareImage* ReadFirmwareImage(const char* input_file);

/* Get the length of the header for image [image]. */
int GetFirmwareHeaderLen(const FirmwareImage* image);

/* Calculate and store the firmware header checksum of [image]
 * in [header_checksum].
 *
 * [header_checksum] must be a valid pointer to a buffer of
 * SHA512_DIGEST_SIZE.
 */
void CalculateFirmwareHeaderChecksum(const FirmwareImage *image,
                                     uint8_t* header_checksum);

/* Get firmware header binary blob from an [image].
 *
 * Caller owns the returned pointer and must Free() it.
 */
uint8_t* GetFirmwareHeaderBlob(const FirmwareImage* image);

/* Get firmware preamble binary blob from an [image].
 *
 * Caller owns the returned pointer and must Free() it.
 */
uint8_t* GetFirmwarePreambleBlob(const FirmwareImage* image);

/* Get a verified firmware binary blob from an [image] and fill its
 * length into blob_len.
 *
 * Caller owns the returned pointer and must Free() it.
 */
uint8_t* GetFirmwareBlob(const FirmwareImage* image, uint64_t* blob_len);

/* Write firmware data from [image] into a file named [input_file].
 *
 * Return 1 on success, 0 on failure.
 */
int WriteFirmwareImage(const char* input_file,
                       const FirmwareImage* image);

/* Pretty print the contents of [image]. Only headers and metadata information
 * is printed.
 */
void PrintFirmwareImage(const FirmwareImage* image);

/* Performs a chained verify of the firmware [image].
 *
 * Returns 0 on success, error code on failure.
 */
int VerifyFirmwareImage(const RSAPublicKey* root_key,
                        const FirmwareImage* image);

/* Maps error codes from VerifyFirmware() to error description. */
const char* VerifyFirmwareErrorString(int error);

/* Add a root key signature to the key header to a firmware image [image]
 * using the private root key in file [root_key_file].
 *
 * Return 1 on success, 0 on failure.
 */
int AddFirmwareKeySignature(FirmwareImage* image, const char* root_key_file);

/* Add firmware and preamble signature to a firmware image [image]
 * using the private signing key in file [signing_key_file].
 *
 * Return 1 on success, 0 on failure.
 */
int AddFirmwareSignature(FirmwareImage* image, const char* signing_key_file);

#endif  /* VBOOT_REFERENCE_FIRMWARE_IMAGE_H_ */
