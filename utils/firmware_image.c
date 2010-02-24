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
#include "rsa_utility.h"
#include "sha_utility.h"
#include "utility.h"

/* Macro to determine the size of a field structure in the FirmwareImage
 * structure. */
#define FIELD_LEN(field) (sizeof(((FirmwareImage*)0)->field))

FirmwareImage* FirmwareImageNew(void) {
  FirmwareImage* fw = (FirmwareImage*) Malloc(sizeof(FirmwareImage));
  return fw;
}

void FirmwareImageFree(FirmwareImage* image) {
  Free(image->sign_key);
  Free(image->key_signature);
  Free(image->preamble_signature);
  Free(image->firmware_signature);
  Free(image->firmware_data);
}

FirmwareImage* ReadFirmwareImage(const char* input_file,
                                 FirmwareImage* image) {
  int fd;
  struct stat fd_stat;

  int image_len = 0;  /* Total size of the firmware image. */
  int header_len = 0;
  int sign_key_len;
  int signature_len;
  uint8_t* firmware_buf;
  MemcpyState st;

  if (!image)
    return NULL;

  if (-1 == (fd = open(input_file, O_RDONLY))) {
    fprintf(stderr, "Couldn't open file for reading.\n");
    return NULL;
  }

  if (-1 == fstat(fd, &fd_stat)) {
    fprintf(stderr, "Couldn't stat file.\n");
    close(fd);
    return NULL;
  }

  firmware_buf = (uint8_t*) Malloc(fd_stat.st_size);
  image_len = fd_stat.st_size;

  /* Read entire file into a buffer. */
  if (image_len != read(fd, firmware_buf, image_len)) {
    fprintf(stderr, "Couldn't read file data.\n");
    close(fd);
    return NULL;
  }
  close(fd);

  st.remaining_len = image_len;
  st.remaining_buf = firmware_buf;

  /* Read and compare magic bytes. */
  if (!StatefulMemcpy(&st, &image->magic, FIRMWARE_MAGIC_SIZE))
    goto parse_failure;

  if (SafeMemcmp(image->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE)) {
    fprintf(stderr, "Wrong Firmware Magic.\n");
    goto parse_failure;
  }

  StatefulMemcpy(&st, &image->header_len, sizeof(image->header_len));
  StatefulMemcpy(&st, &image->sign_algorithm, sizeof(image->sign_algorithm));

  /* Valid Algorithm? */
  if (image->sign_algorithm > kNumAlgorithms)
    goto parse_failure;

  /* Compute size of pre-processed RSA public key and signature. */
  sign_key_len = (2*siglen_map[image->sign_algorithm]*sizeof(uint32_t)
                     + sizeof(uint32_t) + sizeof(int));
  signature_len = siglen_map[image->sign_algorithm] * sizeof(uint32_t);


  /* Check whether the header length is correct. */
  header_len = (sizeof(image->header_len) + sizeof(image->sign_algorithm) +
                sizeof(image->key_version) +
                sizeof(image->header_checksum));
  if (header_len != image->header_len) {
    fprintf(stderr, "Header length mismatch.");
    goto parse_failure;
  }

  /* Read pre-processed public half of the sign key. */
  image->sign_key = (uint8_t*) Malloc(sign_key_len);
  StatefulMemcpy(&st, image->sign_key, sign_key_len);
  StatefulMemcpy(&st, &image->key_version, sizeof(image->key_version));
  StatefulMemcpy(&st, image->header_checksum, sizeof(image->header_checksum));

  /* Read key signature. */
  StatefulMemcpy(&st, image->key_signature, sizeof(image->key_signature));

  /* Read the firmware preamble. */
  StatefulMemcpy(&st,&image->firmware_version, sizeof(image->firmware_version));
  StatefulMemcpy(&st, &image->firmware_len, sizeof(image->firmware_len));
  StatefulMemcpy(&st, image->preamble, sizeof(image->preamble));

  /* Read firmware preamble signature. */
  image->preamble_signature = (uint8_t*) Malloc(signature_len);
  StatefulMemcpy(&st, image->preamble_signature, signature_len);

  image->firmware_signature = (uint8_t*) Malloc(signature_len);
  StatefulMemcpy(&st, image->firmware_signature, signature_len);

  image->firmware_data = (uint8_t*) Malloc(image->firmware_len);
  StatefulMemcpy(&st, image->firmware_data, image->firmware_len);

  if(st.remaining_len != 0) /* Overrun or underrun. */
    goto parse_failure;

  Free(firmware_buf);
  return image;

parse_failure:
  Free(firmware_buf);
  return NULL;
}

void WriteFirmwareHeader(int fd, FirmwareImage* image) {
  int sign_key_len;
  write(fd, &image->header_len, sizeof(image->header_len));
  write(fd, &image->sign_algorithm, sizeof(image->header_len));
  sign_key_len = (image->header_len - sizeof(image->header_len) -
                     sizeof(image->sign_algorithm) -
                     sizeof(image->key_version) -
                     sizeof(image->header_checksum));
  write(fd, image->sign_key, sign_key_len);
  write(fd, &image->key_version, sizeof(image->key_version));
  write(fd, &image->header_checksum, sizeof(image->header_checksum));
}

void WriteFirmwarePreamble(int fd, FirmwareImage* image) {
  write(fd, &image->firmware_version,
        sizeof(image->firmware_version));
  write(fd, &image->firmware_len, sizeof(image->firmware_len));
  write(fd, image->preamble, sizeof(image->preamble));
}

FirmwareImage* WriteFirmwareImage(const char* input_file,
                                  FirmwareImage* image) {
  int fd;
  int signature_len;

  if (!image)
    return NULL;
  if (-1 == (fd = creat(input_file, S_IRWXU))) {
    fprintf(stderr, "Couldn't open file for writing.\n");
    return NULL;
  }

  write(fd, image->magic, sizeof(image->magic));
  WriteFirmwareHeader(fd, image);
  write(fd, image->key_signature, sizeof(image->key_signature));
  signature_len = siglen_map[image->sign_algorithm] * sizeof(uint32_t);
  WriteFirmwarePreamble(fd, image);
  write(fd, image->preamble_signature, signature_len);
  write(fd, image->firmware_signature, signature_len);
  write(fd, image->firmware_data, image->firmware_len);

  close(fd);
  return image;
}

void PrintFirmware(const FirmwareImage* image) {
  if (!image)
    return;

  /* Print header. */
  printf("Header Length = %d\n"
         "Algorithm Id = %d\n"
         "Signature Algorithm = %s\n"
         "Key Version = %d\n\n",
         image->header_len,
         image->sign_algorithm,
         algo_strings[image->sign_algorithm],
         image->key_version);
  /* TODO(gauravsh): Output hash and key signature here? */
  /* Print preamble. */
  printf("Firmware Version = %d\n"
         "Firmware Length = %d\n\n",
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
};

int VerifyFirmwareHeader(const uint8_t* root_key_blob,
                         const uint8_t* header_blob,
                         const int dev_mode,
                         int* algorithm,
                         int* header_len) {
  int sign_key_len;
  int root_key_len;
  uint16_t hlen, algo;
  uint8_t* header_checksum = NULL;

  /* Base Offset for the header_checksum field. Actual offset is
   * this + sign_key_len. */
  int base_header_checksum_offset = (FIELD_LEN(header_len) +
                                     FIELD_LEN(sign_algorithm) +
                                     FIELD_LEN(key_version));


  root_key_len = RSAProcessedKeySize(ROOT_SIGNATURE_ALGORITHM);
  Memcpy(&hlen, header_blob, sizeof(hlen));
  Memcpy(&algo,
         header_blob + FIELD_LEN(sign_algorithm),
         sizeof(algo));
  if (algo >= kNumAlgorithms)
    return VERIFY_FIRMWARE_INVALID_ALGORITHM;
  *algorithm = (int) algo;
  sign_key_len = RSAProcessedKeySize(*algorithm);

  /* Verify if header len is correct? */
  if (hlen != (base_header_checksum_offset +
               sign_key_len +
               FIELD_LEN(header_checksum)))
    return VERIFY_FIRMWARE_INVALID_IMAGE;

  *header_len = (int) hlen;

  /* Verify if the hash of the header is correct. */
  header_checksum = DigestBuf(header_blob,
                              *header_len - FIELD_LEN(header_checksum),
                              SHA512_DIGEST_ALGORITHM);
  if (SafeMemcmp(header_checksum,
                  header_blob + (base_header_checksum_offset + sign_key_len),
                  FIELD_LEN(header_checksum))) {
    Free(header_checksum);
    return VERIFY_FIRMWARE_INVALID_IMAGE;
  }
  Free(header_checksum);

  /* Verify root key signature unless we are in dev mode. */
  if (!dev_mode) {
    if (!RSAVerifyBinary_f(root_key_blob, NULL,  /* Key to use */
                           header_blob,  /* Data to verify */
                           *header_len, /* Length of data */
                           header_blob + *header_len,  /* Expected Signature */
                           ROOT_SIGNATURE_ALGORITHM))
      return VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED;
  }
  return 0;
}

int VerifyFirmwarePreamble(RSAPublicKey* sign_key,
                           const uint8_t* preamble_blob,
                           int algorithm,
                           int* firmware_len) {
  uint32_t len;
  int preamble_len;
  preamble_len = (FIELD_LEN(firmware_version) +
                  FIELD_LEN(firmware_len) +
                  FIELD_LEN(preamble));
  if (!RSAVerifyBinary_f(NULL, sign_key,  /* Key to use */
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

int VerifyFirmwareData(RSAPublicKey* sign_key,
                       const uint8_t* firmware_data_start,
                       int firmware_len,
                       int algorithm) {
  int signature_len = siglen_map[algorithm] * sizeof(uint32_t);
  if (!RSAVerifyBinary_f(NULL, sign_key,  /* Key to use. */
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
  RSAPublicKey* sign_key;
  int sign_key_len, signature_len, header_len, firmware_len;
  const uint8_t* header_ptr;  /* Pointer to header. */
  const uint8_t* sign_key_ptr;  /* Pointer to signing key. */
  const uint8_t* preamble_ptr;  /* Pointer to preamble block. */
  const uint8_t* firmware_ptr;  /* Pointer to firmware signature/data. */

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
  sign_key_len = RSAProcessedKeySize(algorithm);
  sign_key_ptr = header_ptr + (FIELD_LEN(header_len) +
                               FIELD_LEN(sign_algorithm));
  sign_key = RSAPublicKeyFromBuf(sign_key_ptr, sign_key_len);
  signature_len = siglen_map[algorithm] * sizeof(uint32_t);

  /* Only continue if preamble verification succeeds. */
  preamble_ptr = (header_ptr + header_len +
                  FIELD_LEN(key_signature));
  if ((error_code = VerifyFirmwarePreamble(sign_key, preamble_ptr, algorithm,
                                           &firmware_len)))
    return error_code;  /* AKA jump to recovery. */

  /* Only continue if firmware data verification succeeds. */
  firmware_ptr = (preamble_ptr +
                  FIELD_LEN(firmware_version) +
                  FIELD_LEN(firmware_len) +
                  FIELD_LEN(preamble) +
                  signature_len);

  if ((error_code = VerifyFirmwareData(sign_key, firmware_ptr, firmware_len,
                                       algorithm)))
    return error_code;  /* AKA jump to recovery. */

  return 0;  /* Success! */
}

int VerifyFirmwareImage(const RSAPublicKey* root_key,
                        const FirmwareImage* image,
                        const int dev_mode) {
  RSAPublicKey* sign_key;
  uint8_t* header_digest = NULL;
  uint8_t* preamble_digest = NULL;
  uint8_t* firmware_digest = NULL;
  int sign_key_size;
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
  if (!dev_mode) {
    DigestInit(&ctx, ROOT_SIGNATURE_ALGORITHM);
    DigestUpdate(&ctx, (uint8_t*) &image->header_len,
                 sizeof(image->header_len));
    DigestUpdate(&ctx, (uint8_t*) &image->sign_algorithm,
                 sizeof(image->sign_algorithm));
    DigestUpdate(&ctx, image->sign_key,
                 RSAProcessedKeySize(image->sign_algorithm));
    DigestUpdate(&ctx, (uint8_t*) &image->key_version,
                 sizeof(image->key_version));
    DigestUpdate(&ctx, image->header_checksum,
                 sizeof(image->header_checksum));
    header_digest = DigestFinal(&ctx);
    if (!RSA_verify(root_key, image->key_signature,
                    sizeof(image->key_signature),
                    ROOT_SIGNATURE_ALGORITHM,
                    header_digest)) {
      error_code =  VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED;
      goto verify_failure;
    }
  }

  /* Get sign key to verify the rest of the firmware. */
  sign_key_size = RSAProcessedKeySize(image->sign_algorithm);
  sign_key = RSAPublicKeyFromBuf(image->sign_key,
                                 sign_key_size);
  signature_size = siglen_map[image->sign_algorithm] * sizeof(uint32_t);

  if (image->sign_algorithm >= kNumAlgorithms)
    return VERIFY_FIRMWARE_INVALID_ALGORITHM;

  /* Verify firmware preamble signature. */
  DigestInit(&ctx, image->sign_algorithm);
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_version,
               sizeof(image->firmware_version));
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_len,
               sizeof(image->firmware_len));
  DigestUpdate(&ctx, (uint8_t*) &image->preamble,
               sizeof(image->preamble));
  preamble_digest = DigestFinal(&ctx);
  if (!RSA_verify(sign_key, image->preamble_signature,
                  signature_size, image->sign_algorithm,
                  preamble_digest)) {
    error_code = VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED;
    goto verify_failure;
  }

  /* Verify firmware signature. */
  firmware_digest = DigestBuf(image->firmware_data,
                              image->firmware_len,
                              image->sign_algorithm);
  if(!RSA_verify(sign_key, image->firmware_signature,
                 signature_size, image->sign_algorithm,
                 firmware_digest)) {
    error_code = VERIFY_FIRMWARE_SIGNATURE_FAILED;
    goto verify_failure;
  }

verify_failure:
  Free(firmware_digest);
  Free(preamble_digest);
  Free(header_digest);
  return error_code;
}


int AddKeySignature(FirmwareImage* image, char* root_key_file) {
  int tmp_hdr_fd;
  char* tmp_hdr_file = ".tmpHdrFile";
  uint8_t* signature;

  if(-1 == (tmp_hdr_fd = creat(tmp_hdr_file, S_IRWXU))) {
    fprintf(stderr, "Could not open temporary file for writing "
            "firmware header.\n");
    return 0;
  }
  WriteFirmwareHeader(tmp_hdr_fd, image);
  close(tmp_hdr_fd);

  if (!(signature = SignatureFile(tmp_hdr_file, root_key_file,
                                  ROOT_SIGNATURE_ALGORITHM)))
    return 0;
  Memcpy(image->key_signature, signature, RSA8192NUMBYTES);
  return 1;
}

int AddFirmwareSignature(FirmwareImage* image, char* signing_key_file,
                         int algorithm) {
  int tmp_preamble_fd;
  char* tmp_preamble_file = ".tmpPreambleFile";
  int tmp_firmware_fd;
  char* tmp_firmware_file = ".tmpFirmwareFile";
  uint8_t* preamble_signature;
  uint8_t* firmware_signature;
  int signature_len = siglen_map[algorithm] * sizeof(uint32_t);

  /* Write preamble to a file. */
  if(-1 == (tmp_preamble_fd = creat(tmp_preamble_file, S_IRWXU))) {
    fprintf(stderr, "Could not open temporary file for writing "
            "firmware preamble.\n");
    return 0;
  }
  WriteFirmwarePreamble(tmp_preamble_fd, image);
  close(tmp_preamble_fd);
  if (!(preamble_signature = SignatureFile(tmp_preamble_file, signing_key_file,
                                           algorithm)))
    return 0;
  image->preamble_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->preamble_signature, preamble_signature, signature_len);
  Free(preamble_signature);

  if (-1 == (tmp_firmware_fd = creat(tmp_firmware_file, S_IRWXU))) {
    fprintf(stderr, "Could not open temporary file for writing "
            "firmware.\n");
    return 0;
  }
  write(tmp_firmware_fd, image->firmware_data, image->firmware_len);
  close(tmp_firmware_fd);

  if (!(firmware_signature = SignatureFile(tmp_firmware_file, signing_key_file,
                                           algorithm))) {
    fprintf(stderr, "Could not open temporary file for writing "
            "firmware.\n");
    return 0;
  }
  image->firmware_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->firmware_signature, firmware_signature, signature_len);
  Free(firmware_signature);
  return 1;
}
