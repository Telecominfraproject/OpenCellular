/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions to fix, after cgptlib cleanup.
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "cgptlib_internal.h"
#include "cgpt_tofix.h"
#include "crc32.h"
#include "utility.h"

const char *GptError(int errno) {
  const char *error_string[] = {
    /* GPT_SUCCESS */ "Success",
    /* GPT_ERROR_NO_VALID_KERNEL */ "No valid kernel entry",
    /* GPT_ERROR_INVALID_HEADERS */ "Both primary and secondary headers are "
                                    "invalid.",
    /* GPT_ERROR_INVALID_ENTRIES */ "Both primary and secondary entries are "
                                    "invalid.",
    /* GPT_ERROR_INVALID_SECTOR_SIZE */ "Invalid sector size",
    /* GPT_ERROR_INVALID_SECTOR_NUMBER */ "Invalid sector number",
    /* GPT_ERROR_INVALID_UPDATE_TYPE */ "Invalid update type",
  };
  return error_string[errno];
}


/*  Update CRC value if necessary.  */
void UpdateCrc(GptData *gpt) {
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  if (gpt->modified & GPT_MODIFIED_ENTRIES1) {
    primary_header->entries_crc32 =
        Crc32(gpt->primary_entries, TOTAL_ENTRIES_SIZE);
  }
  if (gpt->modified & GPT_MODIFIED_ENTRIES2) {
    secondary_header->entries_crc32 =
        Crc32(gpt->secondary_entries, TOTAL_ENTRIES_SIZE);
  }
  if (gpt->modified & GPT_MODIFIED_HEADER1) {
    primary_header->header_crc32 = 0;
    primary_header->header_crc32 = Crc32(
        (const uint8_t *)primary_header, primary_header->size);
  }
  if (gpt->modified & GPT_MODIFIED_HEADER2) {
    secondary_header->header_crc32 = 0;
    secondary_header->header_crc32 = Crc32(
        (const uint8_t *)secondary_header, secondary_header->size);
  }
}

/* Helper function to get a pointer to the partition entry.
 *   'secondary' is either PRIMARY or SECONDARY.
 *   'entry_index' is the partition index: [0, number_of_entries).
 */
GptEntry *GetEntry(GptData *gpt, int secondary, int entry_index) {
  uint8_t *entries;

  if (secondary == PRIMARY) {
    entries = gpt->primary_entries;
  } else {
    entries = gpt->secondary_entries;
  }

  return (GptEntry*)(&entries[GetNumberOfEntries(gpt) * entry_index]);
}

/* The following functions are helpers to access attributes bit more easily.
 *   'secondary' is either PRIMARY or SECONDARY.
 *   'entry_index' is the partition index: [0, number_of_entries).
 *
 * Get*() return the exact value (shifted and masked).
 */
void SetPriority(GptData *gpt, int secondary, int entry_index, int priority) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);

  assert(priority >= 0 && priority <= CGPT_ATTRIBUTE_MAX_PRIORITY);
  entry->attributes &= ~CGPT_ATTRIBUTE_PRIORITY_MASK;
  entry->attributes |= (uint64_t)priority << CGPT_ATTRIBUTE_PRIORITY_OFFSET;
}

int GetPriority(GptData *gpt, int secondary, int entry_index) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);
  return (entry->attributes & CGPT_ATTRIBUTE_PRIORITY_MASK) >>
         CGPT_ATTRIBUTE_PRIORITY_OFFSET;
}

void SetBad(GptData *gpt, int secondary, int entry_index, int bad) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);

  // There is no bad attribute
  assert(0);
}

int GetBad(GptData *gpt, int secondary, int entry_index) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);

  // There is no bad attribute
  assert(0);
  return 0;
}

void SetTries(GptData *gpt, int secondary, int entry_index, int tries) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);

  assert(tries >= 0 && tries <= CGPT_ATTRIBUTE_MAX_TRIES);
  entry->attributes &= ~CGPT_ATTRIBUTE_TRIES_MASK;
  entry->attributes |= (uint64_t)tries << CGPT_ATTRIBUTE_TRIES_OFFSET;
}

int GetTries(GptData *gpt, int secondary, int entry_index) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);
  return (entry->attributes & CGPT_ATTRIBUTE_TRIES_MASK) >>
         CGPT_ATTRIBUTE_TRIES_OFFSET;
}

void SetSuccessful(GptData *gpt, int secondary, int entry_index, int success) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);

  assert(success >= 0 && success <= CGPT_ATTRIBUTE_MAX_SUCCESSFUL);
  entry->attributes &= ~CGPT_ATTRIBUTE_SUCCESSFUL_MASK;
  entry->attributes |= (uint64_t)success << CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET;
}

int GetSuccessful(GptData *gpt, int secondary, int entry_index) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);
  return (entry->attributes & CGPT_ATTRIBUTE_SUCCESSFUL_MASK) >>
         CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET;
}

uint32_t GetNumberOfEntries(const GptData *gpt) {
  GptHeader *header = 0;
  if (gpt->valid_headers & MASK_PRIMARY)
    header = (GptHeader*)gpt->primary_header;
  else if (gpt->valid_headers & MASK_SECONDARY)
    header = (GptHeader*)gpt->secondary_header;
  else
    assert(0);
  return header->number_of_entries;
}

/* Two headers are NOT bitwise identical. For example, my_lba pointers to header
 * itself so that my_lba in primary and secondary is definitely different.
 * Only the following fields should be identical.
 *
 *   first_usable_lba
 *   last_usable_lba
 *   number_of_entries
 *   size_of_entry
 *   disk_uuid
 *
 * If any of above field are not matched, overwrite secondary with primary since
 * we always trust primary.
 * If any one of header is invalid, copy from another. */
int IsSynonymous(const GptHeader* a, const GptHeader* b) {
  if ((a->first_usable_lba == b->first_usable_lba) &&
      (a->last_usable_lba == b->last_usable_lba) &&
      (a->number_of_entries == b->number_of_entries) &&
      (a->size_of_entry == b->size_of_entry) &&
      (!Memcmp(&a->disk_uuid, &b->disk_uuid, sizeof(Guid))))
    return 1;
  return 0;
}

/* Primary entries and secondary entries should be bitwise identical.
 * If two entries tables are valid, compare them. If not the same,
 * overwrites secondary with primary (primary always has higher priority),
 * and marks secondary as modified.
 * If only one is valid, overwrites invalid one.
 * If all are invalid, does nothing.
 * This function returns bit masks for GptData.modified field.
 * Note that CRC is NOT re-computed in this function.
 */
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries) {
  if (valid_entries == MASK_BOTH) {
    if (Memcmp(gpt->primary_entries, gpt->secondary_entries,
               TOTAL_ENTRIES_SIZE)) {
      Memcpy(gpt->secondary_entries, gpt->primary_entries, TOTAL_ENTRIES_SIZE);
      return GPT_MODIFIED_ENTRIES2;
    }
  } else if (valid_entries == MASK_PRIMARY) {
    Memcpy(gpt->secondary_entries, gpt->primary_entries, TOTAL_ENTRIES_SIZE);
    return GPT_MODIFIED_ENTRIES2;
  } else if (valid_entries == MASK_SECONDARY) {
    Memcpy(gpt->primary_entries, gpt->secondary_entries, TOTAL_ENTRIES_SIZE);
    return GPT_MODIFIED_ENTRIES1;
  }

  return 0;
}

/* The above five fields are shared between primary and secondary headers.
 * We can recover one header from another through copying those fields. */
void CopySynonymousParts(GptHeader* target, const GptHeader* source) {
  target->first_usable_lba = source->first_usable_lba;
  target->last_usable_lba = source->last_usable_lba;
  target->number_of_entries = source->number_of_entries;
  target->size_of_entry = source->size_of_entry;
  Memcpy(&target->disk_uuid, &source->disk_uuid, sizeof(Guid));
}

/* This function repairs primary and secondary headers if possible.
 * If both headers are valid (CRC32 is correct) but
 *   a) indicate inconsistent usable LBA ranges,
 *   b) inconsistent partition entry size and number,
 *   c) inconsistent disk_uuid,
 * we will use the primary header to overwrite secondary header.
 * If primary is invalid (CRC32 is wrong), then we repair it from secondary.
 * If secondary is invalid (CRC32 is wrong), then we repair it from primary.
 * This function returns the bitmasks for modified header.
 * Note that CRC value is NOT re-computed in this function. UpdateCrc() will
 * do it later.
 */
uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers) {
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  if (valid_headers == MASK_BOTH) {
    if (!IsSynonymous(primary_header, secondary_header)) {
      CopySynonymousParts(secondary_header, primary_header);
      return GPT_MODIFIED_HEADER2;
    }
  } else if (valid_headers == MASK_PRIMARY) {
    Memcpy(secondary_header, primary_header, primary_header->size);
    secondary_header->my_lba = gpt->drive_sectors - 1;  /* the last sector */
    secondary_header->entries_lba = secondary_header->my_lba -
        GPT_ENTRIES_SECTORS;
    return GPT_MODIFIED_HEADER2;
  } else if (valid_headers == MASK_SECONDARY) {
    Memcpy(primary_header, secondary_header, secondary_header->size);
    primary_header->my_lba = GPT_PMBR_SECTOR;  /* the second sector on drive */
    primary_header->entries_lba = primary_header->my_lba + GPT_HEADER_SECTOR;
    return GPT_MODIFIED_HEADER1;
  }

  return 0;
}

/* TODO: HORRIBLY broken non-implemented functions.  These will be
 * fixed as part of a second stage of refactoring to use the new
 * cgptlib_internal functions. */
uint32_t CheckOverlappedPartition(GptData *gpt) {
  return 1;
}

uint32_t CheckValidEntries(GptData *gpt) {
  return 1;
}

int NonZeroGuid(const Guid *guid) {
  return 1;
}
