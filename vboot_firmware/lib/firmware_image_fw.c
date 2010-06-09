/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for verifying a verified boot firmware image.
 * (Firmware Portion)
 */

#include "firmware_image_fw.h"

#include "cryptolib.h"
#include "rollback_index.h"
#include "tss_constants.h"
#include "utility.h"

/* Macro to determine the size of a field structure in the FirmwareImage
 * structure. */
#define FIELD_LEN(field) (sizeof(((FirmwareImage*)0)->field))

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

uint64_t GetFirmwarePreambleLen(int algorithm) {
  return (FIELD_LEN(firmware_version) +
          FIELD_LEN(firmware_len) +
          FIELD_LEN(kernel_subkey_sign_algorithm) +
          RSAProcessedKeySize(algorithm) +
          FIELD_LEN(preamble));
}


int VerifyFirmwareHeader(const uint8_t* root_key_blob,
                         const uint8_t* header_blob,
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
                           int firmware_sign_algorithm,
                           uint64_t* firmware_len) {
  uint64_t len;
  int preamble_len;
  uint16_t firmware_version;
  uint16_t kernel_subkey_sign_algorithm;

  Memcpy(&firmware_version, preamble_blob, sizeof(firmware_version));
  Memcpy(&kernel_subkey_sign_algorithm,
         preamble_blob + (FIELD_LEN(firmware_version) +
                          FIELD_LEN(firmware_len)),
         FIELD_LEN(kernel_subkey_sign_algorithm));
  preamble_len = GetFirmwarePreambleLen(kernel_subkey_sign_algorithm);
  if (!RSAVerifyBinary_f(NULL, firmware_sign_key,  /* Key to use */
                         preamble_blob,  /* Data to verify */
                         preamble_len,  /* Length of data */
                         preamble_blob + preamble_len,  /* Expected Signature */
                         firmware_sign_algorithm))
    return VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED;

  Memcpy(&len, preamble_blob + FIELD_LEN(firmware_version),
         sizeof(len));
  *firmware_len = len;
  return 0;
}

int VerifyFirmwareData(RSAPublicKey* firmware_sign_key,
                       const uint8_t* preamble_start,
                       const uint8_t* firmware_data,
                       uint64_t firmware_len,
                       int firmware_sign_algorithm) {
  int signature_len = siglen_map[firmware_sign_algorithm];
  int preamble_len;
  uint16_t kernel_subkey_sign_algorithm;
  uint8_t* digest = NULL;
  const uint8_t* firmware_signature = NULL;
  DigestContext ctx;
  Memcpy(&kernel_subkey_sign_algorithm,
         preamble_start + (FIELD_LEN(firmware_version) +
                          FIELD_LEN(firmware_len)),
         FIELD_LEN(kernel_subkey_sign_algorithm));
  preamble_len = GetFirmwarePreambleLen(kernel_subkey_sign_algorithm);


  /* Since the firmware signature is over the preamble and the firmware data,
   * which does not form a contiguous region of memory, we calculate the
   * message digest ourselves. */
  DigestInit(&ctx, firmware_sign_algorithm);
  DigestUpdate(&ctx, preamble_start, preamble_len);
  DigestUpdate(&ctx, firmware_data, firmware_len);
  digest = DigestFinal(&ctx);
  /* Firmware signature is at the end of preamble and preamble signature. */
  firmware_signature = preamble_start + preamble_len + signature_len;
  if (!RSAVerifyBinaryWithDigest_f(
          NULL, firmware_sign_key,  /* Key to use. */
          digest,  /* Digest of the data to verify. */
          firmware_signature,  /* Expected Signature */
          firmware_sign_algorithm)) {
    Free(digest);
    return VERIFY_FIRMWARE_SIGNATURE_FAILED;
  }
  Free(digest);
  return 0;
}

int VerifyFirmware(const uint8_t* root_key_blob,
                   const uint8_t* verification_header_blob,
                   const uint8_t* firmware_blob) {
  int error_code = 0;
  int firmware_sign_algorithm;  /* Signing key algorithm. */
  RSAPublicKey* firmware_sign_key = NULL;
  int firmware_sign_key_len, signature_len, header_len;
  uint64_t firmware_len;
  const uint8_t* header_ptr = NULL;  /* Pointer to header. */
  const uint8_t* firmware_sign_key_ptr = NULL;  /* Pointer to signing key. */
  const uint8_t* preamble_ptr = NULL;  /* Pointer to preamble block. */

  /* Note: All the offset calculations are based on struct FirmwareImage which
   * is defined in include/firmware_image_fw.h. */

  /* Compare magic bytes. */
  if (SafeMemcmp(verification_header_blob, FIRMWARE_MAGIC,
                 FIRMWARE_MAGIC_SIZE)) {
    debug("Wrong Firmware Magic.\n");
    return VERIFY_FIRMWARE_WRONG_MAGIC;
  }
  header_ptr = verification_header_blob + FIRMWARE_MAGIC_SIZE;

  /* Only continue if header verification succeeds. */
  if ((error_code = VerifyFirmwareHeader(root_key_blob, header_ptr,
                                         &firmware_sign_algorithm,
                                         &header_len))) {
    debug("Couldn't verify Firmware header.\n");
    return error_code;  /* AKA jump to revovery. */
  }
  /* Parse signing key into RSAPublicKey structure since it is required multiple
   * times. */
  firmware_sign_key_len = RSAProcessedKeySize(firmware_sign_algorithm);
  firmware_sign_key_ptr = header_ptr + (FIELD_LEN(header_len) +
                                        FIELD_LEN(firmware_sign_algorithm) +
                                        FIELD_LEN(firmware_key_version));
  firmware_sign_key = RSAPublicKeyFromBuf(firmware_sign_key_ptr,
                                          firmware_sign_key_len);
  signature_len = siglen_map[firmware_sign_algorithm];

  /* Only continue if preamble verification succeeds. */
  preamble_ptr = (header_ptr + header_len +
                  FIELD_LEN(firmware_key_signature));
  if ((error_code = VerifyFirmwarePreamble(firmware_sign_key, preamble_ptr,
                                           firmware_sign_algorithm,
                                           &firmware_len))) {
    RSAPublicKeyFree(firmware_sign_key);
    debug("Couldn't verify Firmware preamble.\n");
    return error_code;  /* AKA jump to recovery. */
  }

  if ((error_code = VerifyFirmwareData(firmware_sign_key, preamble_ptr,
                                       firmware_blob,
                                       firmware_len,
                                       firmware_sign_algorithm))) {
    RSAPublicKeyFree(firmware_sign_key);
    debug("Couldn't verify Firmware data.\n");
    return error_code;  /* AKA jump to recovery. */
  }

  RSAPublicKeyFree(firmware_sign_key);
  return VERIFY_FIRMWARE_SUCCESS;  /* Success! */
}

uint32_t GetLogicalFirmwareVersion(uint8_t* verification_header_blob) {
  uint16_t firmware_key_version;
  uint16_t firmware_version;
  uint16_t firmware_sign_algorithm;
  int firmware_sign_key_len;
  Memcpy(&firmware_sign_algorithm,
         verification_header_blob + (FIELD_LEN(magic) +  /* Offset to field. */
                                      FIELD_LEN(header_len)),
         sizeof(firmware_sign_algorithm));
  Memcpy(&firmware_key_version,
         verification_header_blob + (FIELD_LEN(magic) +  /* Offset to field. */
                                     FIELD_LEN(header_len) +
                                     FIELD_LEN(firmware_sign_algorithm)),
         sizeof(firmware_key_version));
  if (firmware_sign_algorithm >= kNumAlgorithms)
    return 0;
  firmware_sign_key_len = RSAProcessedKeySize(firmware_sign_algorithm);
  Memcpy(&firmware_version,
         verification_header_blob +  (FIELD_LEN(magic) +  /* Offset to field. */
                                      FIELD_LEN(header_len) +
                                      FIELD_LEN(firmware_sign_algorithm) +
                                      FIELD_LEN(firmware_key_version) +
                                      firmware_sign_key_len +
                                      FIELD_LEN(header_checksum) +
                                      FIELD_LEN(firmware_key_signature)),
         sizeof(firmware_version));
  return CombineUint16Pair(firmware_key_version, firmware_version);
}

int VerifyFirmwareDriver_f(uint8_t* root_key_blob,
                           uint8_t* verification_headerA,
                           uint8_t* firmwareA,
                           uint8_t* verification_headerB,
                           uint8_t* firmwareB) {
  /* Contains the logical firmware version (32-bit) which is calculated as
   * (firmware_key_version << 16 | firmware_version) where
   * [firmware_key_version] [firmware_version] are both 16-bit.
   */
  uint32_t firmwareA_lversion, firmwareB_lversion;
  uint8_t firmwareA_is_verified = 0;  /* Whether firmwareA verify succeeded. */
  uint32_t min_lversion;  /* Minimum of firmware A and firmware lversion. */
  uint32_t stored_lversion;  /* Stored logical version in the TPM. */
  uint16_t version, key_version;  /* Temporary variables */

  /* Initialize the TPM since we'll be reading the rollback indices. */
  SetupTPM();

  /* We get the key versions by reading directly from the image blobs without
   * any additional (expensive) sanity checking on the blob since it's faster to
   * outright reject a firmware with an older firmware key version. A malformed
   * or corrupted firmware blob will still fail when VerifyFirmware() is called
   * on it.
   */
  firmwareA_lversion = GetLogicalFirmwareVersion(verification_headerA);
  firmwareB_lversion = GetLogicalFirmwareVersion(verification_headerB);
  min_lversion  = Min(firmwareA_lversion, firmwareB_lversion);
  GetStoredVersions(FIRMWARE_VERSIONS, &key_version, &version);
  stored_lversion = CombineUint16Pair(key_version, version);
  /* Always try FirmwareA first. */
  if (VERIFY_FIRMWARE_SUCCESS == VerifyFirmware(root_key_blob,
                                                verification_headerA,
                                                firmwareA))
    firmwareA_is_verified = 1;
  if (firmwareA_is_verified && (stored_lversion < firmwareA_lversion)) {
    /* Stored version may need to be updated but only if FirmwareB
     * is successfully verified and has a logical version greater than
     * the stored logical version. */
    if (stored_lversion < firmwareB_lversion) {
      if (VERIFY_FIRMWARE_SUCCESS == VerifyFirmware(root_key_blob,
                                                    verification_headerB,
                                                    firmwareB)) {
        WriteStoredVersions(FIRMWARE_VERSIONS,
                            (uint16_t) (min_lversion >> 16),
                            (uint16_t) (min_lversion & 0xFFFF));
        stored_lversion = min_lversion;  /* Update stored version as it's used
                                          * later. */
      }
    }
  }
  /* Lock Firmware TPM rollback indices from further writes.  In this design,
   * this is done by setting the globalLock bit, which is cleared only by
   * TPM_Init at reboot.
   */
  if (TPM_SUCCESS != LockFirmwareVersions()) {
    return VERIFY_FIRMWARE_TPM_ERROR;
  }

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
        (VERIFY_FIRMWARE_SUCCESS == VerifyFirmware(root_key_blob,
                                                   verification_headerB,
                                                   firmwareB)))
        return BOOT_FIRMWARE_B_CONTINUE;
  }
  /* D'oh: No bootable firmware. */
  return BOOT_FIRMWARE_RECOVERY_CONTINUE;
}
