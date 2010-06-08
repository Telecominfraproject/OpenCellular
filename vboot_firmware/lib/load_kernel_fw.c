/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for loading a kernel from disk.
 * (Firmware portion)
 */

#include "load_kernel_fw.h"

#include "boot_device.h"
#include "cgptlib.h"
#include "kernel_image_fw.h"
#include "rollback_index.h"
#include "utility.h"

#define GPT_ENTRIES_SIZE 16384 /* Bytes to read for GPT entries */

#ifdef PRINT_DEBUG_INFO
// TODO: for testing
#include <stdio.h>
#include <inttypes.h>  /* For PRIu64 macro */
#endif

/* TODO: Remove this terrible hack which fakes partition attributes
 * for the kernel partitions so that GptNextKernelEntry() won't
 * choke. */
#include "cgptlib_internal.h"
void FakePartitionAttributes(GptData* gpt) {
  GptHeader* h = (GptHeader*)gpt->primary_header;
  GptEntry* entries = (GptEntry*)gpt->primary_entries;
  GptEntry* e;
  int i;

  for (i = 0, e = entries; i < h->number_of_entries; i++, e++) {
    if (!IsKernelEntry(e))
      continue;

#ifdef PRINT_DEBUG_INFO

    printf("%2d %08x %04x %04x %02x %02x %02x %02x %02x %02x %02x %02x",
           i,
           e->type.u.Uuid.time_low,
           e->type.u.Uuid.time_mid,
           e->type.u.Uuid.time_high_and_version,
           e->type.u.Uuid.clock_seq_high_and_reserved,
           e->type.u.Uuid.clock_seq_low,
           e->type.u.Uuid.node[0],
           e->type.u.Uuid.node[1],
           e->type.u.Uuid.node[2],
           e->type.u.Uuid.node[3],
           e->type.u.Uuid.node[4],
           e->type.u.Uuid.node[5]
           );
    printf(" %8" PRIu64 " %8" PRIu64"\n", e->starting_lba,
           e->ending_lba - e->starting_lba + 1);
    printf("Hacking attributes for kernel partition %d\n", i);
#endif

    SetEntryPriority(e, 2);
    SetEntrySuccessful(e, 1);
  }
}


/* Allocates and reads GPT data from the drive.  The sector_bytes and
 * drive_sectors fields should be filled on input.  The primary and
 * secondary header and entries are filled on output.
 *
 * Returns 0 if successful, 1 if error. */
int AllocAndReadGptData(GptData* gptdata) {

  uint64_t entries_sectors = GPT_ENTRIES_SIZE / gptdata->sector_bytes;

  /* No data to be written yet */
  gptdata->modified = 0;

  /* Allocate all buffers */
  gptdata->primary_header = (uint8_t*)Malloc(gptdata->sector_bytes);
  gptdata->secondary_header = (uint8_t*)Malloc(gptdata->sector_bytes);
  gptdata->primary_entries = (uint8_t*)Malloc(GPT_ENTRIES_SIZE);
  gptdata->secondary_entries = (uint8_t*)Malloc(GPT_ENTRIES_SIZE);

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

  uint64_t entries_sectors = GPT_ENTRIES_SIZE / gptdata->sector_bytes;

  if (gptdata->primary_header) {
    if (gptdata->modified & GPT_MODIFIED_HEADER1) {
      if (0 != BootDeviceWriteLBA(1, 1, gptdata->primary_header))
        return 1;
    }
    Free(gptdata->primary_header);
  }

  if (gptdata->primary_entries) {
    if (gptdata->modified & GPT_MODIFIED_ENTRIES1) {
      if (0 != BootDeviceWriteLBA(2, entries_sectors,
                                  gptdata->primary_entries))
        return 1;
    }
    Free(gptdata->primary_entries);
  }

  if (gptdata->secondary_entries) {
    if (gptdata->modified & GPT_MODIFIED_ENTRIES2) {
      if (0 != BootDeviceWriteLBA(gptdata->drive_sectors - entries_sectors - 1,
                                  entries_sectors, gptdata->secondary_entries))
        return 1;
    }
    Free(gptdata->secondary_entries);
  }

  if (gptdata->secondary_header) {
    if (gptdata->modified & GPT_MODIFIED_HEADER2) {
      if (0 != BootDeviceWriteLBA(gptdata->drive_sectors - 1, 1,
                                  gptdata->secondary_header))
        return 1;
    }
    Free(gptdata->secondary_header);
  }

  /* Success */
  return 0;
}


#define KBUF_SIZE 65536  /* Bytes to read at start of kernel partition */

int LoadKernel(LoadKernelParams* params) {

  GptData gpt;
  uint64_t part_start, part_size;
  uint64_t blba = params->bytes_per_lba;
  uint8_t* kbuf = NULL;
  uint64_t kbuf_sectors;
  int found_partition = 0;
  int good_partition = -1;
  uint16_t tpm_kernel_key_version, tpm_kernel_version;
  uint16_t lowest_kernel_key_version = 0xFFFF;
  uint16_t lowest_kernel_version = 0xFFFF;
  KernelImage *kim = NULL;

  /* Clear output params in case we fail */
  params->partition_number = 0;
  params->bootloader_address = 0;
  params->bootloader_size = 0;

  if (BOOT_MODE_NORMAL == params->boot_mode) {
    /* Read current kernel key index from TPM.  Assumes TPM is already
     * initialized. */
   if (0 != GetStoredVersions(KERNEL_VERSIONS,
                               &tpm_kernel_key_version,
                               &tpm_kernel_version))
      return LOAD_KERNEL_RECOVERY;
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

    /* Allocate kernel header and image work buffers */
    kbuf = (uint8_t*)Malloc(KBUF_SIZE);
    if (!kbuf)
      break;

    kbuf_sectors = KBUF_SIZE / blba;
    kim = (KernelImage*)Malloc(sizeof(KernelImage));
    if (!kim)
      break;

    /* Loop over candidate kernel partitions */
    while (GPT_SUCCESS == GptNextKernelEntry(&gpt, &part_start, &part_size)) {
      RSAPublicKey *kernel_sign_key = NULL;
      int kernel_start, kernel_sectors;

      /* Found at least one kernel partition. */
      found_partition = 1;

      /* Read the first part of the kernel partition  */
      if (part_size < kbuf_sectors)
        continue;
      if (0 != BootDeviceReadLBA(part_start, kbuf_sectors, kbuf))
        continue;

      /* Verify the kernel header and preamble */
      if (VERIFY_KERNEL_SUCCESS != VerifyKernelHeader(
              params->header_sign_key_blob,
              kbuf,
              KBUF_SIZE,
              (BOOT_MODE_DEVELOPER == params->boot_mode ? 1 : 0),
              kim,
              &kernel_sign_key)) {
        continue;
      }

#ifdef PRINT_DEBUG_INFO
      printf("Kernel header:\n");
      printf("header version:     %d\n", kim->header_version);
      printf("header len:         %d\n", kim->header_len);
      printf("firmware sign alg:  %d\n", kim->firmware_sign_algorithm);
      printf("kernel sign alg:    %d\n", kim->kernel_sign_algorithm);
      printf("kernel key version: %d\n", kim->kernel_key_version);
      printf("kernel version:     %d\n", kim->kernel_version);
      printf("kernel len:         %" PRIu64 "\n", kim->kernel_len);
      printf("bootloader addr:    %" PRIu64 "\n", kim->bootloader_offset);
      printf("bootloader size:    %" PRIu64 "\n", kim->bootloader_size);
      printf("padded header size: %" PRIu64 "\n", kim->padded_header_size);
#endif

      /* Check for rollback of key version */
      if (kim->kernel_key_version < tpm_kernel_key_version) {
        RSAPublicKeyFree(kernel_sign_key);
        continue;
      }

      /* Check for rollback of kernel version */
      if (kim->kernel_key_version == tpm_kernel_key_version &&
           kim->kernel_version < tpm_kernel_version) {
        RSAPublicKeyFree(kernel_sign_key);
        continue;
      }

      /* Check for lowest key version from a valid header. */
      if (lowest_kernel_key_version > kim->kernel_key_version) {
        lowest_kernel_key_version = kim->kernel_key_version;
        lowest_kernel_version = kim->kernel_version;
      }
      else if (lowest_kernel_key_version == kim->kernel_key_version &&
               lowest_kernel_version > kim->kernel_version) {
        lowest_kernel_version = kim->kernel_version;
      }

      /* Verify kernel padding is a multiple of sector size. */
      if (0 != kim->padded_header_size % blba) {
        RSAPublicKeyFree(kernel_sign_key);
        continue;
      }

      kernel_start = part_start + (kim->padded_header_size / blba);
      kernel_sectors = (kim->kernel_len + blba - 1) / blba;

      /* Read the kernel data */
      if (0 != BootDeviceReadLBA(kernel_start, kernel_sectors,
                                 params->kernel_buffer)) {
        RSAPublicKeyFree(kernel_sign_key);
        continue;
      }

      /* Verify kernel data */
      if (0 != VerifyKernelData(kernel_sign_key,
                                kim->kernel_signature,
                                params->kernel_buffer,
                                kim->kernel_len,
                                kim->kernel_sign_algorithm)) {
        RSAPublicKeyFree(kernel_sign_key);
        continue;
      }

      /* Done with the kernel signing key, so can free it now */
      RSAPublicKeyFree(kernel_sign_key);

      /* If we're still here, the kernel is valid. */
      /* Save the first good partition we find; that's the one we'll boot */
      if (-1 == good_partition) {
        good_partition = gpt.current_kernel;
        params->partition_number = gpt.current_kernel;
        params->bootloader_address = kim->bootloader_offset;
        params->bootloader_size = kim->bootloader_size;

        /* If we're in developer or recovery mode, there's no rollback
         * protection, so we can stop at the first valid kernel. */
        if (BOOT_MODE_NORMAL != params->boot_mode)
          break;

        /* Otherwise, we're in normal boot mode, so we do care about
         * the key index in the TPM.  If the good partition's key
         * version is the same as the tpm, then the TPM doesn't need
         * updating; we can stop now.  Otherwise, we'll check all the
         * other headers to see if they contain a newer key. */
        if (kim->kernel_key_version == tpm_kernel_key_version &&
            kim->kernel_version == tpm_kernel_version)
          break;
      }
    } /* while(GptNextKernelEntry) */
  } while(0);

  /* Free kernel work and image buffers */
  if (kbuf)
    Free(kbuf);
  if (kim)
    Free(kim);

  /* Write and free GPT data */
  WriteAndFreeGptData(&gpt);

  /* Handle finding a good partition */
  if (good_partition >= 0) {

    if (BOOT_MODE_NORMAL == params->boot_mode) {
      /* See if we need to update the TPM, for normal boot mode only. */
      if ((lowest_kernel_key_version > tpm_kernel_key_version) ||
          (lowest_kernel_key_version == tpm_kernel_key_version &&
           lowest_kernel_version > tpm_kernel_version)) {
        if (0 != WriteStoredVersions(KERNEL_VERSIONS,
                                     lowest_kernel_key_version,
                                     lowest_kernel_version))
          return LOAD_KERNEL_RECOVERY;
      }
    }

    if (BOOT_MODE_RECOVERY != params->boot_mode) {
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

  /* Handle error cases */
  if (found_partition)
    return LOAD_KERNEL_INVALID;
  else
    return LOAD_KERNEL_NOT_FOUND;
}
