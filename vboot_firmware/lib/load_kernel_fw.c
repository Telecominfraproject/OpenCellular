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

// TODO: for testing
#include <stdio.h>
#include "cgptlib_internal.h"

/* TODO: Remove this terrible hack which fakes partition attributes
 * for the kernel partitions so that GptNextKernelEntry() won't
 * choke. */
void FakePartitionAttributes(GptData* gpt) {
  GptEntry* entries = (GptEntry*)gpt->primary_entries;
  GptEntry* e;
  int i;
  printf("Hacking partition attributes...\n");
  printf("Note that GUIDs below have first 3 fields endian-swapped\n");

  for (i = 0, e = entries; i < 12; i++, e++) {

    printf("%2d %08x %04x %04x %02x %02x %02x %02x %02x %02x %02x %02x\n",
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
    if (!IsKernelEntry(e))
      continue;
    printf("Hacking attributes for kernel partition %d\n", i);
    SetEntryPriority(e, 2);
    SetEntrySuccessful(e, 1);
  }
}


int AllocAndReadGptData(GptData *gptdata) {
  /* Allocates and reads GPT data from the drive.  The sector_bytes and
   * drive_sectors fields should be filled on input.  The primary and
   * secondary header and entries are filled on output.
   *
   * Returns 0 if successful, 1 if error. */

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

void WriteAndFreeGptData(GptData *gptdata) {
  /* Writes any changes for the GPT data back to the drive, then frees the
   * buffers. */

  uint64_t entries_sectors = GPT_ENTRIES_SIZE / gptdata->sector_bytes;

  if (gptdata->primary_header) {
    if (gptdata->modified & GPT_MODIFIED_HEADER1)
      BootDeviceWriteLBA(1, 1, gptdata->primary_header);
    Free(gptdata->primary_header);
  }

  if (gptdata->primary_entries) {
    if (gptdata->modified & GPT_MODIFIED_ENTRIES1)
      BootDeviceWriteLBA(2, entries_sectors, gptdata->primary_entries);
    Free(gptdata->primary_entries);
  }

  if (gptdata->secondary_entries) {
    if (gptdata->modified & GPT_MODIFIED_ENTRIES2)
      BootDeviceWriteLBA(gptdata->drive_sectors - entries_sectors - 1,
                         entries_sectors, gptdata->secondary_entries);
    Free(gptdata->secondary_entries);
  }

  if (gptdata->secondary_header) {
    if (gptdata->modified & GPT_MODIFIED_HEADER2)
      BootDeviceWriteLBA(gptdata->drive_sectors - entries_sectors - 1,
                         1, gptdata->secondary_header);
      BootDeviceWriteLBA(gptdata->drive_sectors - 1, 1,
                         gptdata->secondary_header);
    Free(gptdata->secondary_header);
  }
  /* TODO: What to do with return codes from the writes? */
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

  /* Read current kernel key index from TPM.  Assumes TPM is already
   * initialized. */
  /* TODO: Is that a safe assumption?  Normally, SetupTPM() would be called
   * when the RW firmware is verified.  Is it harmful to call SetupTPM()
   * again if it's already initialized?  It'd be easier if we could just do
   * that. */
  GetStoredVersions(KERNEL_VERSIONS,
                    &tpm_kernel_key_version,
                    &tpm_kernel_version);

  do {
    /* Read GPT data */
    gpt.sector_bytes = blba;
    gpt.drive_sectors = params->ending_lba + 1;
    if (0 != AllocAndReadGptData(&gpt))
      break;

    fprintf(stderr, "RRS1\n");

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

    fprintf(stderr, "RRS2\n");

    /* Loop over candidate kernel partitions */
    while (GPT_SUCCESS == GptNextKernelEntry(&gpt, &part_start, &part_size)) {
      RSAPublicKey *kernel_sign_key = NULL;
      int kernel_start, kernel_sectors;

      fprintf(stderr, "RRS3\n");

      /* Found at least one kernel partition. */
      found_partition = 1;

      /* Read the first part of the kernel partition  */
      if (part_size < kbuf_sectors)
        continue;
      if (0 != BootDeviceReadLBA(part_start, kbuf_sectors, kbuf))
        continue;

      fprintf(stderr, "RRS4\n");

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

      fprintf(stderr, "RRS5\n");

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
        params->bootloader_start = (uint8_t*)params->kernel_buffer +
            kim->bootloader_offset;
        params->bootloader_size = kim->bootloader_size;

        /* If the good partition's key version is the same as the tpm, then
         * the TPM doesn't need updating; we can stop now.  Otherwise, we'll
         * check all the other headers to see if they contain a newer key. */
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

  // Write and free GPT data
  WriteAndFreeGptData(&gpt);

  // Handle finding a good partition
  if (good_partition >= 0) {

    /* See if we need to update the TPM */
    if ((lowest_kernel_key_version > tpm_kernel_key_version) ||
        (lowest_kernel_key_version == tpm_kernel_key_version &&
         lowest_kernel_version > tpm_kernel_version)) {
      WriteStoredVersions(KERNEL_VERSIONS,
                          lowest_kernel_key_version,
                          lowest_kernel_version);
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
      LockKernelVersionsByLockingPP();
    }

    /* Success! */
    return LOAD_KERNEL_SUCCESS;
  }

  // Handle error cases
  if (found_partition)
    return LOAD_KERNEL_INVALID;
  else
    return LOAD_KERNEL_NOT_FOUND;
  /* TODO: no error code for "internal error", but what would the firmware do
   * with that anyway?  So in the do-while(0) code above, the firmware just
   * does 'break' to indicate an internal error... */
}
