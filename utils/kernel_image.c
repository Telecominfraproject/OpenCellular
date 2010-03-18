/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for generating and manipulating a verified boot kernel image.
 */

#include "kernel_image.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_keys.h"
#include "padding.h"
#include "rsa_utility.h"
#include "sha_utility.h"
#include "signature_digest.h"
#include "utility.h"

/* Macro to determine the size of a field structure in the KernelImage
 * structure. */
#define FIELD_LEN(field) (sizeof(((KernelImage*)0)->field))

KernelImage* KernelImageNew(void) {
  KernelImage* image = (KernelImage*) Malloc(sizeof(KernelImage));
  if (image) {
    image->kernel_sign_key = NULL;
    image->kernel_key_signature = NULL;
    image->config_signature = NULL;
    image->kernel_signature = NULL;
    image->kernel_data = NULL;
  }
  return image;
}

void KernelImageFree(KernelImage* image) {
  if (image) {
    Free(image->kernel_sign_key);
    Free(image->kernel_key_signature);
    Free(image->config_signature);
    Free(image->kernel_signature);
    Free(image->kernel_data);
    Free(image);
  }
}

KernelImage* ReadKernelImage(const char* input_file) {
  uint64_t file_size;
  int image_len = 0;  /* Total size of the kernel image. */
  int header_len = 0;
  int firmware_sign_key_len;
  int kernel_key_signature_len;
  int kernel_sign_key_len;
  int kernel_signature_len;
  uint8_t* kernel_buf;
  MemcpyState st;
  KernelImage* image = KernelImageNew();

  if (!image)
    return NULL;

  kernel_buf = BufferFromFile(input_file, &file_size);
  image_len = file_size;

  st.remaining_len = image_len;
  st.remaining_buf = kernel_buf;

  /* Read and compare magic bytes. */
  StatefulMemcpy(&st, &image->magic, KERNEL_MAGIC_SIZE);

  if (SafeMemcmp(image->magic, KERNEL_MAGIC, KERNEL_MAGIC_SIZE)) {
    fprintf(stderr, "Wrong Kernel Magic.\n");
    Free(kernel_buf);
    return NULL;
  }
  StatefulMemcpy(&st, &image->header_version, FIELD_LEN(header_version));
  StatefulMemcpy(&st, &image->header_len, FIELD_LEN(header_len));
  StatefulMemcpy(&st, &image->firmware_sign_algorithm,
                 FIELD_LEN(firmware_sign_algorithm));
  StatefulMemcpy(&st, &image->kernel_sign_algorithm,
                 FIELD_LEN(kernel_sign_algorithm));

  /* Valid Kernel Key signing algorithm. */
  if (image->firmware_sign_algorithm >= kNumAlgorithms) {
    Free(kernel_buf);
    return NULL;
  }

  /* Valid Kernel Signing Algorithm? */
  if (image->kernel_sign_algorithm >= kNumAlgorithms) {
    Free(kernel_buf);
    return NULL;
  }

  /* Compute size of pre-processed RSA public keys and signatures. */
  firmware_sign_key_len = RSAProcessedKeySize(image->firmware_sign_algorithm);
  kernel_key_signature_len  = siglen_map[image->firmware_sign_algorithm];
  kernel_sign_key_len = RSAProcessedKeySize(image->kernel_sign_algorithm);
  kernel_signature_len = siglen_map[image->kernel_sign_algorithm];

  /* Check whether key header length is correct. */
  header_len = (FIELD_LEN(header_version) +
                FIELD_LEN(header_len) +
                FIELD_LEN(firmware_sign_algorithm) +
                FIELD_LEN(kernel_sign_algorithm) +
                FIELD_LEN(kernel_key_version) +
                kernel_sign_key_len +
                FIELD_LEN(header_checksum));

  if (header_len != image->header_len) {
    fprintf(stderr, "Header length mismatch. Got: %d, Expected: %d\n",
            image->header_len, header_len);
    Free(kernel_buf);
    return NULL;
  }

  /* Read pre-processed public half of the kernel signing key. */
  StatefulMemcpy(&st, &image->kernel_key_version,
                 FIELD_LEN(kernel_key_version));
  image->kernel_sign_key = (uint8_t*) Malloc(kernel_sign_key_len);
  StatefulMemcpy(&st, image->kernel_sign_key, kernel_sign_key_len);
  StatefulMemcpy(&st, image->header_checksum, FIELD_LEN(header_checksum));

  /* Read key signature. */
  image->kernel_key_signature = (uint8_t*) Malloc(kernel_key_signature_len);
  StatefulMemcpy(&st, image->kernel_key_signature,
                 kernel_key_signature_len);

  /* Read the kernel config. */
  StatefulMemcpy(&st, &image->kernel_version, FIELD_LEN(kernel_version));
  StatefulMemcpy(&st, &image->options.version, FIELD_LEN(options.version));
  StatefulMemcpy(&st, &image->options.cmd_line, FIELD_LEN(options.cmd_line));
  StatefulMemcpy(&st, &image->options.kernel_len,
                 FIELD_LEN(options.kernel_len));
  StatefulMemcpy(&st, &image->options.kernel_load_addr,
                 FIELD_LEN(options.kernel_load_addr));
  StatefulMemcpy(&st, &image->options.kernel_entry_addr,
                 FIELD_LEN(options.kernel_entry_addr));

  /* Read kernel config signature. */
  image->config_signature = (uint8_t*) Malloc(kernel_signature_len);
  StatefulMemcpy(&st, image->config_signature, kernel_signature_len);

  image->kernel_signature = (uint8_t*) Malloc(kernel_signature_len);
  StatefulMemcpy(&st, image->kernel_signature, kernel_signature_len);

  image->kernel_data = (uint8_t*) Malloc(image->options.kernel_len);
  StatefulMemcpy(&st, image->kernel_data, image->options.kernel_len);

  if(st.remaining_len != 0) {  /* Overrun or underrun. */
    Free(kernel_buf);
    return NULL;
  }
  Free(kernel_buf);
  return image;
}

int GetKernelHeaderLen(const KernelImage* image) {
  return (FIELD_LEN(header_version) + FIELD_LEN(header_len) +
          FIELD_LEN(firmware_sign_algorithm) +
          FIELD_LEN(kernel_sign_algorithm) + FIELD_LEN(kernel_key_version) +
          RSAProcessedKeySize(image->kernel_sign_algorithm) +
          FIELD_LEN(header_checksum));
}

uint8_t* GetKernelHeaderBlob(const KernelImage* image) {
  uint8_t* header_blob = NULL;
  MemcpyState st;

  header_blob = (uint8_t*) Malloc(GetKernelHeaderLen(image));
  st.remaining_len = GetKernelHeaderLen(image);
  st.remaining_buf = header_blob;

  StatefulMemcpy_r(&st, &image->header_version, FIELD_LEN(header_version));
  StatefulMemcpy_r(&st, &image->header_len, FIELD_LEN(header_len));
  StatefulMemcpy_r(&st, &image->firmware_sign_algorithm,
                   FIELD_LEN(firmware_sign_algorithm));
  StatefulMemcpy_r(&st, &image->kernel_sign_algorithm,
                   FIELD_LEN(kernel_sign_algorithm));
  StatefulMemcpy_r(&st, &image->kernel_key_version,
                   FIELD_LEN(kernel_key_version));
  StatefulMemcpy_r(&st, image->kernel_sign_key,
                   RSAProcessedKeySize(image->kernel_sign_algorithm));
  StatefulMemcpy_r(&st, &image->header_checksum, FIELD_LEN(header_checksum));

  if (st.remaining_len != 0) {  /* Underrun or Overrun. */
    Free(header_blob);
    return NULL;
  }
  return header_blob;
}

int GetKernelConfigLen(const KernelImage* image) {
  return (FIELD_LEN(kernel_version) +
          FIELD_LEN(options.version) + FIELD_LEN(options.cmd_line) +
          FIELD_LEN(options.kernel_len) + FIELD_LEN(options.kernel_load_addr) +
          FIELD_LEN(options.kernel_entry_addr));
}

uint8_t* GetKernelConfigBlob(const KernelImage* image) {
  uint8_t* config_blob = NULL;
  MemcpyState st;

  config_blob = (uint8_t*) Malloc(GetKernelConfigLen(image));
  st.remaining_len = GetKernelConfigLen(image);
  st.remaining_buf = config_blob;

  StatefulMemcpy_r(&st, &image->kernel_version, FIELD_LEN(kernel_version));
  StatefulMemcpy_r(&st, image->options.version, FIELD_LEN(options.version));
  StatefulMemcpy_r(&st, image->options.cmd_line, FIELD_LEN(options.cmd_line));
  StatefulMemcpy_r(&st, &image->options.kernel_len,
                   FIELD_LEN(options.kernel_len));
  StatefulMemcpy_r(&st, &image->options.kernel_load_addr,
        FIELD_LEN(options.kernel_load_addr));
  StatefulMemcpy_r(&st, &image->options.kernel_entry_addr,
        FIELD_LEN(options.kernel_entry_addr));
  if (st.remaining_len != 0) {  /* Overrun or Underrun. */
    Free(config_blob);
    return NULL;
  }
  return config_blob;
}

uint8_t* GetKernelBlob(const KernelImage* image, int* blob_len) {
  int kernel_key_signature_len;
  int kernel_signature_len;
  uint8_t* kernel_blob = NULL;
  uint8_t* header_blob = NULL;
  uint8_t* config_blob = NULL;
  MemcpyState st;

  if (!image)
    return NULL;
  kernel_key_signature_len = siglen_map[image->firmware_sign_algorithm];
  kernel_signature_len = siglen_map[image->kernel_sign_algorithm];
  *blob_len = (FIELD_LEN(magic) +
               GetKernelHeaderLen(image) +
               kernel_key_signature_len +
               GetKernelConfigLen(image) +
               2 * kernel_signature_len +
               image->options.kernel_len);
  kernel_blob = (uint8_t*) Malloc(*blob_len);
  st.remaining_len = *blob_len;
  st.remaining_buf = kernel_blob;

  header_blob = GetKernelHeaderBlob(image);
  config_blob = GetKernelConfigBlob(image);

  StatefulMemcpy_r(&st, image->magic, FIELD_LEN(magic));
  StatefulMemcpy_r(&st, header_blob, GetKernelHeaderLen(image));
  StatefulMemcpy_r(&st, image->kernel_key_signature, kernel_key_signature_len);
  StatefulMemcpy_r(&st, config_blob, GetKernelConfigLen(image));
  StatefulMemcpy_r(&st, image->config_signature, kernel_signature_len);
  StatefulMemcpy_r(&st, image->kernel_signature, kernel_signature_len);
  StatefulMemcpy_r(&st, image->kernel_data, image->options.kernel_len);

  Free(config_blob);
  Free(header_blob);

  if (st.remaining_len != 0) {  /* Underrun or Overrun. */
    Free(kernel_blob);
    return NULL;
  }
  return kernel_blob;
}

int WriteKernelImage(const char* input_file,
                     const KernelImage* image) {
  int fd;
  uint8_t* kernel_blob;
  int blob_len;

  if (!image)
    return 0;
  if (-1 == (fd = creat(input_file, S_IRWXU))) {
    fprintf(stderr, "Couldn't open file for writing kernel image: %s\n",
            input_file);
    return 0;
  }
  kernel_blob = GetKernelBlob(image, &blob_len);
  if (!kernel_blob) {
    fprintf(stderr, "Couldn't create kernel blob from KernelImage.\n");
    return 0;
  }
  if (blob_len != write(fd, kernel_blob, blob_len)) {
    fprintf(stderr, "Couldn't write Kernel Image to file: %s\n",
            input_file);

    Free(kernel_blob);
    close(fd);
    return 0;
  }
  Free(kernel_blob);
  close(fd);
  return 1;
}

void PrintKernelImage(const KernelImage* image) {
  if (!image)
    return;

  /* Print header. */
  printf("Header Length = %d\n"
         "Firmware Signing key algorithm id = %d\n"
         "Kernel Signing key algorithm id = %d\n"
         "Kernel Signature Algorithm = %s\n"
         "Kernel Key Version = %d\n\n",
         image->header_len,
         image->firmware_sign_algorithm,
         image->kernel_sign_algorithm,
         algo_strings[image->kernel_sign_algorithm],
         image->kernel_key_version);
  /* TODO(gauravsh): Output hash and key signature here? */
  /* Print preamble. */
  printf("Kernel Version = %d\n"
         "Kernel Config Version = %d.%d\n"
         "Kernel Config command line = %s\n"
         "kernel Length = %" PRId64 "\n"
         "Kernel Load Address = %" PRId64 "\n"
         "Kernel Entry Address = %" PRId64 "\n\n",
         image->kernel_version,
         image->options.version[0], image->options.version[1],
         image->options.cmd_line,
         image->options.kernel_len,
         image->options.kernel_load_addr,
         image->options.kernel_entry_addr);
  /* TODO(gauravsh): Output kernel signature here? */
}

char* kVerifyKernelErrors[VERIFY_KERNEL_MAX] = {
  "Success.",
  "Invalid Image.",
  "Kernel Key Signature Failed.",
  "Invalid Kernel Verification Algorithm.",
  "Config Signature Failed.",
  "Kernel Signature Failed.",
  "Wrong Kernel Magic.",
};

int VerifyKernelHeader(const uint8_t* firmware_key_blob,
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
    fprintf(stderr, "VerifyKernelHeader: Header length mismatch\n");
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
    fprintf(stderr, "VerifyKernelHeader: Invalid header hash\n");
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

int VerifyKernelConfig(RSAPublicKey* kernel_sign_key,
                       const uint8_t* config_blob,
                       int algorithm,
                       int* kernel_len) {
  uint32_t len, config_len;
  config_len = GetKernelConfigLen(NULL);
  if (!RSAVerifyBinary_f(NULL, kernel_sign_key,  /* Key to use */
                         config_blob,  /* Data to verify */
                         config_len,  /* Length of data */
                         config_blob + config_len,  /* Expected Signature */
                         algorithm))
    return VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED;

  Memcpy(&len,
         config_blob + (FIELD_LEN(kernel_version) + FIELD_LEN(options.version) +
                              FIELD_LEN(options.cmd_line)),
         sizeof(len));
  *kernel_len = (int) len;
  return 0;
}

int VerifyKernelData(RSAPublicKey* kernel_sign_key,
                     const uint8_t* kernel_data_start,
                     int kernel_len,
                     int algorithm) {
  int signature_len = siglen_map[algorithm];
  if (!RSAVerifyBinary_f(NULL, kernel_sign_key,  /* Key to use. */
                         kernel_data_start + signature_len,  /* Data to
                                                              * verify */
                         kernel_len,  /* Length of data. */
                         kernel_data_start,  /* Expected Signature */
                         algorithm))
    return VERIFY_KERNEL_SIGNATURE_FAILED;
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
      header_len, kernel_len;
  const uint8_t* header_ptr;  /* Pointer to header. */
  const uint8_t* kernel_sign_key_ptr;  /* Pointer to signing key. */
  const uint8_t* config_ptr;  /* Pointer to kernel config block. */
  const uint8_t* kernel_ptr;  /* Pointer to kernel signature/data. */

  /* Note: All the offset calculations are based on struct FirmwareImage which
   * is defined in include/firmware_image.h. */

  /* Compare magic bytes. */
  if (SafeMemcmp(kernel_blob, KERNEL_MAGIC, KERNEL_MAGIC_SIZE))
    return VERIFY_KERNEL_WRONG_MAGIC;
  header_ptr = kernel_blob + KERNEL_MAGIC_SIZE;

  /* Only continue if header verification succeeds. */
  if ((error_code = VerifyKernelHeader(firmware_key_blob, header_ptr, dev_mode,
                                       &firmware_sign_algorithm,
                                       &kernel_sign_algorithm, &header_len))) {
    fprintf(stderr, "VerifyKernel: Kernel header verification failed.\n");
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

  /* Only continue if config verification succeeds. */
  config_ptr = (header_ptr + header_len + kernel_key_signature_len);
  if ((error_code = VerifyKernelConfig(kernel_sign_key, config_ptr,
                                       kernel_sign_algorithm,
                                       &kernel_len))) {
    RSAPublicKeyFree(kernel_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }
  /* Only continue if kernel data verification succeeds. */
  kernel_ptr = (config_ptr +
                GetKernelConfigLen(NULL) +  /* Skip config block/signature. */
                kernel_signature_len);

  if ((error_code = VerifyKernelData(kernel_sign_key, kernel_ptr, kernel_len,
                                     kernel_sign_algorithm))) {
    RSAPublicKeyFree(kernel_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }
  RSAPublicKeyFree(kernel_sign_key);
  return 0;  /* Success! */
}

int VerifyKernelImage(const RSAPublicKey* firmware_key,
                      const KernelImage* image,
                      const int dev_mode) {
  RSAPublicKey* kernel_sign_key = NULL;
  uint8_t* header_digest = NULL;
  uint8_t* config_digest = NULL;
  uint8_t* kernel_digest = NULL;
  int kernel_sign_key_size;
  int kernel_signature_size;
  int error_code = 0;
  DigestContext ctx;

  if (!image)
    return VERIFY_KERNEL_INVALID_IMAGE;

  /* Verify kernel key signature on the key header if we
   * are not in dev mode.
   *
   * TODO(gauravsh): Add additional sanity checks here for:
   *  1) verifying the header length is correct.
   *  2) header_checksum is correct.
   */

  if (image->firmware_sign_algorithm >= kNumAlgorithms)
    return VERIFY_KERNEL_INVALID_ALGORITHM;
  if (image->kernel_sign_algorithm >= kNumAlgorithms)
    return VERIFY_KERNEL_INVALID_ALGORITHM;

  if (!dev_mode) {
    DigestInit(&ctx, image->firmware_sign_algorithm);
    DigestUpdate(&ctx, (uint8_t*) &image->header_version,
                 FIELD_LEN(header_version));
    DigestUpdate(&ctx, (uint8_t*) &image->header_len,
                 FIELD_LEN(header_len));
    DigestUpdate(&ctx, (uint8_t*) &image->firmware_sign_algorithm,
                 FIELD_LEN(firmware_sign_algorithm));
    DigestUpdate(&ctx, (uint8_t*) &image->kernel_sign_algorithm,
                 FIELD_LEN(kernel_sign_algorithm));
    DigestUpdate(&ctx, (uint8_t*) &image->kernel_key_version,
                 FIELD_LEN(kernel_key_version));
    DigestUpdate(&ctx, image->kernel_sign_key,
                 RSAProcessedKeySize(image->kernel_sign_algorithm));
    DigestUpdate(&ctx, image->header_checksum,
                 FIELD_LEN(header_checksum));
    header_digest = DigestFinal(&ctx);
    if (!RSAVerify(firmware_key, image->kernel_key_signature,
                    siglen_map[image->firmware_sign_algorithm],
                    image->firmware_sign_algorithm,
                    header_digest)) {
      fprintf(stderr, "VerifyKernelImage(): Key signature check failed.\n");
      error_code =  VERIFY_KERNEL_KEY_SIGNATURE_FAILED;
      goto verify_failure;
    }
  }

  /* Get kernel signing key to verify the rest of the kernel. */
  kernel_sign_key_size = RSAProcessedKeySize(image->kernel_sign_algorithm);
  kernel_sign_key = RSAPublicKeyFromBuf(image->kernel_sign_key,
                                        kernel_sign_key_size);
  kernel_signature_size = siglen_map[image->kernel_sign_algorithm];

  /* Verify kernel config signature. */
  DigestInit(&ctx, image->kernel_sign_algorithm);
  DigestUpdate(&ctx, (uint8_t*) &image->kernel_version,
               FIELD_LEN(kernel_version));
  DigestUpdate(&ctx, (uint8_t*) image->options.version,
               FIELD_LEN(options.version));
  DigestUpdate(&ctx, (uint8_t*) image->options.cmd_line,
               FIELD_LEN(options.cmd_line));
  DigestUpdate(&ctx, (uint8_t*) &image->options.kernel_len,
               FIELD_LEN(options.kernel_len));
  DigestUpdate(&ctx, (uint8_t*) &image->options.kernel_load_addr,
               FIELD_LEN(options.kernel_load_addr));
  DigestUpdate(&ctx, (uint8_t*) &image->options.kernel_entry_addr,
               FIELD_LEN(options.kernel_entry_addr));
  config_digest = DigestFinal(&ctx);
  if (!RSAVerify(kernel_sign_key, image->config_signature,
                 kernel_signature_size, image->kernel_sign_algorithm,
                 config_digest)) {
    error_code = VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED;
    goto verify_failure;
  }

  /* Verify firmware signature. */
  kernel_digest = DigestBuf(image->kernel_data,
                            image->options.kernel_len,
                            image->kernel_sign_algorithm);
  if (!RSAVerify(kernel_sign_key, image->kernel_signature,
                 kernel_signature_size, image->kernel_sign_algorithm,
                 kernel_digest)) {
    error_code = VERIFY_KERNEL_SIGNATURE_FAILED;
    goto verify_failure;
  }

verify_failure:
  RSAPublicKeyFree(kernel_sign_key);
  Free(kernel_digest);
  Free(config_digest);
  Free(header_digest);
  return error_code;
}

const char* VerifyKernelErrorString(int error) {
  return kVerifyKernelErrors[error];
}

int AddKernelKeySignature(KernelImage* image, const char* firmware_key_file) {
  uint8_t* header_blob = NULL;
  uint8_t* signature = NULL;
  int signature_len = siglen_map[image->firmware_sign_algorithm];
  if (!image || !firmware_key_file)
    return 0;
  header_blob = GetKernelHeaderBlob(image);
  if (!header_blob)
    return 0;
  if (!(signature = SignatureBuf(header_blob,
                                 GetKernelHeaderLen(image),
                                 firmware_key_file,
                                 image->firmware_sign_algorithm))) {
    Free(header_blob);
    return 0;
  }
  image->kernel_key_signature = Malloc(signature_len);
  Memcpy(image->kernel_key_signature, signature, signature_len);
  Free(signature);
  Free(header_blob);
  return 1;
}

int AddKernelSignature(KernelImage* image,
                       const char* kernel_signing_key_file) {
  uint8_t* config_blob = NULL;
  uint8_t* config_signature = NULL;
  uint8_t* kernel_signature = NULL;
  int signature_len = siglen_map[image->kernel_sign_algorithm];

  config_blob = GetKernelConfigBlob(image);
  if (!(config_signature = SignatureBuf(config_blob,
                                        GetKernelConfigLen(image),
                                        kernel_signing_key_file,
                                        image->kernel_sign_algorithm))) {
    fprintf(stderr, "Could not compute signature on the kernel config.\n");
    Free(config_blob);
    return 0;
  }
  Free(config_blob);

  image->config_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->config_signature, config_signature, signature_len);
  Free(config_signature);

  if (!(kernel_signature = SignatureBuf(image->kernel_data,
                                        image->options.kernel_len,
                                        kernel_signing_key_file,
                                        image->kernel_sign_algorithm))) {
    fprintf(stderr, "Could not compute signature on the kernel.\n");
    return 0;
  }
  image->kernel_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->kernel_signature, kernel_signature, signature_len);
  Free(kernel_signature);
  return 1;
}
