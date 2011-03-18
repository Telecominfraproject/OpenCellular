/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
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
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_common.h"

#define KBUF_SIZE 65536  /* Bytes to read at start of kernel partition */
#define LOWEST_TPM_VERSION 0xffffffff

typedef enum BootMode {
  kBootNormal,   /* Normal firmware */
  kBootDev,      /* Dev firmware AND dev switch is on */
  kBootRecovery  /* Recovery firmware, regardless of dev switch position */
} BootMode;


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
  VbSharedDataHeader* shared = (VbSharedDataHeader*)params->shared_data_blob;
  VbNvContext* vnc = params->nv_context;
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)params->gbb_data;
  VbPublicKey* kernel_subkey;
  GptData gpt;
  uint64_t part_start, part_size;
  uint64_t blba;
  uint64_t kbuf_sectors;
  uint8_t* kbuf = NULL;
  int found_partitions = 0;
  int good_partition = -1;
  int good_partition_key_block_valid = 0;
  uint32_t tpm_version = 0;
  uint64_t lowest_version = LOWEST_TPM_VERSION;
  int rec_switch, dev_switch;
  BootMode boot_mode;
  uint32_t test_err = 0;
  uint32_t status;

  int retval = LOAD_KERNEL_RECOVERY;
  int recovery = VBNV_RECOVERY_RO_UNSPECIFIED;
  uint64_t timer_enter = VbGetTimer();

  /* Setup NV storage */
  VbNvSetup(vnc);

  /* Sanity Checks */
  if (!params ||
      !params->bytes_per_lba ||
      !params->ending_lba ||
      !params->kernel_buffer ||
      !params->kernel_buffer_size) {
    VBDEBUG(("LoadKernel() called with invalid params\n"));
    goto LoadKernelExit;
  }

  /* Clear output params in case we fail */
  params->partition_number = 0;
  params->bootloader_address = 0;
  params->bootloader_size = 0;

  /* Handle test errors */
  VbNvGet(vnc, VBNV_TEST_ERROR_FUNC, &test_err);
  if (VBNV_TEST_ERROR_LOAD_KERNEL == test_err) {
    /* Get error code */
    VbNvGet(vnc, VBNV_TEST_ERROR_NUM, &test_err);
    /* Clear test params so we don't repeat the error */
    VbNvSet(vnc, VBNV_TEST_ERROR_FUNC, 0);
    VbNvSet(vnc, VBNV_TEST_ERROR_NUM, 0);
    /* Handle error codes */
    switch (test_err) {
      case LOAD_KERNEL_RECOVERY:
        recovery = VBNV_RECOVERY_RW_TEST_LK;
        goto LoadKernelExit;
      case LOAD_KERNEL_NOT_FOUND:
      case LOAD_KERNEL_INVALID:
      case LOAD_KERNEL_REBOOT:
        retval = test_err;
        goto LoadKernelExit;
      default:
        break;
    }
  }

  /* Initialization */
  blba = params->bytes_per_lba;
  kbuf_sectors = KBUF_SIZE / blba;
  if (0 == kbuf_sectors) {
    VBDEBUG(("LoadKernel() called with sector size > KBUF_SIZE\n"));
    goto LoadKernelExit;
  }

  rec_switch = (BOOT_FLAG_RECOVERY & params->boot_flags ? 1 : 0);
  dev_switch = (BOOT_FLAG_DEVELOPER & params->boot_flags ? 1 : 0);

  if (rec_switch)
    boot_mode = kBootRecovery;
  else if (BOOT_FLAG_DEV_FIRMWARE & params->boot_flags) {
    if (!dev_switch) {
      /* Dev firmware should be signed such that it never boots with the dev
       * switch is off; so something is terribly wrong. */
      VBDEBUG(("LoadKernel() called with dev firmware but dev switch off\n"));
      recovery = VBNV_RECOVERY_RW_DEV_MISMATCH;
      goto LoadKernelExit;
    }
    boot_mode = kBootDev;
  } else {
    /* Normal firmware */
    boot_mode = kBootNormal;
    dev_switch = 0;  /* Always do a fully verified boot */
  }

  if (kBootRecovery == boot_mode) {
    /* Initialize the shared data structure, since LoadFirmware() didn't do it
     * for us. */
    if (0 != VbSharedDataInit(shared, params->shared_data_size)) {
      /* Error initializing the shared data, but we can keep going.  We just
       * can't use the shared data. */
      VBDEBUG(("Shared data init error\n"));
      params->shared_data_size = 0;
      shared = NULL;
    }

    /* Use the recovery key to verify the kernel */
    kernel_subkey = (VbPublicKey*)((uint8_t*)gbb + gbb->recovery_key_offset);

    /* Let the TPM know if we're in recovery mode */
    if (0 != RollbackKernelRecovery(dev_switch)) {
      VBDEBUG(("Error setting up TPM for recovery kernel\n"));
      /* Ignore return code, since we need to boot recovery mode to
       * fix the TPM. */
    }

    /* Read the key indices from the TPM; ignore any errors */
    if (shared) {
      RollbackFirmwareRead(&shared->fw_version_tpm);
      RollbackKernelRead(&shared->kernel_version_tpm);
    }
  } else {
    /* Use the kernel subkey passed from LoadFirmware(). */
    kernel_subkey = &shared->kernel_subkey;

    /* Read current kernel key index from TPM.  Assumes TPM is already
     * initialized. */
    status = RollbackKernelRead(&tpm_version);
    if (0 != status) {
      VBDEBUG(("Unable to get kernel versions from TPM\n"));
      if (status == TPM_E_MUST_REBOOT)
        retval = LOAD_KERNEL_REBOOT;
      else
        recovery = VBNV_RECOVERY_RW_TPM_ERROR;
      goto LoadKernelExit;
    }
    if (shared)
      shared->kernel_version_tpm = tpm_version;
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
      int key_block_valid = 1;

      VBDEBUG(("Found kernel entry at %" PRIu64 " size %" PRIu64 "\n",
              part_start, part_size));

      /* Found at least one kernel partition. */
      found_partitions++;

      /* Read the first part of the kernel partition. */
      if (part_size < kbuf_sectors) {
        VBDEBUG(("Partition too small to hold kernel.\n"));
        goto bad_kernel;
      }

      if (0 != BootDeviceReadLBA(part_start, kbuf_sectors, kbuf)) {
        VBDEBUG(("Unable to read start of partition.\n"));
        goto bad_kernel;
      }

      /* Verify the key block. */
      key_block = (VbKeyBlockHeader*)kbuf;
      if (0 != KeyBlockVerify(key_block, KBUF_SIZE, kernel_subkey, 0)) {
        VBDEBUG(("Verifying key block signature failed.\n"));
        key_block_valid = 0;

        /* If we're not in developer mode, this kernel is bad. */
        if (kBootDev != boot_mode)
          goto bad_kernel;

        /* In developer mode, we can continue if the SHA-512 hash of the key
         * block is valid. */
        if (0 != KeyBlockVerify(key_block, KBUF_SIZE, kernel_subkey, 1)) {
          VBDEBUG(("Verifying key block hash failed.\n"));
          goto bad_kernel;
        }
      }

      /* Check the key block flags against the current boot mode. */
      if (!(key_block->key_block_flags &
            (dev_switch ? KEY_BLOCK_FLAG_DEVELOPER_1 :
             KEY_BLOCK_FLAG_DEVELOPER_0))) {
        VBDEBUG(("Key block developer flag mismatch.\n"));
        key_block_valid = 0;
      }
      if (!(key_block->key_block_flags &
            (rec_switch ? KEY_BLOCK_FLAG_RECOVERY_1 :
             KEY_BLOCK_FLAG_RECOVERY_0))) {
        VBDEBUG(("Key block recovery flag mismatch.\n"));
        key_block_valid = 0;
      }

      /* Check for rollback of key version except in recovery mode. */
      key_version = key_block->data_key.key_version;
      if (kBootRecovery != boot_mode) {
        if (key_version < (tpm_version >> 16)) {
          VBDEBUG(("Key version too old.\n"));
          key_block_valid = 0;
        }
      }

      /* If we're not in developer mode, require the key block to be valid. */
      if (kBootDev != boot_mode && !key_block_valid) {
        VBDEBUG(("Key block is invalid.\n"));
        goto bad_kernel;
      }

      /* Get the key for preamble/data verification from the key block. */
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

      /* If the key block is valid and we're not in recovery mode, check for
       * rollback of the kernel version. */
      combined_version = ((key_version << 16) |
                          (preamble->kernel_version & 0xFFFF));
      if (key_block_valid && kBootRecovery != boot_mode) {
        if (combined_version < tpm_version) {
          VBDEBUG(("Kernel version too low.\n"));
          /* If we're not in developer mode, kernel version must be valid. */
          if (kBootDev != boot_mode)
            goto bad_kernel;
        }
      }

      VBDEBUG(("Kernel preamble is good.\n"));

      /* Check for lowest version from a valid header. */
      if (key_block_valid && lowest_version > combined_version)
        lowest_version = combined_version;
      else {
        VBDEBUG(("Key block valid: %d\n", key_block_valid));
        VBDEBUG(("Combined version: %" PRIu64 "\n", combined_version));
      }

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
      VBPERFSTART("VB_RKD");
      if (0 != BootDeviceReadLBA(part_start + body_offset_sectors,
                                 body_sectors,
                                 params->kernel_buffer)) {
        VBDEBUG(("Unable to read kernel data.\n"));
        VBPERFEND("VB_RKD");
        goto bad_kernel;
      }
      VBPERFEND("VB_RKD");

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
      VBDEBUG(("Partition is good.\n"));
      good_partition_key_block_valid = key_block_valid;
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

      /* If we're in recovery mode or we're about to boot a dev-signed kernel,
       * there's no rollback protection, so we can stop at the first valid
       * kernel. */
      if (kBootRecovery == boot_mode || !key_block_valid) {
        VBDEBUG(("In recovery mode or dev-signed kernel\n"));
        break;
      }

      /* Otherwise, we do care about the key index in the TPM.  If the good
       * partition's key version is the same as the tpm, then the TPM doesn't
       * need updating; we can stop now.  Otherwise, we'll check all the other
       * headers to see if they contain a newer key. */
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
    if (kBootRecovery != boot_mode && good_partition_key_block_valid) {
      /* We only update the TPM in normal and developer boot modes.  In
       * developer mode, we only advanced lowest_version for kernels with valid
       * key blocks, and didn't count self-signed key blocks.  In recovery
       * mode, the TPM stays PP-unlocked, so anything we write gets blown away
       * by the firmware when we go back to normal mode. */
      VBDEBUG(("Boot_flags = not recovery\n"));

      if ((lowest_version > tpm_version) &&
          (lowest_version != LOWEST_TPM_VERSION)) {
        status = RollbackKernelWrite((uint32_t)lowest_version);
        if (0 != status) {
          VBDEBUG(("Error writing kernel versions to TPM.\n"));
          if (status == TPM_E_MUST_REBOOT)
            retval = LOAD_KERNEL_REBOOT;
          else
            recovery = VBNV_RECOVERY_RW_TPM_ERROR;
          goto LoadKernelExit;
        }
        if (shared)
          shared->kernel_version_tpm = (uint32_t)lowest_version;
      }
    }

    /* Lock the kernel versions */
    status = RollbackKernelLock();
    if (0 != status) {
      VBDEBUG(("Error locking kernel versions.\n"));
      /* Don't reboot to recovery mode if we're already there */
      if (kBootRecovery != boot_mode) {
        if (status == TPM_E_MUST_REBOOT)
          retval = LOAD_KERNEL_REBOOT;
        else
          recovery = VBNV_RECOVERY_RW_TPM_ERROR;
        goto LoadKernelExit;
      }
    }

    /* Success! */
    retval = LOAD_KERNEL_SUCCESS;
  } else {
    /* TODO: differentiate between finding an invalid kernel
     * (found_partitions>0) and not finding one at all.  Right now we
     * treat them the same, and return LOAD_KERNEL_INVALID for both. */
    retval = LOAD_KERNEL_INVALID;
  }

LoadKernelExit:

  /* Save whether the good partition's key block was fully verified */
  VbNvSet(vnc, VBNV_FW_VERIFIED_KERNEL_KEY, good_partition_key_block_valid);

  /* Store recovery request, if any, then tear down non-volatile storage */
  VbNvSet(vnc, VBNV_RECOVERY_REQUEST, LOAD_KERNEL_RECOVERY == retval ?
          recovery : VBNV_RECOVERY_NOT_REQUESTED);
  VbNvTeardown(vnc);

  if (shared) {
    /* Save timer values */
    shared->timer_load_kernel_enter = timer_enter;
    shared->timer_load_kernel_exit = VbGetTimer();
    /* Store how much shared data we used, if any */
    params->shared_data_size = shared->data_used;
  }

  return retval;
}
