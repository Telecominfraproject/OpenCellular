/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for generating and manipulating a verified boot firmware image.
 */

#include "firmware_image.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "signature_digest.h"
#include "stateful_util.h"

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
  st.overrun = 0;

  /* Read and compare magic bytes. */
  StatefulMemcpy(&st, &image->magic, FIRMWARE_MAGIC_SIZE);
  if (SafeMemcmp(image->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE)) {
    debug("Wrong Firmware Magic.\n");
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
    debug("Header length mismatch. Got: %d Expected: %d\n",
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
    debug("Invalid firmware header checksum!\n");
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

  if(st.overrun || st.remaining_len != 0) {  /* Overrun or underrun. */
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
  st.overrun = 0;

  StatefulMemcpy_r(&st, &image->header_len, FIELD_LEN(header_len));
  StatefulMemcpy_r(&st, &image->firmware_sign_algorithm, FIELD_LEN(header_len));
  StatefulMemcpy_r(&st, &image->firmware_key_version,
                 FIELD_LEN(firmware_key_version));
  StatefulMemcpy_r(&st, image->firmware_sign_key,
                 RSAProcessedKeySize(image->firmware_sign_algorithm));
  StatefulMemcpy_r(&st, &image->header_checksum, FIELD_LEN(header_checksum));

  if (st.overrun || st.remaining_len != 0) {  /* Underrun or Overrun. */
    Free(header_blob);
    return NULL;
  }
  return header_blob;
}

int GetFirmwarePreambleLen(void) {
  return (FIELD_LEN(firmware_version) + FIELD_LEN(firmware_len) +
          FIELD_LEN(preamble));
}

uint8_t* GetFirmwarePreambleBlob(const FirmwareImage* image) {
  uint8_t* preamble_blob = NULL;
  MemcpyState st;

  preamble_blob = (uint8_t*) Malloc(GetFirmwarePreambleLen());
  st.remaining_len = GetFirmwarePreambleLen();
  st.remaining_buf = preamble_blob;
  st.overrun = 0;

  StatefulMemcpy_r(&st, &image->firmware_version, FIELD_LEN(firmware_version));
  StatefulMemcpy_r(&st, &image->firmware_len, FIELD_LEN(firmware_len));
  StatefulMemcpy_r(&st, image->preamble, FIELD_LEN(preamble));

  if (st.overrun || st.remaining_len != 0 ) {  /* Underrun or Overrun. */
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
               GetFirmwarePreambleLen() +
               2 * firmware_signature_len +
               image->firmware_len);
  firmware_blob = (uint8_t*) Malloc(*blob_len);
  st.remaining_len = *blob_len;
  st.remaining_buf = firmware_blob;
  st.overrun = 0;

  header_blob = GetFirmwareHeaderBlob(image);
  preamble_blob = GetFirmwarePreambleBlob(image);

  StatefulMemcpy_r(&st, image->magic, FIELD_LEN(magic));
  StatefulMemcpy_r(&st, header_blob, GetFirmwareHeaderLen(image));
  StatefulMemcpy_r(&st, image->firmware_key_signature,
                   FIELD_LEN(firmware_key_signature));
  StatefulMemcpy_r(&st, preamble_blob, GetFirmwarePreambleLen());
  StatefulMemcpy_r(&st, image->preamble_signature, firmware_signature_len);
  StatefulMemcpy_r(&st, image->firmware_signature, firmware_signature_len);
  StatefulMemcpy_r(&st, image->firmware_data, image->firmware_len);

  Free(preamble_blob);
  Free(header_blob);

  if (st.overrun || st.remaining_len != 0) { /* Underrun or Overrun. */
    Free(firmware_blob);
    return NULL;
  }
  return firmware_blob;
}

int WriteFirmwareImage(const char* output_file,
                       const FirmwareImage* image,
                       int is_only_vblock,
                       int is_subkey_out) {
  int fd;
  int success = 1;
  uint8_t* firmware_blob;
  uint8_t* subkey_out_buf = NULL;
  uint8_t* subkey_header = NULL;
  uint64_t blob_len;

  if (!image)
    return 0;
  if (-1 == (fd = creat(output_file, 0666))) {
    debug("Couldn't open file for writing.\n");
    return 0;
  }
  if (is_subkey_out) {
    blob_len = GetFirmwareHeaderLen(image) +
        siglen_map[ROOT_SIGNATURE_ALGORITHM];
    subkey_out_buf = (uint8_t*) Malloc(blob_len);
    subkey_header = GetFirmwareHeaderBlob(image);
    Memcpy(subkey_out_buf, subkey_header, GetFirmwareHeaderLen(image));
    Memcpy(subkey_out_buf + GetFirmwareHeaderLen(image),
           image->firmware_key_signature,
           siglen_map[ROOT_SIGNATURE_ALGORITHM]);
    if (blob_len != write(fd, subkey_out_buf, blob_len)) {
      debug("Couldn't write kernel subkey header to file: %s\n",
            output_file);
      success = 0;
    }
    Free(subkey_header);
    Free(subkey_out_buf);
    close(fd);
    return success;
  }

  firmware_blob = GetFirmwareBlob(image, &blob_len);
  if (!firmware_blob) {
    debug("Couldn't create firmware blob from FirmwareImage.\n");
    return 0;
  }
  if (!is_only_vblock) {
    if (blob_len != write(fd, firmware_blob, blob_len)) {
      debug("Couldn't write Firmware Image to file: %s\n", output_file);
      success = 0;
    }
  } else {
    /* Exclude the firmware_data. */
    int vblock_len = blob_len - image->firmware_len;
    if (vblock_len != write(fd, firmware_blob, vblock_len)) {
      debug("Couldn't write Firmware Image verifcation block to file: %s\n",
            output_file);
      success = 0;
    }
  }
  Free(firmware_blob);
  close(fd);
  return success;
}

void PrintFirmwareImage(const FirmwareImage* image) {
  if (!image)
    return;

  /* Print header. */
  debug("Header Length = %d\n"
         "Firmware Signature Algorithm = %s\n"
         "Firmware Key Version = %d\n\n",
         image->header_len,
         algo_strings[image->firmware_sign_algorithm],
         image->firmware_key_version);
  /* TODO(gauravsh): Output hash and key signature here? */
  /* Print preamble. */
  debug("Firmware Version = %d\n"
         "Firmware Length = %" PRIu64 "\n\n",
         image->firmware_version,
         image->firmware_len);
  /* Output key signature here? */
}

int VerifyFirmwareImage(const RSAPublicKey* root_key,
                        const FirmwareImage* image) {
  RSAPublicKey* firmware_sign_key = NULL;
  uint8_t* header_digest = NULL;
  uint8_t* preamble_digest = NULL;
  uint8_t* firmware_digest = NULL;
  int firmware_sign_key_size;
  int signature_size;
  int error_code = 0;
  DigestContext ctx;
  DigestContext firmware_ctx;

  if (!image)
    return VERIFY_FIRMWARE_INVALID_IMAGE;

  /* Verify root key signature on the sign key header if we
   * are not in dev mode.
   *
   * TODO(gauravsh): Add additional sanity checks here for:
   *  1) verifying the header length is correct.
   *  2) header_checksum is correct.
   */

  /* Check key signature. */
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

  /* Verify firmware signature - firmware signature is on the contents
   of firmware preamble + firmware_data. */
  DigestInit(&firmware_ctx, image->firmware_sign_algorithm);
  DigestUpdate(&firmware_ctx, (uint8_t*) &image->firmware_version,
               FIELD_LEN(firmware_version));
  DigestUpdate(&firmware_ctx, (uint8_t*) &image->firmware_len,
               FIELD_LEN(firmware_len));
  DigestUpdate(&firmware_ctx, (uint8_t*) &image->preamble,
               FIELD_LEN(preamble));
  DigestUpdate(&firmware_ctx, image->firmware_data, image->firmware_len);
  firmware_digest = DigestFinal(&firmware_ctx);
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
  uint8_t* preamble_blob = NULL;
  uint8_t* preamble_signature = NULL;
  uint8_t* firmware_signature = NULL;
  uint8_t* firmware_buf = NULL;
  int signature_len = siglen_map[image->firmware_sign_algorithm];

  preamble_blob = GetFirmwarePreambleBlob(image);
  if (!preamble_blob)
    return 0;
  if (!(preamble_signature = SignatureBuf(preamble_blob,
                                          GetFirmwarePreambleLen(),
                                          signing_key_file,
                                          image->firmware_sign_algorithm))) {
    Free(preamble_blob);
    return 0;
  }
  image->preamble_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->preamble_signature, preamble_signature, signature_len);
  Free(preamble_signature);
  /* Firmware signature must be calculated on preamble + firmware_data
   * to avoid splicing attacks. */
  firmware_buf = (uint8_t*) Malloc(GetFirmwarePreambleLen() +
                                   image->firmware_len);
  Memcpy(firmware_buf, preamble_blob, GetFirmwarePreambleLen());
  Memcpy(firmware_buf + GetFirmwarePreambleLen(), image->firmware_data,
         image->firmware_len);
  if (!(firmware_signature = SignatureBuf(firmware_buf,
                                          GetFirmwarePreambleLen() +
                                          image->firmware_len,
                                          signing_key_file,
                                          image->firmware_sign_algorithm))) {
    Free(preamble_blob);
    Free(firmware_buf);
    return 0;
  }
  image->firmware_signature = (uint8_t*) Malloc(signature_len);
  Memcpy(image->firmware_signature, firmware_signature, signature_len);
  Free(firmware_signature);
  Free(firmware_buf);
  Free(preamble_blob);
  return 1;
}
