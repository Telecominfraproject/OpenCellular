/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for generating and manipulating a verified boot firmware image.
 */

#include "firmware_image.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_keys.h"
#include "padding.h"
#include "rollback_index.h"
#include "rsa_utility.h"
#include "sha_utility.h"
#include "signature_digest.h"
#include "utility.h"

/* Macro to determine the size of a field structure in the FirmwareImage
 * structure. */
#define FIELD_LEN(field) (sizeof(((FirmwareImage*)0)->field))

FirmwareImage* FirmwareImageNew(void) {
  FirmwareImage* image = (FirmwareImage*) Malloc(sizeof(FirmwareImage));
  if (image) {
    image->firmware_sign_key = NULL;
    image->preamble_signature = NULL;
    image->firmware_signature = NULL;
    image->firmware_data = NULL;
  }
  return image;
}

void FirmwareImageFree(FirmwareImage* image) {
  if (image) {
    Free(image->firmware_sign_key);
    Free(image->preamble_signature);
    Free(image->firmware_signature);
    Free(image->firmware_data);
    Free(image);
  }
}

FirmwareImage* ReadFirmwareImage(const char* input_file) {
  uint64_t file_size;
  int image_len = 0;  /* Total size of the firmware image. */
  int header_len = 0;
  int firmware_sign_key_len;
  int signature_len;
  uint8_t* firmware_buf;
  uint8_t header_checksum[FIELD_LEN(header_checksum)];
  MemcpyState st;
  FirmwareImage* image = FirmwareImageNew();

  if (!image)
    return NULL;

  firmware_buf = BufferFromFile(input_file, &file_size);
  image_len = file_size;

  st.remaining_len = image_len;
  st.remaining_buf = firmware_buf;

  /* Read and compare magic bytes. */
  StatefulMemcpy(&st, &image->magic, FIRMWARE_MAGIC_SIZE);
  if (SafeMemcmp(image->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE)) {
    fprintf(stderr, "Wrong Firmware Magic.\n");
    Free(firmware_buf);
    return NULL;
  }
  StatefulMemcpy(&st, &image->header_len, FIELD_LEN(header_len));
  StatefulMemcpy(&st, &image->firmware_sign_algorithm,
                 FIELD_LEN(firmware_sign_algorithm));

  /* Valid Algorithm? */
  if (image->firmware_sign_algorithm >= kNumAlgorithms) {
    Free(firmware_buf);
    return NULL;
  }

  /* Compute size of pre-processed RSA public key and signature. */
  firmware_sign_key_len = RSAProcessedKeySize(image->firmware_sign_algorithm);
  signature_len = siglen_map[image->firmware_sign_algorithm];

  /* Check whether the header length is correct. */
  header_len = GetFirmwareHeaderLen(image);
  if (header_len != image->header_len) {
    fprintf(stderr, "Header length mismatch. Got: %d Expected: %d\n",
            image->header_len, header_len);
    Free(firmware_buf);
    return NULL;
  }

  /* Read pre-processed public half of the sign key. */
  StatefulMemcpy(&st, &image->firmware_key_version,
                 FIELD_LEN(firmware_key_version));
  image->firmware_sign_key = (uint8_t*) Malloc(firmware_sign_key_len);
  StatefulMemcpy(&st, image->firmware_sign_key, firmware_sign_key_len);
  StatefulMemcpy(&st, image->header_checksum, FIELD_LEN(header_checksum));

  /* Check whether the header checksum matches. */
  CalculateFirmwareHeaderChecksum(image, header_checksum);
  if (SafeMemcmp(header_checksum, image->header_checksum,
                 FIELD_LEN(header_checksum))) {
    fprintf(stderr, "Invalid firmware header checksum!\n");
    Free(firmware_buf);
    return NULL;
  }

  /* Read key signature. */
  StatefulMemcpy(&st, image->firmware_key_signature,
                 FIELD_LEN(firmware_key_signature));

  /* Read the firmware preamble. */
  StatefulMemcpy(&st,&image->firmware_version, FIELD_LEN(firmware_version));
  StatefulMemcpy(&st, &image->firmware_len, FIELD_LEN(firmware_len));
  StatefulMemcpy(&st, image->preamble, FIELD_LEN(preamble));

  /* Read firmware preamble signature. */
  image->preamble_signature = (uint8_t*) Malloc(signature_len);
  StatefulMemcpy(&st, image->preamble_signature, signature_len);

  image->firmware_signature = (uint8_t*) Malloc(signature_len);
  StatefulMemcpy(&st, image->firmware_signature, signature_len);

  image->firmware_data = (uint8_t*) Malloc(image->firmware_len);
  StatefulMemcpy(&st, image->firmware_data, image->firmware_len);

  if(st.remaining_len != 0) {  /* Overrun or underrun. */
    Free(firmware_buf);
    return NULL;
  }

  Free(firmware_buf);
  return image;
}

int GetFirmwareHeaderLen(const FirmwareImage* image) {
  return (FIELD_LEN(header_len) + FIELD_LEN(firmware_sign_algorithm) +
          RSAProcessedKeySize(image->firmware_sign_algorithm) +
          FIELD_LEN(firmware_key_version) + FIELD_LEN(header_checksum));
}

void CalculateFirmwareHeaderChecksum(const FirmwareImage* image,
                                     uint8_t* header_checksum) {
  uint8_t* checksum;
  DigestContext ctx;
  DigestInit(&ctx, SHA512_DIGEST_ALGORITHM);
  DigestUpdate(&ctx, (uint8_t*) &image->header_len,
               sizeof(image->header_len));
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_sign_algorithm,
               sizeof(image->firmware_sign_algorithm));
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_key_version,
               sizeof(image->firmware_key_version));
  DigestUpdate(&ctx, image->firmware_sign_key,
               RSAProcessedKeySize(image->firmware_sign_algorithm));
  checksum = DigestFinal(&ctx);
  Memcpy(header_checksum, checksum, FIELD_LEN(header_checksum));
  Free(checksum);
  return;
}


uint8_t* GetFirmwareHeaderBlob(const FirmwareImage* image) {
  uint8_t* header_blob = NULL;
  MemcpyState st;

  header_blob = (uint8_t*) Malloc(GetFirmwareHeaderLen(image));
  st.remaining_len = GetFirmwareHeaderLen(image);
  st.remaining_buf = header_blob;

  StatefulMemcpy_r(&st, &image->header_len, FIELD_LEN(header_len));
  StatefulMemcpy_r(&st, &image->firmware_sign_algorithm, FIELD_LEN(header_len));
  StatefulMemcpy_r(&st, &image->firmware_key_version,
                 FIELD_LEN(firmware_key_version));
  StatefulMemcpy_r(&st, image->firmware_sign_key,
                 RSAProcessedKeySize(image->firmware_sign_algorithm));
  StatefulMemcpy_r(&st, &image->header_checksum, FIELD_LEN(header_checksum));

  if (st.remaining_len != 0) {  /* Underrun or Overrun. */
    Free(header_blob);
    return NULL;
  }
  return header_blob;
}

int GetFirmwarePreambleLen(const FirmwareImage* image) {
  return (FIELD_LEN(firmware_version) + FIELD_LEN(firmware_len) +
          FIELD_LEN(preamble));
}

uint8_t* GetFirmwarePreambleBlob(const FirmwareImage* image) {
  uint8_t* preamble_blob = NULL;
  MemcpyState st;

  preamble_blob = (uint8_t*) Malloc(GetFirmwarePreambleLen(image));
  st.remaining_len = GetFirmwarePreambleLen(image);
  st.remaining_buf = preamble_blob;

  StatefulMemcpy_r(&st, &image->firmware_version, FIELD_LEN(firmware_version));
  StatefulMemcpy_r(&st, &image->firmware_len, FIELD_LEN(firmware_len));
  StatefulMemcpy_r(&st, image->preamble, FIELD_LEN(preamble));

  if (st.remaining_len != 0 ) {  /* Underrun or Overrun. */
    Free(preamble_blob);
    return NULL;
  }
  return preamble_blob;
}


uint8_t* GetFirmwareBlob(const FirmwareImage* image, uint64_t* blob_len) {
  int firmware_signature_len;
  uint8_t* firmware_blob = NULL;
  uint8_t* header_blob = NULL;
  uint8_t* preamble_blob = NULL;
  MemcpyState st;

  if (!image)
    return NULL;

  firmware_signature_len = siglen_map[image->firmware_sign_algorithm];
  *blob_len = (FIELD_LEN(magic) +
               GetFirmwareHeaderLen(image) +
               FIELD_LEN(firmware_key_signature) +
               GetFirmwarePreambleLen(image) +
               2 * firmware_signature_len +
               image->firmware_len);
  firmware_blob = (uint8_t*) Malloc(*blob_len);
  st.remaining_len = *blob_len;
  st.remaining_buf = firmware_blob;

  header_blob = GetFirmwareHeaderBlob(image);
  preamble_blob = GetFirmwarePreambleBlob(image);

  StatefulMemcpy_r(&st, image->magic, FIELD_LEN(magic));
  StatefulMemcpy_r(&st, header_blob, GetFirmwareHeaderLen(image));
  StatefulMemcpy_r(&st, image->firmware_key_signature,
                   FIELD_LEN(firmware_key_signature));
  StatefulMemcpy_r(&st, preamble_blob, GetFirmwarePreambleLen(image));
  StatefulMemcpy_r(&st, image->preamble_signature, firmware_signature_len);
  StatefulMemcpy_r(&st, image->firmware_signature, firmware_signature_len);
  StatefulMemcpy_r(&st, image->firmware_data, image->firmware_len);

  Free(preamble_blob);
  Free(header_blob);

  if (st.remaining_len != 0) { /* Underrun or Overrun. */
    Free(firmware_blob);
    return NULL;
  }
  return firmware_blob;
}

int WriteFirmwareImage(const char* input_file,
                       const FirmwareImage* image) {
  int fd;
  uint8_t* firmware_blob;
  uint64_t blob_len;

  if (!image)
    return 0;
  if (-1 == (fd = creat(input_file, S_IRWXU))) {
    fprintf(stderr, "Couldn't open file for writing.\n");
    return 0;
  }

  firmware_blob = GetFirmwareBlob(image, &blob_len);
  if (!firmware_blob) {
    fprintf(stderr, "Couldn't create firmware blob from FirmwareImage.\n");
    return 0;
  }
  if (blob_len != write(fd, firmware_blob, blob_len)) {
    fprintf(stderr, "Couldn't write Firmware Image to file: %s\n", input_file);
    Free(firmware_blob);
    close(fd);
    return 0;
  }
  Free(firmware_blob);
  close(fd);
  return 1;
}

void PrintFirmwareImage(const FirmwareImage* image) {
  if (!image)
    return;

  /* Print header. */
  printf("Header Length = %d\n"
         "Firmware Signature Algorithm = %s\n"
         "Firmware Key Version = %d\n\n",
         image->header_len,
         algo_strings[image->firmware_sign_algorithm],
         image->firmware_key_version);
  /* TODO(gauravsh): Output hash and key signature here? */
  /* Print preamble. */
  printf("Firmware Version = %d\n"
         "Firmware Length = %" PRIu64 "\n\n",
         image->firmware_version,
         image->firmware_len);
  /* Output key signature here? */
}

char* kVerifyFirmwareErrors[VERIFY_FIRMWARE_MAX] = {
  "Success.",
  "Invalid Image.",
  "Root Key Signature Failed.",
  "Invalid Verification Algorithm.",
  "Preamble Signature Failed.",
  "Firmware Signature Failed.",
  "Wrong Firmware Magic.",
  "Invalid Firmware Header Checksum.",
  "Firmware Signing Key Rollback.",
  "Firmware Version Rollback."
};

int VerifyFirmwareHeader(const uint8_t* root_key_blob,
                         const uint8_t* header_blob,
                         const int dev_mode,
                         int* algorithm,
                         int* header_len) {
  int firmware_sign_key_len;
  int root_key_len;
  uint16_t hlen, algo;
  uint8_t* header_checksum = NULL;

  /* Base Offset for the header_checksum field. Actual offset is
   * this + firmware_sign_key_len. */
  int base_header_checksum_offset = (FIELD_LEN(header_len) +
                                     FIELD_LEN(firmware_sign_algorithm) +
                                     FIELD_LEN(firmware_key_version));


  root_key_len = RSAProcessedKeySize(ROOT_SIGNATURE_ALGORITHM);
  Memcpy(&hlen, header_blob, sizeof(hlen));
  Memcpy(&algo,
         header_blob + FIELD_LEN(firmware_sign_algorithm),
         sizeof(algo));
  if (algo >= kNumAlgorithms)
    return VERIFY_FIRMWARE_INVALID_ALGORITHM;
  *algorithm = (int) algo;
  firmware_sign_key_len = RSAProcessedKeySize(*algorithm);

  /* Verify that header len is correct. */
  if (hlen != (base_header_checksum_offset +
               firmware_sign_key_len +
               FIELD_LEN(header_checksum)))
    return VERIFY_FIRMWARE_INVALID_IMAGE;

  *header_len = (int) hlen;

  /* Verify if the hash of the header is correct. */
  header_checksum = DigestBuf(header_blob,
                              *header_len - FIELD_LEN(header_checksum),
                              SHA512_DIGEST_ALGORITHM);
  if (SafeMemcmp(header_checksum,
                  header_blob + (base_header_checksum_offset +
                                 firmware_sign_key_len),
                  FIELD_LEN(header_checksum))) {
    Free(header_checksum);
    return VERIFY_FIRMWARE_WRONG_HEADER_CHECKSUM;
  }
  Free(header_checksum);

  /* Root key signature on the firmware signing key is always checked
   * irrespective of dev mode. */
  if (!RSAVerifyBinary_f(root_key_blob, NULL,  /* Key to use */
                         header_blob,  /* Data to verify */
                         *header_len, /* Length of data */
                         header_blob + *header_len,  /* Expected Signature */
                         ROOT_SIGNATURE_ALGORITHM))
    return VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED;
  return 0;
}

int VerifyFirmwarePreamble(RSAPublicKey* firmware_sign_key,
                           const uint8_t* preamble_blob,
                           int algorithm,
                           int* firmware_len) {
  uint32_t len;
  int preamble_len;
  uint16_t firmware_version;

  Memcpy(&firmware_version, preamble_blob, sizeof(firmware_version));

  preamble_len = (FIELD_LEN(firmware_version) +
                  FIELD_LEN(firmware_len) +
                  FIELD_LEN(preamble));
  if (!RSAVerifyBinary_f(NULL, firmware_sign_key,  /* Key to use */
                         preamble_blob,  /* Data to verify */
                         preamble_len,  /* Length of data */
                         preamble_blob + preamble_len,  /* Expected Signature */
                         algorithm))
    return VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED;

  Memcpy(&len, preamble_blob + FIELD_LEN(firmware_version),
         sizeof(len));
  *firmware_len = (int) len;
  return 0;
}

int VerifyFirmwareData(RSAPublicKey* firmware_sign_key,
                       const uint8_t* firmware_data_start,
                       int firmware_len,
                       int algorithm) {
  int signature_len = siglen_map[algorithm];
  if (!RSAVerifyBinary_f(NULL, firmware_sign_key,  /* Key to use. */
                         firmware_data_start + signature_len,  /* Data to
                                                                * verify */
                         firmware_len,  /* Length of data. */
                         firmware_data_start,  /* Expected Signature */
                         algorithm))
    return VERIFY_FIRMWARE_SIGNATURE_FAILED;
  return 0;
}

int VerifyFirmware(const uint8_t* root_key_blob,
                   const uint8_t* firmware_blob,
                   const int dev_mode) {
  int error_code;
  int algorithm;  /* Signing key algorithm. */
  RSAPublicKey* firmware_sign_key = NULL;
  int firmware_sign_key_len, signature_len, header_len, firmware_len;
  const uint8_t* header_ptr = NULL;  /* Pointer to header. */
  const uint8_t* firmware_sign_key_ptr = NULL;  /* Pointer to signing key. */
  const uint8_t* preamble_ptr = NULL;  /* Pointer to preamble block. */
  const uint8_t* firmware_ptr = NULL;  /* Pointer to firmware signature/data. */

  /* Note: All the offset calculations are based on struct FirmwareImage which
   * is defined in include/firmware_image.h. */

  /* Compare magic bytes. */
  if (SafeMemcmp(firmware_blob, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE))
    return VERIFY_FIRMWARE_WRONG_MAGIC;
  header_ptr = firmware_blob + FIRMWARE_MAGIC_SIZE;

  /* Only continue if header verification succeeds. */
  if ((error_code = VerifyFirmwareHeader(root_key_blob, header_ptr, dev_mode,
                                         &algorithm, &header_len)))
    return error_code;  /* AKA jump to revovery. */

  /* Parse signing key into RSAPublicKey structure since it is required multiple
   * times. */
  firmware_sign_key_len = RSAProcessedKeySize(algorithm);
  firmware_sign_key_ptr = header_ptr + (FIELD_LEN(header_len) +
                                        FIELD_LEN(firmware_sign_algorithm) +
                                        FIELD_LEN(firmware_key_version));
  firmware_sign_key = RSAPublicKeyFromBuf(firmware_sign_key_ptr,
                                          firmware_sign_key_len);
  signature_len = siglen_map[algorithm];

  /* Only continue if preamble verification succeeds. */
  preamble_ptr = (header_ptr + header_len +
                  FIELD_LEN(firmware_key_signature));
  if ((error_code = VerifyFirmwarePreamble(firmware_sign_key, preamble_ptr,
                                           algorithm,
                                           &firmware_len))) {
    RSAPublicKeyFree(firmware_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }
  /* Only continue if firmware data verification succeeds. */
  firmware_ptr = (preamble_ptr +
                  GetFirmwarePreambleLen(NULL) +
                  signature_len);

  if ((error_code = VerifyFirmwareData(firmware_sign_key, firmware_ptr,
                                       firmware_len,
                                       algorithm))) {
    RSAPublicKeyFree(firmware_sign_key);
    return error_code;  /* AKA jump to recovery. */
  }

  RSAPublicKeyFree(firmware_sign_key);
  return 0;  /* Success! */
}

int VerifyFirmwareImage(const RSAPublicKey* root_key,
                        const FirmwareImage* image,
                        const int dev_mode) {
  RSAPublicKey* firmware_sign_key = NULL;
  uint8_t* header_digest = NULL;
  uint8_t* preamble_digest = NULL;
  uint8_t* firmware_digest = NULL;
  int firmware_sign_key_size;
  int signature_size;
  int error_code = 0;
  DigestContext ctx;

  if (!image)
    return VERIFY_FIRMWARE_INVALID_IMAGE;

  /* Verify root key signature on the sign key header if we
   * are not in dev mode.
   *
   * TODO(gauravsh): Add additional sanity checks here for:
   *  1) verifying the header length is correct.
   *  2) header_checksum is correct.
   */
  /* TODO(gauravsh): The [dev_mode] switch is actually irrelevant
   * for the firmware verification.
   * Change this to always verify the root key signature and change
   * test expectations appropriately.
   */
  if (!dev_mode) {
    DigestInit(&ctx, ROOT_SIGNATURE_ALGORITHM);
    DigestUpdate(&ctx, (uint8_t*) &image->header_len,
                 FIELD_LEN(header_len));
    DigestUpdate(&ctx, (uint8_t*) &image->firmware_sign_algorithm,
                 FIELD_LEN(firmware_sign_algorithm));
    DigestUpdate(&ctx, (uint8_t*) &image->firmware_key_version,
                 FIELD_LEN(firmware_key_version));
    DigestUpdate(&ctx, image->firmware_sign_key,
                 RSAProcessedKeySize(image->firmware_sign_algorithm));
    DigestUpdate(&ctx, image->header_checksum,
                 FIELD_LEN(header_checksum));
    header_digest = DigestFinal(&ctx);
    if (!RSAVerify(root_key, image->firmware_key_signature,
                    FIELD_LEN(firmware_key_signature),
                    ROOT_SIGNATURE_ALGORITHM,
                    header_digest)) {
      error_code =  VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED;
      goto verify_failure;
    }
  }

  /* Get sign key to verify the rest of the firmware. */
  firmware_sign_key_size = RSAProcessedKeySize(image->firmware_sign_algorithm);
  firmware_sign_key = RSAPublicKeyFromBuf(image->firmware_sign_key,
                                 firmware_sign_key_size);
  signature_size = siglen_map[image->firmware_sign_algorithm];

  if (image->firmware_sign_algorithm >= kNumAlgorithms)
    return VERIFY_FIRMWARE_INVALID_ALGORITHM;

  /* Verify firmware preamble signature. */
  DigestInit(&ctx, image->firmware_sign_algorithm);
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_version,
               FIELD_LEN(firmware_version));
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_len,
               FIELD_LEN(firmware_len));
  DigestUpdate(&ctx, (uint8_t*) &image->preamble,
               FIELD_LEN(preamble));
  preamble_digest = DigestFinal(&ctx);
  if (!RSAVerify(firmware_sign_key, image->preamble_signature,
                  signature_size, image->firmware_sign_algorithm,
                  preamble_digest)) {
    error_code = VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED;
    goto verify_failure;
  }

  /* Verify firmware signature. */
  firmware_digest = DigestBuf(image->firmware_data,
                              image->firmware_len,
                              image->firmware_sign_algorithm);
  if (!RSAVerify(firmware_sign_key, image->firmware_signature,
                 signature_size, image->firmware_sign_algorithm,
                 firmware_digest)) {
    error_code = VERIFY_FIRMWARE_SIGNATURE_FAILED;
    goto verify_failure;
  }

verify_failure:
  RSAPublicKeyFree(firmware_sign_key);
  Free(firmware_digest);
  Free(preamble_digest);
  Free(header_digest);
  return error_code;
}

const char* VerifyFirmwareErrorString(int error) {
  return kVerifyFirmwareErrors[error];
}

int AddFirmwareKeySignature(FirmwareImage* image, const char* root_key_file) {
  uint8_t* header_blob = NULL;
  uint8_t* signature;
  if (!image || !root_key_file)
    return 0;
  header_blob = GetFirmwareHeaderBlob(image);
  if (!header_blob)
    return 0;
  if (!(signature = SignatureBuf(header_blob,
                                 GetFirmwareHeaderLen(image),
                                 root_key_file,
                                 ROOT_SIGNATURE_ALGORITHM))) {
    Free(header_blob);
    return 0;
  }
  Memcpy(image->firmware_key_signature, signature, RSA8192NUMBYTES);
  Free(header_blob);
  Free(signature);
  return 1;
}

int AddFirmwareSignature(FirmwareImage* image, const char* signing_key_file) {
  uint8_t* preamble_blob;
  uint8_t* preamble_signature;
  uint8_t* firmware_signature;
  int signature_len = siglen_map[image->firmware_sign_algorithm];

  preamble_blob = GetFirmwarePreambleBlob(image);
  if (!(preamble_signature = SignatureBuf(preamble_blob,
                                          GetFirmwarePreambleLen(image),
                                          signing_key_file,
                                          image->firmware_sign_algorithm))) {
    Free(preamble_blob);
    return 0;
  }
  image->preamble_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->preamble_signature, preamble_signature, signature_len);
  Free(preamble_signature);

  if (!(firmware_signature = SignatureBuf(image->firmware_data,
                                          image->firmware_len,
                                          signing_key_file,
                                          image->firmware_sign_algorithm)))
    return 0;
  image->firmware_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->firmware_signature, firmware_signature, signature_len);
  Free(firmware_signature);
  return 1;
}

uint32_t GetLogicalFirmwareVersion(uint8_t* firmware_blob) {
  uint16_t firmware_key_version;
  uint16_t firmware_version;
  uint16_t firmware_sign_algorithm;
  int firmware_sign_key_len;
  Memcpy(&firmware_sign_algorithm,
         firmware_blob + (FIELD_LEN(magic) +  /* Offset to field. */
                          FIELD_LEN(header_len)),
         sizeof(firmware_sign_algorithm));
  Memcpy(&firmware_key_version,
         firmware_blob + (FIELD_LEN(magic) +  /* Offset to field. */
                          FIELD_LEN(header_len) +
                          FIELD_LEN(firmware_sign_algorithm)),
         sizeof(firmware_key_version));
  if (firmware_sign_algorithm >= kNumAlgorithms)
    return 0;
  firmware_sign_key_len = RSAProcessedKeySize(firmware_sign_algorithm);
  Memcpy(&firmware_version,
         firmware_blob +  (FIELD_LEN(magic) +  /* Offset to field. */
                           FIELD_LEN(header_len) +
                           FIELD_LEN(firmware_key_version) +
                           firmware_sign_key_len +
                           FIELD_LEN(header_checksum) +
                           FIELD_LEN(firmware_key_signature)),
         sizeof(firmware_version));
  return CombineUint16Pair(firmware_key_version, firmware_version);
}

int VerifyFirmwareDriver_f(uint8_t* root_key_blob,
                           uint8_t* firmwareA,
                           uint8_t* firmwareB) {
  /* Contains the logical firmware version (32-bit) which is calculated as
   * (firmware_key_version << 16 | firmware_version) where
   * [firmware_key_version] [firmware_version] are both 16-bit.
   */
  uint32_t firmwareA_lversion, firmwareB_lversion;
  uint8_t firmwareA_is_verified = 0;  /* Whether firmwareA verify succeeded. */
  uint32_t min_lversion;  /* Minimum of firmware A and firmware lversion. */
  uint32_t stored_lversion;  /* Stored logical version in the TPM. */

  /* Initialize the TPM since we'll be reading the rollback indices. */
  SetupTPM();

  /* We get the key versions by reading directly from the image blobs without
   * any additional (expensive) sanity checking on the blob since it's faster to
   * outright reject a firmware with an older firmware key version. A malformed
   * or corrupted firmware blob will still fail when VerifyFirmware() is called
   * on it.
   */
  firmwareA_lversion = GetLogicalFirmwareVersion(firmwareA);
  firmwareB_lversion = GetLogicalFirmwareVersion(firmwareB);
  min_lversion  = Min(firmwareA_lversion, firmwareB_lversion);
  stored_lversion = CombineUint16Pair(GetStoredVersion(FIRMWARE_KEY_VERSION),
                                      GetStoredVersion(FIRMWARE_VERSION));
  /* Always try FirmwareA first. */
  if (VERIFY_FIRMWARE_SUCCESS == VerifyFirmware(root_key_blob, firmwareA,
                                                0))
    firmwareA_is_verified = 1;
  if (firmwareA_is_verified && (stored_lversion < firmwareA_lversion)) {
    /* Stored version may need to be updated but only if FirmwareB
     * is successfully verified and has a logical version greater than
     * the stored logical version. */
    if (VERIFY_FIRMWARE_SUCCESS == VerifyFirmware(root_key_blob, firmwareB,
                                                    0)) {
        if (stored_lversion < firmwareB_lversion) {
          WriteStoredVersion(FIRMWARE_KEY_VERSION,
                             (uint16_t) (min_lversion >> 16));
          WriteStoredVersion(FIRMWARE_VERSION,
                             (uint16_t) (min_lversion & 0x00FFFF));
          stored_lversion = min_lversion;  /* Update stored version as it's used
                                            * later. */
        }
    }
  }
  /* Lock Firmware TPM rollback indices from further writes. */
  /* TODO(gauravsh): Figure out if these can be combined into one
   * 32-bit location since we seem to always use them together. This can help
   * us minimize the number of NVRAM writes/locks (which are limited over flash
   * memory lifetimes.
   */
  LockStoredVersion(FIRMWARE_KEY_VERSION);
  LockStoredVersion(FIRMWARE_VERSION);

  /* Determine which firmware (if any) to jump to.
   *
   * We always attempt to jump to FirmwareA first. If verification of FirmwareA
   * fails, we try FirmwareB. In all cases, if the firmware successfully
   * verified but is a rollback, we jump to recovery.
   *
   * Note: This means that if FirmwareA verified successfully and is a
   * rollback, then no attempt is made to check FirmwareB. We still jump to
   * recovery. FirmwareB is only used as a backup in case FirmwareA gets
   * corrupted. Since newer firmware updates are always written to A,
   * the case where firmware A is verified but a rollback should not occur in
   * normal operation.
   */
  if (firmwareA_is_verified) {
    if (stored_lversion <= firmwareA_lversion)
      return BOOT_FIRMWARE_A_CONTINUE;
  } else {
    /* If FirmwareA was not valid, then we skipped over the
     * check to update the rollback indices and a Verify of FirmwareB wasn't
     * attempted.
     * If FirmwareB is not a rollback, then we attempt to do the verification.
     */
    if (stored_lversion <= firmwareB_lversion &&
        (VERIFY_FIRMWARE_SUCCESS == VerifyFirmware(root_key_blob, firmwareB,
                                                   0)))
      return BOOT_FIRMWARE_B_CONTINUE;
  }
  /* D'oh: No bootable firmware. */
  return BOOT_FIRMWARE_RECOVERY_CONTINUE;
}
