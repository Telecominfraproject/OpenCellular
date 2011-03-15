/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

/* Static variables for UpdateFirmwareBodyHash().  It's less than
 * optimal to have static variables in a library, but in UEFI the
 * caller is deep inside a different firmware stack and doesn't have a
 * good way to pass the params struct back to us. */
typedef struct VbLoadFirmwareInternal {
  DigestContext body_digest_context;
  uint64_t body_size_accum;
} VbLoadFirmwareInternal;


void UpdateFirmwareBodyHash(LoadFirmwareParams* params,
                             uint8_t* data, uint64_t size) {
  VbLoadFirmwareInternal* lfi =
      (VbLoadFirmwareInternal*)params->load_firmware_internal;

  DigestUpdate(&lfi->body_digest_context, data, size);
  lfi->body_size_accum += size;
}


int LoadFirmwareSetup(void) {
  /* TODO: handle test errors (requires passing in VbNvContext) */
  /* TODO: start initializing the TPM */
  return LOAD_FIRMWARE_SUCCESS;
}


int LoadFirmware(LoadFirmwareParams* params) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)params->shared_data_blob;
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)params->gbb_data;
  VbPublicKey* root_key;
  VbLoadFirmwareInternal* lfi;
  VbNvContext* vnc = params->nv_context;

  uint32_t try_b_count;
  uint32_t tpm_version = 0;
  uint64_t lowest_version = 0xFFFFFFFF;
  uint32_t status;
  uint32_t test_err = 0;
  int good_index = -1;
  int is_dev;
  int index;
  int i;

  int retval = LOAD_FIRMWARE_RECOVERY;
  int recovery = VBNV_RECOVERY_RO_UNSPECIFIED;

  /* Clear output params in case we fail */
  params->firmware_index = 0;

  VBDEBUG(("LoadFirmware started...\n"));

  /* Setup NV storage */
  VbNvSetup(vnc);

  /* Initialize shared data structure. */
  if (0 != VbSharedDataInit(shared, params->shared_data_size)) {
    VBDEBUG(("Shared data init error\n"));
    recovery = VBNV_RECOVERY_RO_SHARED_DATA;
    goto LoadFirmwareExit;
  }

  /* Handle test errors */
  VbNvGet(vnc, VBNV_TEST_ERROR_FUNC, &test_err);
  if (VBNV_TEST_ERROR_LOAD_FIRMWARE == test_err) {
    /* Get error code */
    VbNvGet(vnc, VBNV_TEST_ERROR_NUM, &test_err);
    /* Clear test params so we don't repeat the error */
    VbNvSet(vnc, VBNV_TEST_ERROR_FUNC, 0);
    VbNvSet(vnc, VBNV_TEST_ERROR_NUM, 0);
    /* Handle error codes */
    switch (test_err) {
      case LOAD_FIRMWARE_RECOVERY:
        recovery = VBNV_RECOVERY_RO_TEST_LF;
        goto LoadFirmwareExit;
      case LOAD_FIRMWARE_REBOOT:
        retval = test_err;
        goto LoadFirmwareExit;
      default:
        break;
    }
  }

  /* Must have a root key from the GBB */
  if (!gbb) {
    VBDEBUG(("No GBB\n"));
    goto LoadFirmwareExit;
  }
  root_key = (VbPublicKey*)((uint8_t*)gbb + gbb->rootkey_offset);

  /* Parse flags */
  is_dev = (params->boot_flags & BOOT_FLAG_DEVELOPER ? 1 : 0);

  /* Initialize the TPM and read rollback indices. */
  VBPERFSTART("VB_TPMI");
  status = RollbackFirmwareSetup(is_dev, &tpm_version);
  if (0 != status) {
    VBDEBUG(("Unable to setup TPM and read stored versions.\n"));
    VBPERFEND("VB_TPMI");
    if (status == TPM_E_MUST_REBOOT)
      retval = LOAD_FIRMWARE_REBOOT;
    else
      recovery = VBNV_RECOVERY_RO_TPM_ERROR;
    goto LoadFirmwareExit;
  }
  VBPERFEND("VB_TPMI");

  /* Read try-b count and decrement if necessary */
  VbNvGet(vnc, VBNV_TRY_B_COUNT, &try_b_count);
  if (0 != try_b_count)
    VbNvSet(vnc, VBNV_TRY_B_COUNT, try_b_count - 1);
  VbNvSet(vnc, VBNV_TRIED_FIRMWARE_B, try_b_count ? 1 : 0);

  /* Allocate our internal data */
  lfi = (VbLoadFirmwareInternal*)Malloc(sizeof(VbLoadFirmwareInternal));
  if (!lfi)
    return LOAD_FIRMWARE_RECOVERY;

  params->load_firmware_internal = (uint8_t*)lfi;

  /* Loop over indices */
  for (i = 0; i < 2; i++) {
    VbKeyBlockHeader* key_block;
    uint64_t vblock_size;
    VbFirmwarePreambleHeader* preamble;
    RSAPublicKey* data_key;
    uint64_t key_version;
    uint64_t combined_version;
    uint8_t* body_digest;

    /* If try B count is non-zero try firmware B first */
    index = (try_b_count ? 1 - i : i);
    if (0 == index) {
      key_block = (VbKeyBlockHeader*)params->verification_block_0;
      vblock_size = params->verification_size_0;
    } else {
      key_block = (VbKeyBlockHeader*)params->verification_block_1;
      vblock_size = params->verification_size_1;
    }

    /* Check the key block flags against the current boot mode.  Do this
     * before verifying the key block, since flags are faster to check than
     * the RSA signature. */
    if (!(key_block->key_block_flags &
          (is_dev ? KEY_BLOCK_FLAG_DEVELOPER_1 :
           KEY_BLOCK_FLAG_DEVELOPER_0))) {
      VBDEBUG(("Developer flag mismatch.\n"));
      continue;
    }
    /* RW firmware never runs in recovery mode. */
    if (!(key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)) {
      VBDEBUG(("Recovery flag mismatch.\n"));
      continue;
    }

    /* Verify the key block */
    VBPERFSTART("VB_VKB");
    if ((0 != KeyBlockVerify(key_block, vblock_size, root_key, 0))) {
      VBDEBUG(("Key block verification failed.\n"));
      VBPERFEND("VB_VKB");
      continue;
    }
    VBPERFEND("VB_VKB");

    /* Check for rollback of key version. */
    key_version = key_block->data_key.key_version;
    if (key_version < (tpm_version >> 16)) {
      VBDEBUG(("Key rollback detected.\n"));
      continue;
    }

    /* Get the key for preamble/data verification from the key block. */
    data_key = PublicKeyToRSA(&key_block->data_key);
    if (!data_key) {
      VBDEBUG(("Unable to parse data key.\n"));
      continue;
    }

    /* Verify the preamble, which follows the key block. */
    VBPERFSTART("VB_VPB");
    preamble = (VbFirmwarePreambleHeader*)((uint8_t*)key_block +
                                           key_block->key_block_size);
    if ((0 != VerifyFirmwarePreamble(preamble,
                                     vblock_size - key_block->key_block_size,
                                     data_key))) {
      VBDEBUG(("Preamble verfication failed.\n"));
      RSAPublicKeyFree(data_key);
      VBPERFEND("VB_VPB");
      continue;
    }
    VBPERFEND("VB_VPB");

    /* Check for rollback of firmware version. */
    combined_version = ((key_version << 16) |
                        (preamble->firmware_version & 0xFFFF));
    if (combined_version < tpm_version) {
      VBDEBUG(("Firmware version rollback detected.\n"));
      RSAPublicKeyFree(data_key);
      continue;
    }

    /* Check for lowest key version from a valid header. */
    if (lowest_version > combined_version)
      lowest_version = combined_version;

    /* If we already have good firmware, no need to read another one;
     * we only needed to look at the versions to check for
     * rollback. */
    if (-1 != good_index)
      continue;

    /* Read the firmware data */
    VBPERFSTART("VB_RFD");
    DigestInit(&lfi->body_digest_context, data_key->algorithm);
    lfi->body_size_accum = 0;
    if (0 != GetFirmwareBody(params, index)) {
      VBDEBUG(("GetFirmwareBody() failed for index %d\n", index));
      RSAPublicKeyFree(data_key);
      VBPERFEND("VB_RFD");
      continue;
    }
    if (lfi->body_size_accum != preamble->body_signature.data_size) {
      VBDEBUG(("Hash updated %d bytes but expected %d\n",
               (int)lfi->body_size_accum,
               (int)preamble->body_signature.data_size));
      RSAPublicKeyFree(data_key);
      VBPERFEND("VB_RFD");
      continue;
    }
    VBPERFEND("VB_RFD");

    /* Verify firmware data */
    VBPERFSTART("VB_VFD");
    body_digest = DigestFinal(&lfi->body_digest_context);
    if (0 != VerifyDigest(body_digest, &preamble->body_signature, data_key)) {
      VBDEBUG(("Firmware body verification failed.\n"));
      RSAPublicKeyFree(data_key);
      Free(body_digest);
      VBPERFEND("VB_VFD");
      continue;
    }
    VBPERFEND("VB_VFD");

    /* Done with the digest and data key, so can free them now */
    RSAPublicKeyFree(data_key);
    Free(body_digest);

    /* If we're still here, the firmware is valid. */
    VBDEBUG(("Firmware %d is valid.\n", index));
    if (-1 == good_index) {
      /* Save the key we actually used */
      if (0 != VbSharedDataSetKernelKey(shared, &preamble->kernel_subkey)) {
        VBDEBUG(("Unable to save kernel subkey to shared data.\n"));
        continue;  /* The firmware signature was good, but the public
                    * key was bigger that the caller can handle. */
      }

      /* Save the good index, now that we're sure we can actually use
       * this firmware.  That's the one we'll boot. */
      good_index = index;
      params->firmware_index = index;

      /* If the good firmware's key version is the same as the tpm,
       * then the TPM doesn't need updating; we can stop now.
       * Otherwise, we'll check all the other headers to see if they
       * contain a newer key. */
      if (combined_version == tpm_version)
        break;
    }
  }

  /* Free internal data */
  Free(lfi);
  params->load_firmware_internal = NULL;

  /* Handle finding good firmware */
  if (good_index >= 0) {

    /* Update TPM if necessary */
    if (lowest_version > tpm_version) {
      VBPERFSTART("VB_TPMU");
      status = RollbackFirmwareWrite((uint32_t)lowest_version);
      VBPERFEND("VB_TPMU");
      if (0 != status) {
        VBDEBUG(("Unable to write stored versions.\n"));
        if (status == TPM_E_MUST_REBOOT)
          retval = LOAD_FIRMWARE_REBOOT;
        else
          recovery = VBNV_RECOVERY_RO_TPM_ERROR;
        goto LoadFirmwareExit;
      }
    }

    /* Lock firmware versions in TPM */
    VBPERFSTART("VB_TPML");
    status = RollbackFirmwareLock();
    VBPERFEND("VB_TPML");
    if (0 != status) {
      VBDEBUG(("Unable to lock firmware versions.\n"));
      if (status == TPM_E_MUST_REBOOT)
        retval = LOAD_FIRMWARE_REBOOT;
      else
        recovery = VBNV_RECOVERY_RO_TPM_ERROR;
      goto LoadFirmwareExit;
    }

    /* Success */
    VBDEBUG(("Will boot firmware index %d\n", (int)params->firmware_index));
    retval = LOAD_FIRMWARE_SUCCESS;
  } else {
    /* No good firmware, so go to recovery mode. */
    VBDEBUG(("Alas, no good firmware.\n"));
    recovery = VBNV_RECOVERY_RO_INVALID_RW;
  }

LoadFirmwareExit:
  /* Store recovery request, if any, then tear down non-volatile storage */
  VbNvSet(vnc, VBNV_RECOVERY_REQUEST, LOAD_FIRMWARE_RECOVERY == retval ?
          recovery : VBNV_RECOVERY_NOT_REQUESTED);
  VbNvTeardown(vnc);

  /* Note that we don't reduce params->shared_data_size to shared->data_used,
   * since we want to leave space for LoadKernel() to add to the shared data
   * buffer. */

  return retval;
}


int S3Resume(void) {

  /* TODO: handle test errors (requires passing in VbNvContext) */

  /* Resume the TPM */
  uint32_t status = RollbackS3Resume();

  /* If we can't resume, just do a full reboot.  No need to go to recovery
   * mode here, since if the TPM is really broken we'll catch it on the
   * next boot. */
  if (status == TPM_SUCCESS)
    return LOAD_FIRMWARE_SUCCESS;
  else
    return LOAD_FIRMWARE_REBOOT;
}
