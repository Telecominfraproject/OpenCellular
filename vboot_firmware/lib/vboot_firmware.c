/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "vboot_firmware.h"

#include "load_firmware_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_common.h"


void UpdateFirmwareBodyHash2(uint8_t* data, uint64_t size) {
  /* TODO: actually update the hash. */
}


int LoadFirmware2(LoadFirmwareParams* params) {

  VbPublicKey* root_key = (VbPublicKey*)params->firmware_root_key_blob;

  uint16_t tpm_key_version = 0;
  uint16_t tpm_fw_version = 0;
  uint64_t lowest_key_version = 0xFFFF;
  uint64_t lowest_fw_version = 0xFFFF;
  int good_index = -1;
  int index;

  /* Clear output params in case we fail */
  params->firmware_index = 0;
  params->kernel_sign_key_blob = NULL;
  params->kernel_sign_key_size = 0;

  /* Must have a root key */
  if (!root_key)
    return LOAD_FIRMWARE_RECOVERY;

  /* Initialize the TPM and read rollback indices. */
  if (0 != SetupTPM() )
    return LOAD_FIRMWARE_RECOVERY;
  if (0 != GetStoredVersions(FIRMWARE_VERSIONS,
                             &tpm_key_version, &tpm_fw_version))
    return LOAD_FIRMWARE_RECOVERY;

  /* Loop over indices */
  for (index = 0; index < 2; index++) {
    VbKeyBlockHeader* key_block;
    uint64_t vblock_size;
    VbFirmwarePreambleHeader* preamble;
    RSAPublicKey* data_key;
    uint64_t key_version;
    uint8_t* body_data;
    uint64_t body_size;

    /* Verify the key block */
    if (0 == index) {
      key_block = (VbKeyBlockHeader*)params->verification_block_0;
      vblock_size = params->verification_size_0;
    } else {
      key_block = (VbKeyBlockHeader*)params->verification_block_1;
      vblock_size = params->verification_size_1;
    }
    if ((0 != VerifyKeyBlock(key_block, vblock_size, root_key)))
      continue;

    /* Check for rollback of key version. */
    key_version = key_block->data_key.key_version;
    if (key_version < tpm_key_version)
      continue;

    /* Get the key for preamble/data verification from the key block. */
    data_key = PublicKeyToRSA(&key_block->data_key);
    if (!data_key)
      continue;

    /* Verify the preamble, which follows the key block. */
    preamble = (VbFirmwarePreambleHeader*)((uint8_t*)key_block +
                                           key_block->key_block_size);
    if ((0 != VerifyFirmwarePreamble2(preamble,
                                      vblock_size - key_block->key_block_size,
                                      data_key))) {
      RSAPublicKeyFree(data_key);
      continue;
    }

    /* Check for rollback of firmware version. */
    if (key_version == tpm_key_version &&
        preamble->firmware_version < tpm_fw_version) {
      RSAPublicKeyFree(data_key);
      continue;
    }

    /* Check for lowest key version from a valid header. */
    if (lowest_key_version > key_version) {
      lowest_key_version = key_version;
      lowest_fw_version = preamble->firmware_version;
    }
    else if (lowest_key_version == key_version &&
             lowest_fw_version > preamble->firmware_version) {
      lowest_fw_version = preamble->firmware_version;
    }

    /* If we already have good firmware, no need to read another one;
     * we only needed to look at the versions to check for
     * rollback. */
    if (-1 != good_index)
      continue;

    /* Read the firmware data */
    /* TODO: should set up hash for UpdateFirmwareBodyHash(). */
    body_data = GetFirmwareBody(index, &body_size);
    if (!body_data || (body_size != preamble->body_signature.data_size)) {
      RSAPublicKeyFree(data_key);
      continue;
    }

    /* Verify firmware data */
    /* TODO: should use hash from UpdateFirmwareBodyHash() rather than
     * recalculating it in VerifyData().  */
    if (0 != VerifyData(body_data, &preamble->body_signature, data_key)) {
      RSAPublicKeyFree(data_key);
      continue;
    }

    /* Done with the data key, so can free it now */
    RSAPublicKeyFree(data_key);

    /* If we're still here, the firmware is valid. */
    /* Save the first good firmware we find; that's the one we'll boot */
    if (-1 == good_index) {
      good_index = index;
      params->firmware_index = index;
      params->kernel_sign_key_blob = &preamble->kernel_subkey;
      params->kernel_sign_key_size = (preamble->kernel_subkey.key_offset +
                                      preamble->kernel_subkey.key_size);

      /* If the good firmware's key version is the same as the tpm,
       * then the TPM doesn't need updating; we can stop now.
       * Otherwise, we'll check all the other headers to see if they
       * contain a newer key. */
      if (key_version == tpm_key_version &&
          preamble->firmware_version == tpm_fw_version)
        break;
    }
  }

  /* Handle finding good firmware */
  if (good_index >= 0) {

    /* Update TPM if necessary */
    if ((lowest_key_version > tpm_key_version) ||
        (lowest_key_version == tpm_key_version &&
         lowest_fw_version > tpm_fw_version)) {
      if (0 != WriteStoredVersions(FIRMWARE_VERSIONS,
                                   lowest_key_version,
                                   lowest_fw_version))
        return LOAD_FIRMWARE_RECOVERY;
    }

    /* Lock Firmware TPM rollback indices from further writes.  In
     * this design, this is done by setting the globalLock bit, which
     * is cleared only by TPM_Init at reboot.  */
    if (0 != LockFirmwareVersions())
      return LOAD_FIRMWARE_RECOVERY;
  }

  /* If we're still here, no good firmware, so go to recovery mode. */
  return LOAD_FIRMWARE_RECOVERY;
}
