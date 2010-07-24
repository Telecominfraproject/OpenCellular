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
#include "cgptlib_internal.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_common.h"

#define KBUF_SIZE 65536  /* Bytes to read at start of kernel partition */


/* Allocates and reads GPT data from the drive.  The sector_bytes and
 * drive_sectors fields should be filled on input.  The primary and
 * secondary header and entries are filled on output.
 *
 * Returns 0 if successful, 1 if error. */
int AllocAndReadGptData(GptData* gptdata) {

  uint64_t entries_sectors = TOTAL_ENTRIES_SIZE / gptdata->sector_bytes;

  /* No data to be written yet */
  gptdata->modified = 0;

  /* Allocate all buffers */
  gptdata->primary_header = (uint8_t*)Malloc(gptdata->sector_bytes);
  gptdata->secondary_header = (uint8_t*)Malloc(gptdata->sector_bytes);
  gptdata->primary_entries = (uint8_t*)Malloc(TOTAL_ENTRIES_SIZE);
  gptdata->secondary_entries = (uint8_t*)Malloc(TOTAL_ENTRIES_SIZE);

  if (gptdata->primary_header == NULL || gptdata->secondary_header == NULL ||
      gptdata->primary_entries == NULL || gptdata->secondary_entries == NULL)
    return 1;

  /* Read data from the drive, skipping the protective MBR */
  if (0 != BootDeviceReadLBA(1, 1, gptdata->primary_header))
    return 1;
  if (0 != BootDeviceReadLBA(2, entries_sectors, gptdata->primary_entries))
    return 1;
  if (0 != BootDeviceReadLBA(gptdata->drive_sectors - entries_sectors - 1,
                             entries_sectors, gptdata->secondary_entries))
    return 1;
  if (0 != BootDeviceReadLBA(gptdata->drive_sectors - 1,
                             1, gptdata->secondary_header))
    return 1;

  return 0;
}


/* Writes any changes for the GPT data back to the drive, then frees
 * the buffers.
 *
 * Returns 0 if successful, 1 if error. */
int WriteAndFreeGptData(GptData* gptdata) {

  uint64_t entries_sectors = TOTAL_ENTRIES_SIZE / gptdata->sector_bytes;

  if (gptdata->primary_header) {
    if (gptdata->modified & GPT_MODIFIED_HEADER1) {
      VBDEBUG(("Updating GPT header 1\n"));
      if (0 != BootDeviceWriteLBA(1, 1, gptdata->primary_header))
        return 1;
    }
    Free(gptdata->primary_header);
  }

  if (gptdata->primary_entries) {
    if (gptdata->modified & GPT_MODIFIED_ENTRIES1) {
      VBDEBUG(("Updating GPT entries 1\n"));
      if (0 != BootDeviceWriteLBA(2, entries_sectors,
                                  gptdata->primary_entries))
        return 1;
    }
    Free(gptdata->primary_entries);
  }

  if (gptdata->secondary_entries) {
    if (gptdata->modified & GPT_MODIFIED_ENTRIES2) {
      VBDEBUG(("Updating GPT header 2\n"));
      if (0 != BootDeviceWriteLBA(gptdata->drive_sectors - entries_sectors - 1,
                                  entries_sectors, gptdata->secondary_entries))
        return 1;
    }
    Free(gptdata->secondary_entries);
  }

  if (gptdata->secondary_header) {
    if (gptdata->modified & GPT_MODIFIED_HEADER2) {
      VBDEBUG(("Updating GPT entries 2\n"));
      if (0 != BootDeviceWriteLBA(gptdata->drive_sectors - 1, 1,
                                  gptdata->secondary_header))
        return 1;
    }
    Free(gptdata->secondary_header);
  }

  /* Success */
  return 0;
}

/* disable MSVC warning on const logical expression (as in } while(0);) */
__pragma(warning(disable: 4127))

int LoadKernel(LoadKernelParams* params) {
  VbPublicKey* kernel_subkey;
  GptData gpt;
  uint64_t part_start, part_size;
  uint64_t blba;
  uint64_t kbuf_sectors;
  uint8_t* kbuf = NULL;
  int found_partitions = 0;
  int good_partition = -1;
  uint16_t tpm_key_version = 0;
  uint16_t tpm_kernel_version = 0;
  uint64_t lowest_key_version = 0xFFFF;
  uint64_t lowest_kernel_version = 0xFFFF;
  int is_dev;
  int is_rec;
  int is_normal;
  uint32_t status;

  /* Sanity Checks */
  if (!params ||
      !params->bytes_per_lba ||
      !params->ending_lba ||
      !params->kernel_buffer ||
      !params->kernel_buffer_size) {
    VBDEBUG(("LoadKernel() called with invalid params\n"));
    return LOAD_KERNEL_INVALID;
  }

  /* Initialization */
  kernel_subkey = (VbPublicKey*)params->header_sign_key_blob;
  blba = params->bytes_per_lba;
  kbuf_sectors = KBUF_SIZE / blba;
  is_dev = (BOOT_FLAG_DEVELOPER & params->boot_flags ? 1 : 0);
  is_rec = (BOOT_FLAG_RECOVERY & params->boot_flags ? 1 : 0);
  is_normal = (!is_dev && !is_rec);

  /* Clear output params in case we fail */
  params->partition_number = 0;
  params->bootloader_address = 0;
  params->bootloader_size = 0;

  if (!is_dev) {
    /* TODO: should use the TPM all the time; for now, only use when
     * not in developer mode. */
    /* Let the TPM know if we're in recovery mode */
    if (is_rec) {
      if (0 != RollbackKernelRecovery(is_dev ? 1 : 0)) {
        VBDEBUG(("Error setting up TPM for recovery kernel\n"));
        /* Ignore return code, since we need to boot recovery mode to
         * fix the TPM. */
      }
    }
  }

  if (is_normal) {
    /* Read current kernel key index from TPM.  Assumes TPM is already
     * initialized. */
    status = RollbackKernelRead(&tpm_key_version, &tpm_kernel_version);
    if (0 != status) {
      VBDEBUG(("Unable to get kernel versions from TPM\n"));
      return (status == TPM_E_MUST_REBOOT ?
              LOAD_KERNEL_REBOOT : LOAD_KERNEL_RECOVERY);
    }
  } else if (is_dev && !is_rec) {
    /* In developer mode, we ignore the kernel subkey, and just use
     * the SHA-512 hash to verify the key block. */
    kernel_subkey = NULL;
  }

  do {
    /* Read GPT data */
    gpt.sector_bytes = (uint32_t)blba;
    gpt.drive_sectors = params->ending_lba + 1;
    if (0 != AllocAndReadGptData(&gpt)) {
      VBDEBUG(("Unable to read GPT data\n"));
      break;
    }

    /* Initialize GPT library */
    if (GPT_SUCCESS != GptInit(&gpt)) {
      VBDEBUG(("Error parsing GPT\n"));
      break;
    }

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

      VBDEBUG(("Found kernel entry at %" PRIu64 " size %" PRIu64 "\n",
              part_start, part_size));

      /* Found at least one kernel partition. */
      found_partitions++;

      /* Read the first part of the kernel partition  */
      if (part_size < kbuf_sectors)
        continue;
      if (0 != BootDeviceReadLBA(part_start, kbuf_sectors, kbuf))
        continue;

      /* Verify the key block */
      key_block = (VbKeyBlockHeader*)kbuf;
      if ((0 != KeyBlockVerify(key_block, KBUF_SIZE, kernel_subkey))) {
        VBDEBUG(("Verifying key block failed.\n"));
        continue;
      }

      /* Check the key block flags against the current boot mode in normal
       * and recovery modes (not in developer mode booting from SSD). */
      if (is_rec || is_normal) {
        if (!(key_block->key_block_flags &
              (is_dev ? KEY_BLOCK_FLAG_DEVELOPER_1 :
               KEY_BLOCK_FLAG_DEVELOPER_0))) {
          VBDEBUG(("Developer flag mismatch.\n"));
          continue;
        }
        if (!(key_block->key_block_flags &
              (is_rec ? KEY_BLOCK_FLAG_RECOVERY_1 :
               KEY_BLOCK_FLAG_RECOVERY_0))) {
          VBDEBUG(("Recovery flag mismatch.\n"));
          continue;
        }
      }

      /* Check for rollback of key version.  Note this is implicitly
       * skipped in recovery and developer modes because those set
       * key_version=0 above. */
      key_version = key_block->data_key.key_version;
      if (key_version < tpm_key_version) {
        VBDEBUG(("Key version too old.\n"));
        continue;
      }

      /* Get the key for preamble/data verification from the key block */
      data_key = PublicKeyToRSA(&key_block->data_key);
      if (!data_key)
        continue;

      /* Verify the preamble, which follows the key block */
      preamble = (VbKernelPreambleHeader*)(kbuf + key_block->key_block_size);
      if ((0 != VerifyKernelPreamble(preamble,
                                     KBUF_SIZE - key_block->key_block_size,
                                     data_key))) {
        VBDEBUG(("Preamble verification failed.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Check for rollback of kernel version.  Note this is implicitly
       * skipped in recovery and developer modes because those set
       * key_version=0 and kernel_version=0 above. */
      if (key_version == tpm_key_version &&
          preamble->kernel_version < tpm_kernel_version) {
        VBDEBUG(("Kernel version too low.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      VBDEBUG(("Kernel preamble is good.\n"));

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
      if ((preamble->body_load_address != (size_t)params->kernel_buffer) &&
          !(params->boot_flags & BOOT_FLAG_SKIP_ADDR_CHECK)) {
        VBDEBUG(("Wrong body load address.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Verify kernel body starts at a multiple of the sector size. */
      body_offset = key_block->key_block_size + preamble->preamble_size;
      if (0 != body_offset % blba) {
        VBDEBUG(("Kernel body not at multiple of sector size.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Verify kernel body fits in the partition */
      if (body_offset + preamble->body_signature.data_size >
          part_size * blba) {
        VBDEBUG(("Kernel body doesn't fit in partition.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Read the kernel data */
      if (0 != BootDeviceReadLBA(
              part_start + (body_offset / blba),
              (preamble->body_signature.data_size + blba - 1) / blba,
              params->kernel_buffer)) {
        VBDEBUG(("Unable to read kernel data.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Verify kernel data */
      if (0 != VerifyData((const uint8_t*)params->kernel_buffer,
                          params->kernel_buffer_size,
                          &preamble->body_signature, data_key)) {
        VBDEBUG(("Kernel data verification failed.\n"));
        RSAPublicKeyFree(data_key);
        continue;
      }

      /* Done with the kernel signing key, so can free it now */
      RSAPublicKeyFree(data_key);

      /* If we're still here, the kernel is valid. */
      /* Save the first good partition we find; that's the one we'll boot */
      VBDEBUG(("Partiton is good.\n"));
      /* TODO: GPT partitions start at 1, but cgptlib starts them at 0.
       * Adjust here, until cgptlib is fixed. */
      good_partition = gpt.current_kernel + 1;
      params->partition_number = gpt.current_kernel + 1;
      GetCurrentKernelUniqueGuid(&gpt, &params->partition_guid);
      params->bootloader_address = preamble->bootloader_address;
      params->bootloader_size = preamble->bootloader_size;
      /* If we're in developer or recovery mode, there's no rollback
       * protection, so we can stop at the first valid kernel. */
      if (!is_normal) {
        VBDEBUG(("Boot_flags = !is_normal\n"));
        break;
      }

      /* Otherwise, we're in normal boot mode, so we do care about the
       * key index in the TPM.  If the good partition's key version is
       * the same as the tpm, then the TPM doesn't need updating; we
       * can stop now.  Otherwise, we'll check all the other headers
       * to see if they contain a newer key. */
      if (key_version == tpm_key_version &&
          preamble->kernel_version == tpm_kernel_version) {
        VBDEBUG(("Same key version\n"));
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
    VBDEBUG(("Good_partition >= 0\n"));

    /* See if we need to update the TPM */
    if (is_normal) {
      /* We only update the TPM in normal boot mode.  In developer
       * mode, the kernel is self-signed by the developer, so we can't
       * trust the key version and wouldn't want to roll the TPM
       * forward.  In recovery mode, the TPM stays PP-unlocked, so
       * anything we write gets blown away by the firmware when we go
       * back to normal mode. */
      VBDEBUG(("Boot_flags = is_normal\n"));
      if ((lowest_key_version > tpm_key_version) ||
          (lowest_key_version == tpm_key_version &&
           lowest_kernel_version > tpm_kernel_version)) {

        status = RollbackKernelWrite((uint16_t)lowest_key_version,
                                     (uint16_t)lowest_kernel_version);
        if (0 != status) {
          VBDEBUG(("Error writing kernel versions to TPM.\n"));
      return (status == TPM_E_MUST_REBOOT ?
              LOAD_KERNEL_REBOOT : LOAD_KERNEL_RECOVERY);
        }
      }
    }

    if (!is_dev) {
      /* TODO: should use the TPM all the time; for now, only use when
       * not in developer mode. */
      /* Lock the kernel versions */
      status = RollbackKernelLock();
      if (0 != status) {
        VBDEBUG(("Error locking kernel versions.\n"));
        /* Don't reboot to recovery mode if we're already there */
        if (!is_rec)
          return (status == TPM_E_MUST_REBOOT ?
                  LOAD_KERNEL_REBOOT : LOAD_KERNEL_RECOVERY);
      }
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
