/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for generating and manipulating a verified boot kernel image.
 * (Userland portion)
 */

#include "kernel_image.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "rollback_index.h"
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
    Memset(image->kernel_config,
           0,
           sizeof(image->kernel_config));
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
  uint8_t header_checksum[FIELD_LEN(header_checksum)];
  MemcpyState st;
  KernelImage* image = KernelImageNew();

  if (!image)
    return NULL;

  kernel_buf = BufferFromFile(input_file, &file_size);
  image_len = file_size;

  st.remaining_len = image_len;
  st.remaining_buf = kernel_buf;
  st.overrun = 0;

  /* Read and compare magic bytes. */
  StatefulMemcpy(&st, &image->magic, KERNEL_MAGIC_SIZE);

  if (SafeMemcmp(image->magic, KERNEL_MAGIC, KERNEL_MAGIC_SIZE)) {
    debug("Wrong Kernel Magic.\n");
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
  header_len = GetKernelHeaderLen(image);
  if (header_len != image->header_len) {
    debug("Header length mismatch. Got: %d, Expected: %d\n",
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

  /* Check whether the header checksum matches. */
  CalculateKernelHeaderChecksum(image, header_checksum);
  if (SafeMemcmp(header_checksum, image->header_checksum,
                 FIELD_LEN(header_checksum))) {
    debug("Invalid kernel header checksum!\n");
    Free(kernel_buf);
    return NULL;
  }

  /* Read key signature. */
  image->kernel_key_signature = (uint8_t*) Malloc(kernel_key_signature_len);
  StatefulMemcpy(&st, image->kernel_key_signature,
                 kernel_key_signature_len);

  /* Read the kernel config. */
  StatefulMemcpy(&st, &image->kernel_version, FIELD_LEN(kernel_version));
  StatefulMemcpy(&st, &image->kernel_len, FIELD_LEN(kernel_len));

  /* Read config and kernel signatures. */
  image->config_signature = (uint8_t*) Malloc(kernel_signature_len);
  StatefulMemcpy(&st, image->config_signature, kernel_signature_len);
  image->kernel_signature = (uint8_t*) Malloc(kernel_signature_len);
  StatefulMemcpy(&st, image->kernel_signature, kernel_signature_len);

  /* Read kernel config command line and kernel image data. */
  StatefulMemcpy(&st, image->kernel_config, FIELD_LEN(kernel_config));
  image->kernel_data = (uint8_t*) Malloc(image->kernel_len);
  StatefulMemcpy(&st, image->kernel_data, image->kernel_len);

  if(st.overrun || st.remaining_len != 0) {  /* Overrun or underrun. */
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

void CalculateKernelHeaderChecksum(const KernelImage* image,
                                   uint8_t* header_checksum) {
  uint8_t* checksum;
  DigestContext ctx;
  DigestInit(&ctx, SHA512_DIGEST_ALGORITHM);
  DigestUpdate(&ctx, (uint8_t*) &image->header_version,
               sizeof(image->header_version));
  DigestUpdate(&ctx, (uint8_t*) &image->header_len,
               sizeof(image->header_len));
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_sign_algorithm,
               sizeof(image->firmware_sign_algorithm));
  DigestUpdate(&ctx, (uint8_t*) &image->kernel_sign_algorithm,
               sizeof(image->kernel_sign_algorithm));
  DigestUpdate(&ctx, (uint8_t*) &image->kernel_key_version,
               sizeof(image->kernel_key_version));
  DigestUpdate(&ctx, image->kernel_sign_key,
               RSAProcessedKeySize(image->kernel_sign_algorithm));
  checksum = DigestFinal(&ctx);
  Memcpy(header_checksum, checksum, FIELD_LEN(header_checksum));
  Free(checksum);
  return;
}

uint8_t* GetKernelHeaderBlob(const KernelImage* image) {
  uint8_t* header_blob = NULL;
  MemcpyState st;

  header_blob = (uint8_t*) Malloc(GetKernelHeaderLen(image));
  st.remaining_len = GetKernelHeaderLen(image);
  st.remaining_buf = header_blob;
  st.overrun = 0;

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

  if (st.overrun || st.remaining_len != 0) {  /* Underrun or Overrun. */
    Free(header_blob);
    return NULL;
  }
  return header_blob;
}

int GetKernelConfigLen(const KernelImage* image) {
  return (FIELD_LEN(kernel_version) +
          FIELD_LEN(kernel_len) +
          FIELD_LEN(kernel_config));
}

uint8_t* GetKernelConfigBlob(const KernelImage* image) {
  uint8_t* config_blob = NULL;
  MemcpyState st;

  config_blob = (uint8_t*) Malloc(GetKernelConfigLen(image));
  st.remaining_len = GetKernelConfigLen(image);
  st.remaining_buf = config_blob;
  st.overrun = 0;

  StatefulMemcpy_r(&st, &image->kernel_version, FIELD_LEN(kernel_version));
  StatefulMemcpy_r(&st, &image->kernel_len, FIELD_LEN(kernel_len));
  StatefulMemcpy_r(&st, image->kernel_config, FIELD_LEN(kernel_config));

  if (st.overrun || st.remaining_len != 0) {  /* Overrun or Underrun. */
    Free(config_blob);
    return NULL;
  }
  return config_blob;
}

uint8_t* GetKernelBlob(const KernelImage* image, uint64_t* blob_len) {
  int kernel_key_signature_len;
  int kernel_signature_len;
  uint8_t* kernel_blob = NULL;
  uint8_t* header_blob = NULL;
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
               image->kernel_len);
  kernel_blob = (uint8_t*) Malloc(*blob_len);
  st.remaining_len = *blob_len;
  st.remaining_buf = kernel_blob;
  st.overrun = 0;

  header_blob = GetKernelHeaderBlob(image);

  StatefulMemcpy_r(&st, image->magic, FIELD_LEN(magic));
  StatefulMemcpy_r(&st, header_blob, GetKernelHeaderLen(image));
  StatefulMemcpy_r(&st, image->kernel_key_signature, kernel_key_signature_len);
  /* Copy over kernel config blob (including signatures.) */
  StatefulMemcpy_r(&st, &image->kernel_version, FIELD_LEN(kernel_version));
  StatefulMemcpy_r(&st, &image->kernel_len, FIELD_LEN(kernel_len));
  StatefulMemcpy_r(&st, image->config_signature, kernel_signature_len);
  StatefulMemcpy_r(&st, image->kernel_signature, kernel_signature_len);
  StatefulMemcpy_r(&st, image->kernel_config, FIELD_LEN(kernel_config));
  StatefulMemcpy_r(&st, image->kernel_data, image->kernel_len);

  Free(header_blob);

  if (st.overrun || st.remaining_len != 0) {  /* Underrun or Overrun. */
    Free(kernel_blob);
    return NULL;
  }
  return kernel_blob;
}

int WriteKernelImage(const char* input_file,
                     const KernelImage* image,
                     int is_only_vblock) {
  int fd;
  int success = 1;
  uint8_t* kernel_blob;
  uint64_t blob_len;

  if (!image)
    return 0;
  if (-1 == (fd = creat(input_file, S_IRWXU))) {
    debug("Couldn't open file for writing kernel image: %s\n",
            input_file);
    return 0;
  }
  kernel_blob = GetKernelBlob(image, &blob_len);
  if (!kernel_blob) {
    debug("Couldn't create kernel blob from KernelImage.\n");
    return 0;
  }
  if (!is_only_vblock) {
    if (blob_len != write(fd, kernel_blob, blob_len)) {
      debug("Couldn't write Kernel Image to file: %s\n",
            input_file);
      success = 0;
    }
  } else {
    /* Exclude kernel_config and kernel_data. */
    int vblock_len = blob_len - (image->kernel_len +
                                 sizeof(image->kernel_config));
    if (vblock_len != write(fd, kernel_blob, vblock_len)) {
      debug("Couldn't write Kernel Image Verification block to file: %s\n",
            input_file);
      success = 0;
    }
  }
  Free(kernel_blob);
  close(fd);
  return success;
}

void PrintKernelImage(const KernelImage* image) {
  if (!image)
    return;

  /* Print header. */
  printf("Header Version = %d\n"
         "Header Length = %d\n"
         "Kernel Key Signature Algorithm = %s\n"
         "Kernel Signature Algorithm = %s\n"
         "Kernel Key Version = %d\n\n",
         image->header_version,
         image->header_len,
         algo_strings[image->firmware_sign_algorithm],
         algo_strings[image->kernel_sign_algorithm],
         image->kernel_key_version);
  /* TODO(gauravsh): Output hash and key signature here? */
  /* Print preamble. */
  printf("Kernel Version = %d\n"
         "Kernel Config command line = \"%s\"\n"
         "kernel Length = %" PRId64 "\n",
         image->kernel_version,
         image->kernel_config,
         image->kernel_len);
  /* TODO(gauravsh): Output kernel signature here? */
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
  DigestContext kernel_ctx;
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
      debug("VerifyKernelImage(): Key signature check failed.\n");
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
  DigestUpdate(&ctx, (uint8_t*) &image->kernel_len,
               FIELD_LEN(kernel_len));
  DigestUpdate(&ctx, (uint8_t*) image->kernel_config,
               FIELD_LEN(kernel_config));
  config_digest = DigestFinal(&ctx);
  if (!RSAVerify(kernel_sign_key, image->config_signature,
                 kernel_signature_size, image->kernel_sign_algorithm,
                 config_digest)) {
    error_code = VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED;
    goto verify_failure;
  }

  /* Verify kernel signature - kernel signature is computed on the contents
     of kernel version + kernel options + kernel_data. */
  DigestInit(&kernel_ctx, image->kernel_sign_algorithm);
  DigestUpdate(&kernel_ctx, (uint8_t*) &image->kernel_version,
               FIELD_LEN(kernel_version));
  DigestUpdate(&kernel_ctx, (uint8_t*) &image->kernel_len,
               FIELD_LEN(kernel_len));
  DigestUpdate(&kernel_ctx, (uint8_t*) image->kernel_config,
               FIELD_LEN(kernel_config));
  DigestUpdate(&kernel_ctx, image->kernel_data, image->kernel_len);
  kernel_digest = DigestFinal(&kernel_ctx);
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
  uint8_t* kernel_buf;
  int signature_len = siglen_map[image->kernel_sign_algorithm];

  config_blob = GetKernelConfigBlob(image);
  if (!(config_signature = SignatureBuf(config_blob,
                                        GetKernelConfigLen(image),
                                        kernel_signing_key_file,
                                        image->kernel_sign_algorithm))) {
    debug("Could not compute signature on the kernel config.\n");
    Free(config_blob);
    return 0;
  }

  image->config_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->config_signature, config_signature, signature_len);
  Free(config_signature);
  /* Kernel signature muse be calculated on the kernel version, options and
   * kernel data to avoid splicing attacks. */
  kernel_buf = (uint8_t*) Malloc(GetKernelConfigLen(image) +
                                 image->kernel_len);
  Memcpy(kernel_buf, config_blob, GetKernelConfigLen(image));
  Memcpy(kernel_buf + GetKernelConfigLen(image), image->kernel_data,
         image->kernel_len);
  if (!(kernel_signature = SignatureBuf(kernel_buf,
                                        GetKernelConfigLen(image) +
                                        image->kernel_len,
                                        kernel_signing_key_file,
                                        image->kernel_sign_algorithm))) {
    Free(config_blob);
    Free(kernel_buf);
    debug("Could not compute signature on the kernel.\n");
    return 0;
  }
  image->kernel_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->kernel_signature, kernel_signature, signature_len);
  Free(kernel_signature);
  Free(kernel_buf);
  Free(config_blob);
  return 1;
}

void PrintKernelEntry(kernel_entry* entry) {
  debug("Boot Priority = %d\n", entry->boot_priority);
  debug("Boot Tries Remaining = %d\n", entry->boot_tries_remaining);
  debug("Boot Success Flag = %d\n", entry->boot_success_flag);
}
