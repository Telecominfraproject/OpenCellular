/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * API definitions for a generating and manipulating verified boot kernel images.
 * (Userland portion.)
 */

#ifndef VBOOT_REFERENCE_KERNEL_IMAGE_H_
#define VBOOT_REFERENCE_KERNEL_IMAGE_H_

#include "kernel_image_fw.h"

/* Allocate and return a new KernelImage structure. */
KernelImage* KernelImageNew(void);

/* Deep free the contents of [image]. */
void KernelImageFree(KernelImage* image);

/* Read kernel data from file named [input_file].
 *
 * Returns a filled up KernelImage on success, NULL on error.
 */
KernelImage* ReadKernelImage(const char* input_file);

/* Get the length of the header for kernel image [image]. */
int GetKernelHeaderLen(const KernelImage* image);

/* Calculate and store the kernel header checksum of [image]
 * in [header_checksum].
 *
 * [header_checksum] must be a valid pointer to a buffer of
 * SHA512_DIGEST_SIZE.
 */
void CalculateKernelHeaderChecksum(const KernelImage* image,
                                   uint8_t* header_checksum);

/* Get kernel header binary blob from an [image].
 *
 * Caller owns the returned pointer and must Free() it.
 */
uint8_t* GetKernelHeaderBlob(const KernelImage* image);

/* Get kernel config binary blob from an [image].
 *
 * Caller owns the returned pointer and must Free() it.
 */
uint8_t* GetKernelConfigBlob(const KernelImage* image);

/* Get a verified kernel binary blob from an [image] and fill
 * its length into blob_len.
 *
 * Caller owns the returned pointer and must Free() it.
 */
uint8_t* GetKernelBlob(const KernelImage* image, uint64_t* blob_len);

/* Write kernel data from [image] to a file named [input_file].
 * If [is_only_vblock] is non-zero, only the verification block is output.
 *
 * Return 1 on success, 0 on error.
 */
int WriteKernelImage(const char* input_file,
                     const KernelImage* image,
                     int is_only_vblock);

/* Pretty print the contents of [image]. Only headers and metadata information
 * is printed.
 */
void PrintKernelImage(const KernelImage* image);

/* Performs a chained verify of the kernel [image]. If [dev_mode] is
 * 0 (inactive), then the [firmware_signing_key] is used to verify the signature
 * of the signing key, else the check is skipped.
 *
 * Returns 0 on success, error code on failure.
 */
int VerifyKernelImage(const RSAPublicKey* firmware_signing_key,
                      const KernelImage* image,
                      int dev_mode);


/* Maps error codes from VerifyKernel*() to error description. */
const char* VerifyKernelErrorString(int error);

/* Add a kernel signing key signature to the key header to a kernel image
 * [image] using the private key in file [firmware_key_file].
 *
 * Return 1 on success, 0 on failure.
 */
int AddKernelKeySignature(KernelImage* image, const char* firmware_key_file);

/* Add a kernel and kernel config signature to a kernel image [image]
 * using the private signing key in file [kernel_sigining_key_file].
 *
 * Return 1 on success, 0 on failure.
 */
int AddKernelSignature(KernelImage* image,
                       const char* kernel_sigining_key_file);

void PrintKernelEntry(kernel_entry* entry);

#endif  /* VBOOT_REFERENCE_KERNEL_IMAGE_H_ */
