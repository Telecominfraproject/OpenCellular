/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "mtdlib.h"

#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "utility.h"
#include "vboot_api.h"

int MtdInit(MtdData *mtd) {
  int ret;

  mtd->modified = 0;
  mtd->current_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;
  mtd->current_priority = 999;

  ret = MtdSanityCheck(mtd);
  if (GPT_SUCCESS != ret) {
    VBDEBUG(("MtdInit() failed sanity check\n"));
    return ret;
  }

  return GPT_SUCCESS;
}

int MtdCheckParameters(MtdData *disk) {
  if (disk->sector_bytes != 512) {
    return GPT_ERROR_INVALID_SECTOR_SIZE;
  }

  /* At minimum, the disk must consist of at least one erase block */
  if (disk->drive_sectors < disk->flash_block_bytes / disk->sector_bytes) {
    return GPT_ERROR_INVALID_SECTOR_NUMBER;
  }

  /* Write pages must be an integer multiple of sector size */
  if (disk->flash_page_bytes == 0 ||
      disk->flash_page_bytes % disk->sector_bytes != 0) {
    return GPT_ERROR_INVALID_FLASH_GEOMETRY;
  }

  /* Erase blocks must be an integer multiple of write pages */
  if (disk->flash_block_bytes == 0 ||
      disk->flash_block_bytes % disk->flash_page_bytes != 0) {
    return GPT_ERROR_INVALID_FLASH_GEOMETRY;
  }

  /* Without a FTS region, why are you using MTD? */
  if (disk->fts_block_size == 0) {
    return GPT_ERROR_INVALID_FLASH_GEOMETRY;
  }
  return GPT_SUCCESS;
}

int MtdGetEntryPriority(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_PRIORITY_MASK) >>
          MTD_ATTRIBUTE_PRIORITY_OFFSET);
}

int MtdGetEntryTries(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_TRIES_MASK) >>
          MTD_ATTRIBUTE_TRIES_OFFSET);
}

int MtdGetEntrySuccessful(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_SUCCESSFUL_MASK) >>
          MTD_ATTRIBUTE_SUCCESSFUL_OFFSET);
}

int MtdGetEntryType(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_TYPE_MASK) >> MTD_ATTRIBUTE_TYPE_OFFSET);
}

int MtdIsKernelEntry(const MtdDiskPartition *e) {
  return MtdGetEntryType(e) == MTD_PARTITION_TYPE_CHROMEOS_KERNEL;
}


static void SetBitfield(MtdDiskPartition *e,
                        uint32_t offset, uint32_t mask, uint32_t v) {
  e->flags = (e->flags & ~mask) | ((v << offset) & mask);
}
void MtdSetEntrySuccessful(MtdDiskPartition *e, int successful) {
  SetBitfield(e, MTD_ATTRIBUTE_SUCCESSFUL_OFFSET, MTD_ATTRIBUTE_SUCCESSFUL_MASK,
              successful);
}
void MtdSetEntryPriority(MtdDiskPartition *e, int priority) {
  SetBitfield(e, MTD_ATTRIBUTE_PRIORITY_OFFSET, MTD_ATTRIBUTE_PRIORITY_MASK,
              priority);
}
void MtdSetEntryTries(MtdDiskPartition *e, int tries) {
  SetBitfield(e, MTD_ATTRIBUTE_TRIES_OFFSET, MTD_ATTRIBUTE_TRIES_MASK, tries);
}
void MtdSetEntryType(MtdDiskPartition *e, int type) {
  SetBitfield(e, MTD_ATTRIBUTE_TYPE_OFFSET, MTD_ATTRIBUTE_TYPE_MASK, type);
}

void MtdModified(MtdData *mtd) {
  mtd->modified = 1;
  mtd->primary.crc32 = MtdHeaderCrc(&mtd->primary);
}

int MtdIsPartitionValid(const MtdDiskPartition *part) {
  return MtdGetEntryType(part) != 0;
}

int MtdCheckEntries(MtdDiskPartition *entries, MtdDiskLayout *h) {
  uint32_t i, j;

  for (i = 0; i < MTD_MAX_PARTITIONS; i++) {
    for (j = 0; j < MTD_MAX_PARTITIONS; j++) {
      if (i != j) {
        MtdDiskPartition *entry = entries + i;
        MtdDiskPartition *e2 = entries + j;

        if (!MtdIsPartitionValid(entry) || !MtdIsPartitionValid(e2))
          continue;

        if((entry->starting_lba == 0 && entry->ending_lba == 0) ||
           (e2->starting_lba == 0 && e2->ending_lba == 0)) {
          continue;
        }

        if (entry->ending_lba > h->last_lba) {
          return GPT_ERROR_OUT_OF_REGION;
        }
        if (entry->starting_lba < h->first_lba) {
          return GPT_ERROR_OUT_OF_REGION;
        }
        if (entry->starting_lba > entry->ending_lba) {
          return GPT_ERROR_OUT_OF_REGION;
        }

        if ((entry->starting_lba >= e2->starting_lba) &&
            (entry->starting_lba <= e2->ending_lba)) {
          return GPT_ERROR_START_LBA_OVERLAP;
        }
        if ((entry->ending_lba >= e2->starting_lba) &&
            (entry->ending_lba <= e2->ending_lba)) {
          return GPT_ERROR_END_LBA_OVERLAP;
        }
      }
    }
  }
  return GPT_SUCCESS;
}

int MtdSanityCheck(MtdData *disk) {
  MtdDiskLayout *h = &disk->primary;
  int ret;

  ret = MtdCheckParameters(disk);
  if(GPT_SUCCESS != ret)
    return ret;

  if (Memcmp(disk->primary.signature, MTD_DRIVE_SIGNATURE,
              sizeof(disk->primary.signature))) {
    return GPT_ERROR_INVALID_HEADERS;
  }

  if (disk->primary.first_lba > disk->primary.last_lba ||
      disk->primary.last_lba > disk->drive_sectors) {
    return GPT_ERROR_INVALID_SECTOR_NUMBER;
  }

  if (h->crc32 != MtdHeaderCrc(h)) {
    return GPT_ERROR_CRC_CORRUPTED;
  }
  if (h->size < MTD_DRIVE_V1_SIZE) {
    return GPT_ERROR_INVALID_HEADERS;
  }
  return MtdCheckEntries(h->partitions, h);
}

void MtdRepair(MtdData *gpt) {

}

void MtdGetCurrentKernelUniqueGuid(MtdData *gpt, void *dest) {
  Memset(dest, 0, 16);
}

uint32_t MtdHeaderCrc(MtdDiskLayout *h) {
  uint32_t crc32, original_crc32;

  /* Original CRC is calculated with the CRC field 0. */
  original_crc32 = h->crc32;
  h->crc32 = 0;
  crc32 = Crc32((const uint8_t *)h, h->size);
  h->crc32 = original_crc32;

  return crc32;
}



int MtdNextKernelEntry(MtdData *mtd, uint64_t *start_sector, uint64_t *size)
{
  MtdDiskLayout *header = &mtd->primary;
  MtdDiskPartition *entries = header->partitions;
  MtdDiskPartition *e;
  int new_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;
  int new_prio = 0;
  uint32_t i;

  /*
   * If we already found a kernel, continue the scan at the current
   * kernel's priority, in case there is another kernel with the same
   * priority.
   */
  if (mtd->current_kernel != CGPT_KERNEL_ENTRY_NOT_FOUND) {
    for (i = mtd->current_kernel + 1;
         i < MTD_MAX_PARTITIONS; i++) {
      e = entries + i;
      if (!MtdIsKernelEntry(e))
        continue;
      VBDEBUG(("GptNextKernelEntry looking at same prio "
         "partition %d\n", i+1));
      VBDEBUG(("GptNextKernelEntry s%d t%d p%d\n",
         MtdGetEntrySuccessful(e), MtdGetEntryTries(e),
         MtdGetEntryPriority(e)));
      if (!(MtdGetEntrySuccessful(e) || MtdGetEntryTries(e)))
        continue;
      if (MtdGetEntryPriority(e) == mtd->current_priority) {
        mtd->current_kernel = i;
        *start_sector = e->starting_lba;
        *size = e->ending_lba - e->starting_lba + 1;
        VBDEBUG(("GptNextKernelEntry likes it\n"));
        return GPT_SUCCESS;
      }
    }
  }

  /*
   * We're still here, so scan for the remaining kernel with the highest
   * priority less than the previous attempt.
   */
  for (i = 0, e = entries; i < MTD_MAX_PARTITIONS; i++, e++) {
    int current_prio = MtdGetEntryPriority(e);
    if (!MtdIsKernelEntry(e))
      continue;
    VBDEBUG(("GptNextKernelEntry looking at new prio "
       "partition %d\n", i+1));
    VBDEBUG(("GptNextKernelEntry s%d t%d p%d\n",
       MtdGetEntrySuccessful(e), MtdGetEntryTries(e),
       MtdGetEntryPriority(e)));
    if (!(MtdGetEntrySuccessful(e) || MtdGetEntryTries(e)))
      continue;
    if (current_prio >= mtd->current_priority) {
      /* Already returned this kernel in a previous call */
      continue;
    }
    if (current_prio > new_prio) {
      new_kernel = i;
      new_prio = current_prio;
    }
  }

  /*
   * Save what we found.  Note that if we didn't find a new kernel,
   * new_prio will still be -1, so future calls to this function will
   * also fail.
   */
  mtd->current_kernel = new_kernel;
  mtd->current_priority = new_prio;

  if (CGPT_KERNEL_ENTRY_NOT_FOUND == new_kernel) {
    VBDEBUG(("GptNextKernelEntry no more kernels\n"));
    return GPT_ERROR_NO_VALID_KERNEL;
  }

  VBDEBUG(("GptNextKernelEntry likes partition %d\n", new_kernel + 1));
  e = entries + new_kernel;
  *start_sector = e->starting_lba;
  *size = e->ending_lba - e->starting_lba + 1;
  return GPT_SUCCESS;
}

int MtdUpdateKernelEntry(MtdData *mtd, uint32_t update_type)
{
  MtdDiskLayout *header = &mtd->primary;
  MtdDiskPartition *entries = header->partitions;
  MtdDiskPartition *e = entries + mtd->current_kernel;
  int modified = 0;

  if (mtd->current_kernel == CGPT_KERNEL_ENTRY_NOT_FOUND)
    return GPT_ERROR_INVALID_UPDATE_TYPE;
  if (!MtdIsKernelEntry(e))
    return GPT_ERROR_INVALID_UPDATE_TYPE;

  switch (update_type) {
  case GPT_UPDATE_ENTRY_TRY: {
    /* Used up a try */
    int tries;
    if (MtdGetEntrySuccessful(e)) {
      /*
       * Successfully booted this partition, so tries field
       * is ignored.
       */
      return GPT_SUCCESS;
    }
    tries = MtdGetEntryTries(e);
    if (tries > 1) {
      /* Still have tries left */
      modified = 1;
      MtdSetEntryTries(e, tries - 1);
      break;
    }
    /* Out of tries, so drop through and mark partition bad. */
  }
  case GPT_UPDATE_ENTRY_BAD: {
    /* Giving up on this partition entirely. */
    if (!MtdGetEntrySuccessful(e)) {
      /*
       * Only clear tries and priority if the successful bit
       * is not set.
       */
      modified = 1;
      MtdSetEntryTries(e, 0);
      MtdSetEntryPriority(e, 0);
    }
    break;
  }
  default:
    return GPT_ERROR_INVALID_UPDATE_TYPE;
  }

  if (modified) {
    MtdModified(mtd);
  }

  return GPT_SUCCESS;
}
