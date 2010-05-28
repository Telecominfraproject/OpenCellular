/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for verifying a verified boot kernel image.
 * (Firmware portion)
 */

#include "kernel_image_fw.h"

#include "cryptolib.h"
#include "rollback_index.h"
#include "stateful_util.h"
#include "utility.h"

/* Macro to determine the size of a field structure in the KernelImage
 * structure. */
#define FIELD_LEN(field) (sizeof(((KernelImage*)0)->field))

char* kVerifyKernelErrors[VERIFY_KERNEL_MAX] = {
  "Success.",
  "Invalid Image.",
  "Kernel Key Signature Failed.",
  "Invalid Kernel Verification Algorithm.",
  "Preamble Signature Failed.",
  "Kernel Signature Failed.",
  "Wrong Kernel Magic.",
};

inline uint64_t GetKernelPreambleLen(int algorithm) {
  return (FIELD_LEN(kernel_version) +
          FIELD_LEN(kernel_len) +
          FIELD_LEN(bootloader_offset) +
          FIELD_LEN(bootloader_size) +
          FIELD_LEN(padded_header_size) +
          siglen_map[algorithm]);
}

uint64_t GetVblockHeaderSize(const uint8_t* vkernel_blob) {
  uint64_t len = 0;
  uint16_t firmware_sign_algorithm;
  uint16_t kernel_sign_algorithm;
  int algorithms_offset = (FIELD_LEN(magic) +
                           FIELD_LEN(header_version) +
                           FIELD_LEN(header_len));
  if (SafeMemcmp(vkernel_blob, KERNEL_MAGIC, KERNEL_MAGIC_SIZE)) {
    debug("Not a valid verified boot kernel blob.\n");
    return 0;
  }
  Memcpy(&firmware_sign_algorithm,
         vkernel_blob + algorithms_offset,
         sizeof(firmware_sign_algorithm));
  Memcpy(&kernel_sign_algorithm,
         vkernel_blob + algorithms_offset + FIELD_LEN(kernel_sign_algorithm),
         sizeof(kernel_sign_algorithm));
  if (firmware_sign_algorithm >= kNumAlgorithms) {
    debug("Invalid firmware signing algorithm.\n");
    return 0;
  }
  if (kernel_sign_algorithm >= kNumAlgorithms) {
    debug("Invalid kernel signing algorithm.\n");
    return 0;
  }
  len = algorithms_offset;  /* magic, header length and version. */
  len += (FIELD_LEN(firmware_sign_algorithm) +
          FIELD_LEN(kernel_sign_algorithm) +
          FIELD_LEN(kernel_key_version) +
          RSAProcessedKeySize(kernel_sign_algorithm) +  /* kernel_sign_key */
          FIELD_LEN(header_checksum) +
          siglen_map[firmware_sign_algorithm] +  /* kernel_key_signature */
          GetKernelPreambleLen(kernel_sign_algorithm) +
          siglen_map[kernel_sign_algorithm]);  /* preamble_signature */
  return len;
}

int VerifyKernelKeyHeader(const uint8_t* firmware_key_blob,
                          const uint8_t* header_blob,
                          const int dev_mode,
                          int* firmware_algorithm,
                          int* kernel_algorithm,
                          int* kernel_header_len) {
  int kernel_sign_key_len;
  int firmware_sign_key_len;
  uint16_t header_version, header_len;
  uint16_t firmware_sign_algorithm, kernel_sign_algorithm;
  uint8_t* header_checksum = NULL;

  /* Base Offset for the header_checksum field. Actual offset is
   * this + kernel_sign_key_len. */
  int base_header_checksum_offset = (FIELD_LEN(header_version) +
                                     FIELD_LEN(header_len) +
                                     FIELD_LEN(firmware_sign_algorithm) +
                                     FIELD_LEN(kernel_sign_algorithm) +
                                     FIELD_LEN(kernel_key_version));

  Memcpy(&header_version, header_blob, sizeof(header_version));
  Memcpy(&header_len, header_blob + FIELD_LEN(header_version),
         sizeof(header_len));
  Memcpy(&firmware_sign_algorithm,
         header_blob + (FIELD_LEN(header_version) +
                        FIELD_LEN(header_len)),
         sizeof(firmware_sign_algorithm));
  Memcpy(&kernel_sign_algorithm,
         header_blob + (FIELD_LEN(header_version) +
                        FIELD_LEN(header_len) +
                        FIELD_LEN(firmware_sign_algorithm)),
         sizeof(kernel_sign_algorithm));

  /* TODO(gauravsh): Make this return two different error types depending
   * on whether the firmware or kernel signing algorithm is invalid. */
  if (firmware_sign_algorithm >= kNumAlgorithms)
    return VERIFY_KERNEL_INVALID_ALGORITHM;
  if (kernel_sign_algorithm >= kNumAlgorithms)
    return VERIFY_KERNEL_INVALID_ALGORITHM;

  *firmware_algorithm = (int) firmware_sign_algorithm;
  *kernel_algorithm = (int) kernel_sign_algorithm;
  kernel_sign_key_len = RSAProcessedKeySize(kernel_sign_algorithm);
  firmware_sign_key_len = RSAProcessedKeySize(firmware_sign_algorithm);


  /* Verify if header len is correct? */
  if (header_len != (base_header_checksum_offset +
                     kernel_sign_key_len +
                     FIELD_LEN(header_checksum))) {
    debug("VerifyKernelKeyHeader: Header length mismatch\n");
    return VERIFY_KERNEL_INVALID_IMAGE;
  }
  *kernel_header_len = (int) header_len;

  /* Verify if the hash of the header is correct. */
  header_checksum = DigestBuf(header_blob,
                              header_len - FIELD_LEN(header_checksum),
                              SHA512_DIGEST_ALGORITHM);
  if (SafeMemcmp(header_checksum,
                 header_blob + (base_header_checksum_offset +
                                kernel_sign_key_len),
                 FIELD_LEN(header_checksum))) {
    Free(header_checksum);
    debug("VerifyKernelKeyHeader: Invalid header hash\n");
    return VERIFY_KERNEL_INVALID_IMAGE;
  }
  Free(header_checksum);

  /* Verify kernel key signature unless we are in dev mode. */
  if (!dev_mode) {
    if (!RSAVerifyBinary_f(firmware_key_blob, NULL,  /* Key to use */
                           header_blob,  /* Data to verify */
                           header_len, /* Length of data */
                           header_blob + header_len,  /* Expected Signature */
                           firmware_sign_algorithm))
      return VERIFY_KERNEL_KEY_SIGNATURE_FAILED;
  }
  return 0;
}

int VerifyKernelPreamble(RSAPublicKey* kernel_sign_key,
                         const uint8_t* preamble_blob,
                         int algorithm,
                         uint64_t* kernel_len) {
  int preamble_len = GetKernelPreambleLen(algorithm);
  if (!RSAVerifyBinary_f(NULL, kernel_sign_key, /* Key to use */
                         preamble_blob,  /* Data to verify */
                         preamble_len, /* Length of data */
                         preamble_blob + preamble_len,  /* Expected Signature */
                         algorithm))
    return VERIFY_KERNEL_PREAMBLE_SIGNATURE_FAILED;
  Memcpy(kernel_len,
         preamble_blob + FIELD_LEN(kernel_version),
         FIELD_LEN(kernel_len));
  return 0;
}

int VerifyKernelData(RSAPublicKey* kernel_sign_key,
                     const uint8_t* kernel_signature,
                     const uint8_t* kernel_data,
                     uint64_t kernel_len,
                     int algorithm) {

  if (!RSAVerifyBinary_f(NULL, kernel_sign_key, /* Key to use */
                         kernel_data,  /* Data to verify */
                         kernel_len, /* Length of data */
                         kernel_signature,  /* Expected Signature */
                         algorithm))
    return VERIFY_KERNEL_SIGNATURE_FAILED;
  return 0;
}

int VerifyKernelHeader(const uint8_t* firmware_key_blob,
                       const uint8_t* kernel_header_blob,
                       uint64_t kernel_header_blob_len,
                       const int dev_mode,
                       KernelImage* image,
                       RSAPublicKey** kernel_sign_key) {
  int error_code;
  int firmware_sign_algorithm;  /* Firmware signing key algorithm. */
  int kernel_sign_algorithm;  /* Kernel signing key algorithm. */
  int kernel_sign_key_len, kernel_key_signature_len, kernel_signature_len,
      header_len;
  uint64_t kernel_len;
  const uint8_t* header_ptr = NULL;  /* Pointer to key header. */
  const uint8_t* preamble_ptr = NULL;  /* Pointer to start of preamble. */
  MemcpyState st;

  /* Note: All the offset calculations are based on struct KernelImage which
   * is defined in include/kernel_image_fw.h. */
  st.remaining_buf = (void *)kernel_header_blob;
  st.remaining_len = kernel_header_blob_len;
  st.overrun = 0;

  /* Clear destination image struct */
  Memset(image, 0, sizeof(KernelImage));

  /* Read and compare magic bytes. */
  StatefulMemcpy(&st, &image->magic, KERNEL_MAGIC_SIZE);
  if (SafeMemcmp(image->magic, KERNEL_MAGIC, KERNEL_MAGIC_SIZE)) {
    return VERIFY_KERNEL_WRONG_MAGIC;
  }
  StatefulMemcpy(&st, &image->header_version, FIELD_LEN(header_version));
  StatefulMemcpy(&st, &image->header_len, FIELD_LEN(header_len));
  StatefulMemcpy(&st, &image->firmware_sign_algorithm,
                 FIELD_LEN(firmware_sign_algorithm));
  StatefulMemcpy(&st, &image->kernel_sign_algorithm,
                 FIELD_LEN(kernel_sign_algorithm));

  header_ptr = kernel_header_blob + KERNEL_MAGIC_SIZE;

  /* Only continue if header verification succeeds. */
  if ((error_code = VerifyKernelKeyHeader(firmware_key_blob, header_ptr,
                                          dev_mode,
                                          &firmware_sign_algorithm,
                                          &kernel_sign_algorithm,
                                          &header_len))) {
    debug("VerifyKernelHeader: Kernel Key Header verification failed.\n");
    return error_code;  /* AKA jump to recovery. */
  }

  /* Read pre-processed public half of the kernel signing key. */
  kernel_sign_key_len = RSAProcessedKeySize(kernel_sign_algorithm);
  StatefulMemcpy(&st, &image->kernel_key_version,
                 FIELD_LEN(kernel_key_version));
  image->kernel_sign_key = (uint8_t*)st.remaining_buf;
  StatefulSkip(&st, kernel_sign_key_len);
  StatefulMemcpy(&st, image->header_checksum, FIELD_LEN(header_checksum));

  /* Parse signing key into RSAPublicKey structure since it is
   * required multiple times. */
  *kernel_sign_key = RSAPublicKeyFromBuf(image->kernel_sign_key,
                                        kernel_sign_key_len);
  kernel_signature_len = siglen_map[kernel_sign_algorithm];
  kernel_key_signature_len = siglen_map[firmware_sign_algorithm];
  image->kernel_key_signature = (uint8_t*)st.remaining_buf;
  StatefulSkip(&st, kernel_signature_len);

  /* Only continue if preamble verification succeeds. */
  /* TODO: should pass the remaining len into VerifyKernelPreamble() */
  preamble_ptr = (const uint8_t*)st.remaining_buf;
  if ((error_code = VerifyKernelPreamble(*kernel_sign_key, preamble_ptr,
                                         kernel_sign_algorithm,
                                         &kernel_len))) {
    RSAPublicKeyFree(*kernel_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }

  /* Copy preamble fields */
  StatefulMemcpy(&st, &image->kernel_version, FIELD_LEN(kernel_version));
  StatefulMemcpy(&st, &image->kernel_len, FIELD_LEN(kernel_len));
  StatefulMemcpy(&st, &image->bootloader_offset, FIELD_LEN(bootloader_offset));
  StatefulMemcpy(&st, &image->bootloader_size, FIELD_LEN(bootloader_size));
  StatefulMemcpy(&st, &image->padded_header_size,
                 FIELD_LEN(padded_header_size));
  image->kernel_signature = (uint8_t*)st.remaining_buf;
  StatefulSkip(&st, kernel_signature_len);
  image->preamble_signature = (uint8_t*)st.remaining_buf;

  return 0;
}

int VerifyKernel(const uint8_t* firmware_key_blob,
                 const uint8_t* kernel_blob,
                 const int dev_mode) {
  int error_code;
  int firmware_sign_algorithm;  /* Firmware signing key algorithm. */
  int kernel_sign_algorithm;  /* Kernel Signing key algorithm. */
  RSAPublicKey* kernel_sign_key;
  int kernel_sign_key_len, kernel_key_signature_len, kernel_signature_len,
      header_len;
  uint64_t kernel_len;
  const uint8_t* header_ptr;  /* Pointer to header. */
  const uint8_t* kernel_sign_key_ptr;  /* Pointer to signing key. */
  const uint8_t* preamble_ptr;  /* Pointer to kernel preamble block. */
  const uint8_t* kernel_ptr;  /* Pointer to kernel signature/data. */
  const uint8_t* kernel_signature;

  /* Note: All the offset calculations are based on struct FirmwareImage which
   * is defined in include/firmware_image.h. */

  /* Compare magic bytes. */
  if (SafeMemcmp(kernel_blob, KERNEL_MAGIC, KERNEL_MAGIC_SIZE)) {
    debug("VerifyKernel: Kernel magic bytes not found.\n");
    return VERIFY_KERNEL_WRONG_MAGIC;
  }
  header_ptr = kernel_blob + KERNEL_MAGIC_SIZE;

  /* Only continue if header verification succeeds. */
  if ((error_code = VerifyKernelKeyHeader(firmware_key_blob, header_ptr, dev_mode,
                                          &firmware_sign_algorithm,
                                          &kernel_sign_algorithm, &header_len))) {
    debug("VerifyKernel: Kernel header verification failed.\n");
    return error_code;  /* AKA jump to recovery. */
  }
  /* Parse signing key into RSAPublicKey structure since it is required multiple
   * times. */
  kernel_sign_key_len = RSAProcessedKeySize(kernel_sign_algorithm);
  kernel_sign_key_ptr = header_ptr + (FIELD_LEN(header_version) +
                                      FIELD_LEN(header_len) +
                                      FIELD_LEN(firmware_sign_algorithm) +
                                      FIELD_LEN(kernel_sign_algorithm) +
                                      FIELD_LEN(kernel_key_version));
  kernel_sign_key = RSAPublicKeyFromBuf(kernel_sign_key_ptr,
                                        kernel_sign_key_len);
  kernel_signature_len = siglen_map[kernel_sign_algorithm];
  kernel_key_signature_len = siglen_map[firmware_sign_algorithm];

  /* Only continue if preamble verification succeeds. */
  preamble_ptr = (header_ptr + header_len + kernel_key_signature_len);
  if ((error_code = VerifyKernelPreamble(kernel_sign_key, preamble_ptr,
                                         kernel_sign_algorithm,
                                         &kernel_len))) {
    debug("VerifyKernel: Kernel preamble verification failed.\n");
    RSAPublicKeyFree(kernel_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }
  /* Only continue if kernel data verification succeeds. */
  kernel_ptr = (preamble_ptr +
                GetKernelPreambleLen(kernel_sign_algorithm) +
                kernel_signature_len);  /* preamble signature. */
  kernel_signature = kernel_ptr - 2 * kernel_signature_len; /* end of kernel
                                                             * preamble. */

  if ((error_code = VerifyKernelData(kernel_sign_key,  /* Verification key */
                                     kernel_signature,  /* kernel signature */
                                     kernel_ptr,  /* Start of kernel data */
                                     kernel_len,  /* Length of kernel data. */
                                     kernel_sign_algorithm))) {
    RSAPublicKeyFree(kernel_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }
  RSAPublicKeyFree(kernel_sign_key);
  return 0;  /* Success! */
}

uint32_t GetLogicalKernelVersion(uint8_t* kernel_blob) {
  uint8_t* kernel_ptr;
  uint16_t kernel_key_version;
  uint16_t kernel_version;
  uint16_t firmware_sign_algorithm;
  uint16_t kernel_sign_algorithm;
  int kernel_key_signature_len;
  int kernel_sign_key_len;
  kernel_ptr = kernel_blob + (FIELD_LEN(magic) +
                              FIELD_LEN(header_version) +
                              FIELD_LEN(header_len));
  Memcpy(&firmware_sign_algorithm, kernel_ptr, sizeof(firmware_sign_algorithm));
  kernel_ptr += FIELD_LEN(firmware_sign_algorithm);
  Memcpy(&kernel_sign_algorithm, kernel_ptr, sizeof(kernel_sign_algorithm));
  kernel_ptr += FIELD_LEN(kernel_sign_algorithm);
  Memcpy(&kernel_key_version, kernel_ptr, sizeof(kernel_key_version));

  if (firmware_sign_algorithm >= kNumAlgorithms)
    return 0;
  if (kernel_sign_algorithm >= kNumAlgorithms)
    return 0;

  kernel_key_signature_len = siglen_map[firmware_sign_algorithm];
  kernel_sign_key_len = RSAProcessedKeySize(kernel_sign_algorithm);
  kernel_ptr += (FIELD_LEN(kernel_key_version) +
                 kernel_sign_key_len +
                 FIELD_LEN(header_checksum) +
                 kernel_key_signature_len);
  Memcpy(&kernel_version, kernel_ptr, sizeof(kernel_version));
  return CombineUint16Pair(kernel_key_version, kernel_version);
}

int VerifyKernelDriver_f(uint8_t* firmware_key_blob,
                         kernel_entry* kernelA,
                         kernel_entry* kernelB,
                         int dev_mode) {
  int i;
  /* Contains the logical kernel version (32-bit) which is calculated as
   * (kernel_key_version << 16 | kernel_version) where
   * [kernel_key_version], [firmware_version] are both 16-bit.
   */
  uint32_t kernelA_lversion, kernelB_lversion;
  uint32_t min_lversion;  /* Minimum of kernel A and kernel B lversion. */
  uint32_t stored_lversion;  /* Stored logical version in the TPM. */
  kernel_entry* try_kernel[2];  /* Kernel in try order. */
  int try_kernel_which[2];  /* Which corresponding kernel in the try order */
  uint32_t try_kernel_lversion[2];  /* Their logical versions. */

  /* [kernel_to_boot] will eventually contain the boot path to follow
   * and is returned to the caller. Initially, we set it to recovery. If
   * a valid bootable kernel is found, it will be set to that. */
  int kernel_to_boot = BOOT_KERNEL_RECOVERY_CONTINUE;


  /* The TPM must already have be initialized, so no need to call SetupTPM(). */

  /* We get the key versions by reading directly from the image blobs without
   * any additional (expensive) sanity checking on the blob since it's faster to
   * outright reject a kernel with an older kernel key version. A malformed
   * or corrupted kernel blob will still fail when VerifyKernel() is called
   * on it.
   */
  kernelA_lversion = GetLogicalKernelVersion(kernelA->kernel_blob);
  kernelB_lversion = GetLogicalKernelVersion(kernelB->kernel_blob);
  min_lversion  = Min(kernelA_lversion, kernelB_lversion);
  stored_lversion = CombineUint16Pair(GetStoredVersion(KERNEL_KEY_VERSION),
                                      GetStoredVersion(KERNEL_VERSION));

  /* TODO(gauravsh): The kernel entries kernelA and kernelB come from the
   * partition table - verify its signature/checksum before proceeding
   * further. */

  /* The logic for deciding which kernel to boot from is taken from the
   * the Chromium OS Drive Map design document.
   *
   * We went to consider the kernels in their according to their boot
   * priority attribute value.
   */

  if (kernelA->boot_priority >= kernelB->boot_priority) {
    try_kernel[0] = kernelA;
    try_kernel_which[0] = BOOT_KERNEL_A_CONTINUE;
    try_kernel_lversion[0] = kernelA_lversion;
    try_kernel[1] = kernelB;
    try_kernel_which[1] = BOOT_KERNEL_B_CONTINUE;
    try_kernel_lversion[1] = kernelB_lversion;
  } else {
    try_kernel[0] = kernelB;
    try_kernel_which[0] = BOOT_KERNEL_B_CONTINUE;
    try_kernel_lversion[0] = kernelB_lversion;
    try_kernel[1] = kernelA;
    try_kernel_which[1] = BOOT_KERNEL_A_CONTINUE;
    try_kernel_lversion[1] = kernelA_lversion;
  }

  /* TODO(gauravsh): Changes to boot_tries_remaining and boot_priority
   * below should be propagated to partition table. This will be added
   * once the firmware parition table parsing code is in. */
  for (i = 0; i < 2; i++) {
    if ((try_kernel[i]->boot_success_flag ||
         try_kernel[i]->boot_tries_remaining) &&
        (VERIFY_KERNEL_SUCCESS == VerifyKernel(firmware_key_blob,
                                               try_kernel[i]->kernel_blob,
                                               dev_mode))) {
      if (try_kernel[i]->boot_tries_remaining > 0)
        try_kernel[i]->boot_tries_remaining--;
      if (stored_lversion > try_kernel_lversion[i])
        continue;  /* Rollback: I am afraid I can't let you do that Dave. */
      if (i == 0 && (stored_lversion < try_kernel_lversion[1])) {
        /* The higher priority kernel is valid and bootable, See if we
         * need to update the stored version for rollback prevention. */
        if (VERIFY_KERNEL_SUCCESS == VerifyKernel(firmware_key_blob,
                                                  try_kernel[1]->kernel_blob,
                                                  dev_mode)) {
          WriteStoredVersion(KERNEL_KEY_VERSION,
                             (uint16_t) (min_lversion >> 16));
          WriteStoredVersion(KERNEL_VERSION,
                             (uint16_t) (min_lversion & 0xFFFF));
          stored_lversion = min_lversion;  /* Update stored version as it's
                                            * used later. */
        }
      }
      kernel_to_boot = try_kernel_which[i];
      break;  /* We found a valid kernel. */
    }
    try_kernel[i]->boot_priority = 0;
    }  /* for loop. */

  /* Lock Kernel TPM rollback indices from further writes.  In this design,
   * this is tied to locking physical presence---so (software) physical
   * presence cannot be asserted after this point.  This is a big side effect,
   * so we want to make it clear in the function name.
   * TODO(gauravsh): figure out better abstractions.
   */
  LockKernelVersionsByLockingPP();
  return kernel_to_boot;
}
