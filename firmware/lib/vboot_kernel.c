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
  uint32_t tpm_version = 0;
  uint64_t lowest_version = 0xFFFFFFFF;
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
  if (0 == kbuf_sectors) {
    VBDEBUG(("LoadKernel() called with sector size > KBUF_SIZE\n"));
    return LOAD_KERNEL_INVALID;
  }

  is_dev = (BOOT_FLAG_DEVELOPER & params->boot_flags ? 1 : 0);
  is_rec = (BOOT_FLAG_RECOVERY & params->boot_flags ? 1 : 0);
  is_normal = (!is_dev && !is_rec);

  /* Clear output params in case we fail */
  params->partition_number = 0;
  params->bootloader_address = 0;
  params->bootloader_size = 0;

  /* Let the TPM know if we're in recovery mode */
  if (is_rec) {
    if (0 != RollbackKernelRecovery(is_dev)) {
      VBDEBUG(("Error setting up TPM for recovery kernel\n"));
      /* Ignore return code, since we need to boot recovery mode to
       * fix the TPM. */
    }
  }

  if (is_normal) {
    /* Read current kernel key index from TPM.  Assumes TPM is already
     * initialized. */
    status = RollbackKernelRead(&tpm_version);
    if (0 != status) {
      VBDEBUG(("Unable to get kernel versions from TPM\n"));
      return (status == TPM_E_MUST_REBOOT ?
              LOAD_KERNEL_REBOOT : LOAD_KERNEL_RECOVERY);
    }
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
      RSAPublicKey* data_key = NULL;
      uint64_t key_version;
      uint64_t combined_version;
      uint64_t body_offset;
      uint64_t body_offset_sectors;
      uint64_t body_sectors;

      VBDEBUG(("Found kernel entry at %" PRIu64 " size %" PRIu64 "\n",
              part_start, part_size));

      /* Found at least one kernel partition. */
      found_partitions++;

      /* Read the first part of the kernel partition  */
      if (part_size < kbuf_sectors) {
        VBDEBUG(("Partition too small to hold kernel.\n"));
        goto bad_kernel;
      }

      if (0 != BootDeviceReadLBA(part_start, kbuf_sectors, kbuf)) {
        VBDEBUG(("Unable to read start of partition.\n"));
        goto bad_kernel;
      }

      /* Verify the key block.  In developer mode, we ignore the key
       * and use only the SHA-512 hash to verify the key block. */
      key_block = (VbKeyBlockHeader*)kbuf;
      if ((0 != KeyBlockVerify(key_block, KBUF_SIZE, kernel_subkey,
                               is_dev && !is_rec))) {
        VBDEBUG(("Verifying key block failed.\n"));
        goto bad_kernel;
      }

      /* Check the key block flags against the current boot mode in normal
       * and recovery modes (not in developer mode booting from SSD). */
      if (is_rec || is_normal) {
        if (!(key_block->key_block_flags &
              (is_dev ? KEY_BLOCK_FLAG_DEVELOPER_1 :
               KEY_BLOCK_FLAG_DEVELOPER_0))) {
          VBDEBUG(("Developer flag mismatch.\n"));
          goto bad_kernel;
        }
        if (!(key_block->key_block_flags &
              (is_rec ? KEY_BLOCK_FLAG_RECOVERY_1 :
               KEY_BLOCK_FLAG_RECOVERY_0))) {
          VBDEBUG(("Recovery flag mismatch.\n"));
          goto bad_kernel;
        }
      }

      /* Check for rollback of key version.  Note this is implicitly
       * skipped in recovery and developer modes because those set
       * key_version=0 above. */
      key_version = key_block->data_key.key_version;
      if (key_version < (tpm_version >> 16)) {
        VBDEBUG(("Key version too old.\n"));
        goto bad_kernel;
      }

      /* Get the key for preamble/data verification from the key block */
      data_key = PublicKeyToRSA(&key_block->data_key);
      if (!data_key) {
        VBDEBUG(("Data key bad.\n"));
        goto bad_kernel;
      }

      /* Verify the preamble, which follows the key block */
      preamble = (VbKernelPreambleHeader*)(kbuf + key_block->key_block_size);
      if ((0 != VerifyKernelPreamble(preamble,
                                     KBUF_SIZE - key_block->key_block_size,
                                     data_key))) {
        VBDEBUG(("Preamble verification failed.\n"));
        goto bad_kernel;
      }

      /* Check for rollback of kernel version.  Note this is implicitly
       * skipped in recovery and developer modes because rollback_index
       * sets those to 0 in those modes. */
      combined_version = ((key_version << 16) |
                          (preamble->kernel_version & 0xFFFF));
      if (combined_version < tpm_version) {
        VBDEBUG(("Kernel version too low.\n"));
        goto bad_kernel;
      }

      VBDEBUG(("Kernel preamble is good.\n"));

      /* Check for lowest version from a valid header. */
      if (lowest_version > combined_version)
        lowest_version = combined_version;

      /* If we already have a good kernel, no need to read another
       * one; we only needed to look at the versions to check for
       * rollback.  So skip to the next kernel preamble. */
      if (-1 != good_partition)
        continue;

      /* Verify body load address matches what we expect */
      if ((preamble->body_load_address != (size_t)params->kernel_buffer) &&
          !(params->boot_flags & BOOT_FLAG_SKIP_ADDR_CHECK)) {
        VBDEBUG(("Wrong body load address.\n"));
        goto bad_kernel;
      }

      /* Verify kernel body starts at a multiple of the sector size. */
      body_offset = key_block->key_block_size + preamble->preamble_size;
      if (0 != body_offset % blba) {
        VBDEBUG(("Kernel body not at multiple of sector size.\n"));
        goto bad_kernel;
      }
      body_offset_sectors = body_offset / blba;

      /* Verify kernel body fits in the buffer */
      body_sectors = (preamble->body_signature.data_size + blba - 1) / blba;
      if (body_sectors * blba > params->kernel_buffer_size) {
        VBDEBUG(("Kernel body doesn't fit in memory.\n"));
        goto bad_kernel;
      }

      /* Verify kernel body fits in the partition */
      if (body_offset_sectors + body_sectors > part_size) {
        VBDEBUG(("Kernel body doesn't fit in partition.\n"));
        goto bad_kernel;
      }

      /* Read the kernel data */
      if (0 != BootDeviceReadLBA(part_start + body_offset_sectors,
                                 body_sectors,
                                 params->kernel_buffer)) {
        VBDEBUG(("Unable to read kernel data.\n"));
        goto bad_kernel;
      }

      /* Verify kernel data */
      if (0 != VerifyData((const uint8_t*)params->kernel_buffer,
                          params->kernel_buffer_size,
                          &preamble->body_signature, data_key)) {
        VBDEBUG(("Kernel data verification failed.\n"));
        goto bad_kernel;
      }

      /* Done with the kernel signing key, so can free it now */
      RSAPublicKeyFree(data_key);
      data_key = NULL;

      /* If we're still here, the kernel is valid. */
      /* Save the first good partition we find; that's the one we'll boot */
      VBDEBUG(("Partiton is good.\n"));
      /* TODO: GPT partitions start at 1, but cgptlib starts them at 0.
       * Adjust here, until cgptlib is fixed. */
      good_partition = gpt.current_kernel + 1;
      params->partition_number = gpt.current_kernel + 1;
      GetCurrentKernelUniqueGuid(&gpt, &params->partition_guid);
      /* TODO: GetCurrentKernelUniqueGuid() should take a destination size, or
       * the dest should be a struct, so we know it's big enough. */
      params->bootloader_address = preamble->bootloader_address;
      params->bootloader_size = preamble->bootloader_size;

      /* Update GPT to note this is the kernel we're trying */
      GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_TRY);

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
      if (combined_version == tpm_version) {
        VBDEBUG(("Same kernel version\n"));
        break;
      }

      /* Continue, so that we skip the error handling code below */
      continue;

   bad_kernel:
      /* Handle errors parsing this kernel */
      if (NULL != data_key)
        RSAPublicKeyFree(data_key);

      VBDEBUG(("Marking kernel as invalid.\n"));
      GptUpdateKernelEntry(&gpt, GPT_UPDATE_ENTRY_BAD);


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
      if (lowest_version > tpm_version) {
        status = RollbackKernelWrite((uint32_t)lowest_version);
        if (0 != status) {
          VBDEBUG(("Error writing kernel versions to TPM.\n"));
          return (status == TPM_E_MUST_REBOOT ?
                  LOAD_KERNEL_REBOOT : LOAD_KERNEL_RECOVERY);
        }
      }
    }

    /* Lock the kernel versions */
    status = RollbackKernelLock();
    if (0 != status) {
      VBDEBUG(("Error locking kernel versions.\n"));
      /* Don't reboot to recovery mode if we're already there */
      if (!is_rec)
        return (status == TPM_E_MUST_REBOOT ?
                LOAD_KERNEL_REBOOT : LOAD_KERNEL_RECOVERY);
    }

    /* Success! */
    return LOAD_KERNEL_SUCCESS;
  }

  /* The BIOS may attempt to display different screens depending on whether
   * we find an invalid kernel partition (return LOAD_KERNEL_INVALID) or not.
   * But the flow is changing, so for now treating both cases as invalid gives
   * slightly less confusing user feedback. Sigh.
   */
  return LOAD_KERNEL_INVALID;
}
