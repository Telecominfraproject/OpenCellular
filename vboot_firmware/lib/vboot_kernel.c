/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for loading a kernel from disk.
 * (Firmware portion)
 */

#include "vboot_kernel.h"

#include "boot_device.h"
#include "cgptlib.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "utility.h"

#define KBUF_SIZE 65536  /* Bytes to read at start of kernel partition */

int LoadKernel2(LoadKernelParams* params) {

  VbPublicKey* kernel_subkey = (VbPublicKey*)params->header_sign_key_blob;

  GptData gpt;
  uint64_t part_start, part_size;
  uint64_t blba = params->bytes_per_lba;
  uint64_t kbuf_sectors = KBUF_SIZE / blba;
  uint8_t* kbuf = NULL;
  int found_partitions = 0;
  int good_partition = -1;
  uint16_t tpm_key_version = 0;
  uint16_t tpm_kernel_version = 0;
  uint64_t lowest_key_version = 0xFFFF;
  uint64_t lowest_kernel_version = 0xFFFF;
  int is_dev = ((BOOT_FLAG_DEVELOPER & params->boot_flags) &&
                !(BOOT_FLAG_RECOVERY & params->boot_flags));
  int is_normal = (!(BOOT_FLAG_DEVELOPER & params->boot_flags) &&
                   !(BOOT_FLAG_RECOVERY & params->boot_flags));

  /* Clear output params in case we fail */
  params->partition_number = 0;
  params->bootloader_address = 0;
  params->bootloader_size = 0;

  if (is_normal) {
    /* Read current kernel key index from TPM.  Assumes TPM is already
     * initialized. */
    if (0 != GetStoredVersions(KERNEL_VERSIONS,
                               &tpm_key_version,
                               &tpm_kernel_version))
      return LOAD_KERNEL_RECOVERY;
  } else if (is_dev) {
    /* In developer mode, we ignore the kernel subkey, and just use
     * the SHA-512 hash to verify the key block. */
    kernel_subkey = NULL;
  }

  do {
    /* Read GPT data */
    gpt.sector_bytes = blba;
    gpt.drive_sectors = params->ending_lba + 1;
    if (0 != AllocAndReadGptData(&gpt))
      break;

    /* Initialize GPT library */
    if (GPT_SUCCESS != GptInit(&gpt))
      break;

    /* TODO: TERRIBLE KLUDGE - fake partition attributes */
    FakePartitionAttributes(&gpt);

    /* Allocate kernel header buffers */
    kbuf = (uint8_t*)Malloc(KBUF_SIZE);
    if (!kbuf)
      break;

    /* Loop over candidate kernel partitions */
    while (GPT_SUCCESS == GptNextKernelEntry(&gpt, &part_start, &part_size)) {
      VbKeyBlockHeader* key_block;
      VbKernelPreambleHeader* preamble;
      RSAPublicKey* data_key;
      uint64_t key_version;
      uint64_t body_offset;

      /* Found at least one kernel partition. */
      found_partitions++;

      /* Read the first part of the kernel partition  */
      if (part_size < kbuf_sectors)
        continue;
      if (0 != BootDeviceReadLBA(part_start, kbuf_sectors, kbuf))
        continue;

      /* Verify the key block */
      key_block = (VbKeyBlockHeader*)kbuf;
      if ((0 != VerifyKeyBlock(key_block, KBUF_SIZE, kernel_subkey)))
        continue;

      /* Check the key block flags against the current boot mode */
      if (!(key_block->key_block_flags &&
            ((BOOT_FLAG_DEVELOPER & params->boot_flags) ?
             KEY_BLOCK_FLAG_DEVELOPER_1 : KEY_BLOCK_FLAG_DEVELOPER_0)))
        continue;
      if (!(key_block->key_block_flags &&
            ((BOOT_FLAG_RECOVERY & params->boot_flags) ?
             KEY_BLOCK_FLAG_RECOVERY_1 : KEY_BLOCK_FLAG_RECOVERY_0)))
        continue;

      /* Check for rollback of key version.  Note this is implicitly
       * skipped in recovery and developer modes because those set
       * key_version=0 above. */
      key_version = key_block->data_key.key_version;
      if (key_version < tpm_key_version)
        continue;

      /* Get the key for preamble/data verification from the key block */
      data_key = PublicKeyToRSA(&key_block->data_key);
      if (!data_key)
        continue;

      /* Verify the preamble, which follows the key block */
      preamble = (VbKernelPreambleHeader*)(kbuf + key_block->key_block_size);
      if ((0 != VerifyKernelPreamble2(preamble,
                                     KBUF_SIZE - key_block->key_block_size,
                                     data_key))) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Check for rollback of kernel version.  Note this is implicitly
       * skipped in recovery and developer modes because those set
       * key_version=0 and kernel_version=0 above. */
      if (key_version == tpm_key_version &&
          preamble->kernel_version < tpm_kernel_version) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Check for lowest key version from a valid header. */
      if (lowest_key_version > key_version) {
        lowest_key_version = key_version;
        lowest_kernel_version = preamble->kernel_version;
      }
      else if (lowest_key_version == key_version &&
               lowest_kernel_version > preamble->kernel_version) {
        lowest_kernel_version = preamble->kernel_version;
      }

      /* If we already have a good kernel, no need to read another
       * one; we only needed to look at the versions to check for
       * rollback. */
      if (-1 != good_partition)
        continue;

      /* Verify body load address matches what we expect */
      if (preamble->body_load_address != (size_t)params->kernel_buffer) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Verify kernel body starts at a multiple of the sector size. */
      body_offset = key_block->key_block_size + preamble->preamble_size;
      if (0 != body_offset % blba) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Verify kernel body fits in the partition */
      if (body_offset + preamble->body_signature.data_size >
          part_size * blba) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Read the kernel data */
      if (0 != BootDeviceReadLBA(
              part_start + (body_offset / blba),
              (preamble->body_signature.data_size + blba - 1) / blba,
              params->kernel_buffer)) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Verify kernel data */
      if (0 != VerifyData((const uint8_t*)params->kernel_buffer,
                          &preamble->body_signature, data_key)) {
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Done with the kernel signing key, so can free it now */
      RSAPublicKeyFree(data_key);

      /* If we're still here, the kernel is valid. */
      /* Save the first good partition we find; that's the one we'll boot */
      if (-1 == good_partition) {
        good_partition = gpt.current_kernel;
        params->partition_number = gpt.current_kernel;
        params->bootloader_address = preamble->bootloader_address;
        params->bootloader_size = preamble->bootloader_size;
        /* If we're in developer or recovery mode, there's no rollback
         * protection, so we can stop at the first valid kernel. */
        if (!is_normal)
          break;

        /* Otherwise, we're in normal boot mode, so we do care about
         * the key index in the TPM.  If the good partition's key
         * version is the same as the tpm, then the TPM doesn't need
         * updating; we can stop now.  Otherwise, we'll check all the
         * other headers to see if they contain a newer key. */
        if (key_version == tpm_key_version &&
            preamble->kernel_version == tpm_kernel_version)
          break;
      }
    } /* while(GptNextKernelEntry) */
  } while(0);

  /* Free kernel buffer */
  if (kbuf)
    Free(kbuf);

  /* Write and free GPT data */
  WriteAndFreeGptData(&gpt);

  /* Handle finding a good partition */
  if (good_partition >= 0) {

    /* See if we need to update the TPM */
    if (is_normal) {
      /* We only update the TPM in normal boot mode.  In developer
       * mode, the kernel is self-signed by the developer, so we can't
       * trust the key version and wouldn't want to roll the TPM
       * forward.  In recovery mode, the TPM stays PP-unlocked, so
       * anything we write gets blown away by the firmware when we go
       * back to normal mode. */
      if ((lowest_key_version > tpm_key_version) ||
          (lowest_key_version == tpm_key_version &&
           lowest_kernel_version > tpm_kernel_version)) {
        if (0 != WriteStoredVersions(KERNEL_VERSIONS,
                                     lowest_key_version,
                                     lowest_kernel_version))
          return LOAD_KERNEL_RECOVERY;
      }
    }

    if (!(BOOT_FLAG_RECOVERY & params->boot_flags)) {
      /* We can lock the TPM now, since we've decided which kernel we
       * like.  If we don't find a good kernel, we leave the TPM
       * unlocked so we can try again on the next boot device.  If no
       * kernels are good, we'll reboot to recovery mode, so it's ok to
       * leave the TPM unlocked in that case too.
       *
       * If we're already in recovery mode, we need to leave PP unlocked,
       * so don't lock the kernel versions. */
      if (0 != LockKernelVersionsByLockingPP())
        return LOAD_KERNEL_RECOVERY;
    }

    /* Success! */
    return LOAD_KERNEL_SUCCESS;
  }

  // Handle error cases
  if (found_partitions)
    return LOAD_KERNEL_INVALID;
  else
    return LOAD_KERNEL_NOT_FOUND;
}
