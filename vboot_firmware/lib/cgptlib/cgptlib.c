/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgptlib.h"
#include <string.h>
#include "cgptlib_internal.h"
#include "crc32.h"
#include "gpt.h"
#include "quick_sort.h"
#include "utility.h"

/* Macro to invalidate a GPT header/entries */
#define INVALIDATE_HEADER(valid_headers, index) \
  do { \
    debug("- INVALIDATE_HEADER() at %s():%d\n", __FUNCTION__, __LINE__); \
    valid_headers &= ~(1<<index); \
  } while (0)
#define INVALIDATE_ENTRIES(valid_entries, index) \
  do { \
    debug("- INVALIDATE_ENTRIES() at %s():%d\n", __FUNCTION__, __LINE__); \
    valid_entries &= ~(1<<index); \
  } while (0)

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

/* Checks if sector_bytes and drive_sectors are valid values. */
int CheckParameters(GptData *gpt) {
  /* Currently, we only support 512-byte sector. In the future, we may support
   * larger sector. */
  if (gpt->sector_bytes != 512)
    return GPT_ERROR_INVALID_SECTOR_SIZE;

  /* The sector number of a drive should be reasonable. If the given value is
   * too small to contain basic GPT structure (PMBR + Headers + Entries),
   * the value is wrong. */
  if (gpt->drive_sectors < (GPT_PMBR_SECTOR +
                            GPT_HEADER_SECTOR * 2 +
                            GPT_ENTRIES_SECTORS * 2))
    return GPT_ERROR_INVALID_SECTOR_NUMBER;

  return GPT_SUCCESS;
}

/* Expects header signature should be GPT_HEADER_SIGNATURE. */
uint32_t CheckHeaderSignature(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (Memcmp(headers[i]->signature,
               GPT_HEADER_SIGNATURE,
               GPT_HEADER_SIGNATURE_SIZE)) {
      INVALIDATE_HEADER(gpt->valid_headers, i);
    }
  }
  return gpt->valid_headers;
}

/* The header revision should be GPT_HEADER_REVISION. */
uint32_t CheckRevision(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (headers[i]->revision != GPT_HEADER_REVISION)
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }
  return gpt->valid_headers;
}

/* A valid header size should be between MIN_SIZE_OF_HEADER and
 * MAX_SIZE_OF_HEADER. */
uint32_t CheckSize(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if ((headers[i]->size < MIN_SIZE_OF_HEADER) ||
        (headers[i]->size > MAX_SIZE_OF_HEADER))
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }
  return gpt->valid_headers;
}

/* Reserved and padding fields should be zero. */
uint32_t CheckReservedFields(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (headers[i]->reserved || headers[i]->padding)
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }
  return gpt->valid_headers;
}

/* my_lba field points to the header itself.
 * So that the my_lba of primary header should be 1 (right after PMBR).
 * The my_lba of secondary header should be the last secotr on drive. */
uint32_t CheckMyLba(GptData *gpt) {
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  if (primary_header->my_lba != GPT_PMBR_SECTOR)  /* 2nd sector on drive */
    INVALIDATE_HEADER(gpt->valid_headers, PRIMARY);
  if (secondary_header->my_lba != (gpt->drive_sectors - 1))  /* last sector */
    INVALIDATE_HEADER(gpt->valid_headers, SECONDARY);
  return gpt->valid_headers;
}

/* SizeOfPartitionEntry must be between MIN_SIZE_OF_ENTRY and
 * MAX_SIZE_OF_ENTRY, and a multiple of SIZE_OF_ENTRY_MULTIPLE. */
uint32_t CheckSizeOfPartitionEntry(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    uint32_t size_of_entry = headers[i]->size_of_entry;
    if ((size_of_entry < MIN_SIZE_OF_ENTRY) ||
        (size_of_entry > MAX_SIZE_OF_ENTRY) ||
        (size_of_entry & (SIZE_OF_ENTRY_MULTIPLE - 1)))
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }
  return gpt->valid_headers;
}

/* number_of_entries must be between MIN_NUMBER_OF_ENTRIES and
 * MAX_NUMBER_OF_ENTRIES, and size_of_entry * number_of_entries must be
 * equal to TOTAL_ENTRIES_SIZE. */
uint32_t CheckNumberOfEntries(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    uint32_t number_of_entries = headers[i]->number_of_entries;
    if ((number_of_entries < MIN_NUMBER_OF_ENTRIES) ||
        (number_of_entries > MAX_NUMBER_OF_ENTRIES) ||
        (number_of_entries * headers[i]->size_of_entry != TOTAL_ENTRIES_SIZE))
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }
  return gpt->valid_headers;
}

/* Make sure entries_lba is correct.
 *   2 for primary entries
 *   drive_sectors-1-GPT_ENTRIES_SECTORS for secondary entries. */
uint32_t CheckEntriesLba(GptData *gpt) {
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  /* We assume the primary partition entry table is located at the sector
   * right after primary partition header. */
  if (primary_header->entries_lba != (GPT_PMBR_SECTOR + GPT_HEADER_SECTOR))
    INVALIDATE_HEADER(gpt->valid_headers, PRIMARY);
  /* We assume the secondary partition entry table is the 32 sectors
   * right before the secondary partition header. */
  if (secondary_header->entries_lba !=
      (gpt->drive_sectors - 1 - GPT_ENTRIES_SECTORS))
    INVALIDATE_HEADER(gpt->valid_headers, SECONDARY);
  return gpt->valid_headers;
}

/* FirstUsableLBA must be after the end of the primary GPT table array.
 * LastUsableLBA must be before the start of the secondary GPT table array.
 * FirstUsableLBA <= LastUsableLBA. */
uint32_t CheckValidUsableLbas(GptData *gpt) {
  uint64_t end_of_primary_entries;
  uint64_t start_of_secondary_entries;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  end_of_primary_entries = GPT_PMBR_SECTOR + GPT_HEADER_SECTOR +
                           GPT_ENTRIES_SECTORS;
  start_of_secondary_entries = (gpt->drive_sectors - 1 - GPT_ENTRIES_SECTORS);

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (headers[i]->first_usable_lba < end_of_primary_entries)
      INVALIDATE_HEADER(gpt->valid_headers, i);
    if (headers[i]->last_usable_lba >= start_of_secondary_entries)
      INVALIDATE_HEADER(gpt->valid_headers, i);
    if (headers[i]->first_usable_lba > headers[i]->last_usable_lba)
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }

  if (headers[PRIMARY]->first_usable_lba - headers[PRIMARY]->entries_lba <
      GPT_ENTRIES_SECTORS)
    INVALIDATE_HEADER(gpt->valid_headers, PRIMARY);
  if (headers[SECONDARY]->last_usable_lba >= headers[SECONDARY]->entries_lba)
    INVALIDATE_HEADER(gpt->valid_headers, SECONDARY);

  return gpt->valid_headers;
}

/* Checks header CRC */
uint32_t CheckHeaderCrc(GptData *gpt) {
  uint32_t crc32, original_crc32;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (!(gpt->valid_headers & (1 << i))) continue;
    original_crc32 = headers[i]->header_crc32;
    headers[i]->header_crc32 = 0;
    crc32 = Crc32((const uint8_t *)headers[i], headers[i]->size);
    headers[i]->header_crc32 = original_crc32;
    if (crc32 != original_crc32)
      INVALIDATE_HEADER(gpt->valid_headers, i);
  }
  return gpt->valid_headers;
}

/* Checks entries CRC */
uint32_t CheckEntriesCrc(GptData *gpt) {
  uint32_t crc32;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  GptEntry *entries[] = {
    (GptEntry*)gpt->primary_entries,
    (GptEntry*)gpt->secondary_entries,
  };
  uint32_t entries_crc32;
  int i;

  if (gpt->valid_headers & MASK_PRIMARY)
    entries_crc32 = headers[PRIMARY]->entries_crc32;
  else
    entries_crc32 = headers[SECONDARY]->entries_crc32;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    crc32 = Crc32((const uint8_t *)entries[i], TOTAL_ENTRIES_SIZE);
    if (crc32 != entries_crc32)
      INVALIDATE_ENTRIES(gpt->valid_entries, i);
  }
  return gpt->valid_entries;
}

/* Returns non-zero if the given GUID is non-zero. */
int NonZeroGuid(const Guid *guid) {
  static Guid zero = {{{0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}}}};
  return Memcmp(&zero, guid, sizeof(zero));
}

/* Checks if entries geometry is valid.
 * All active (non-zero PartitionTypeGUID) partition entries should have:
 *   entry.StartingLBA >= header.FirstUsableLBA
 *   entry.EndingLBA <= header.LastUsableLBA
 *   entry.StartingLBA <= entry.EndingLBA
 */
uint32_t CheckValidEntries(GptData *gpt) {
  uint32_t valid_entries = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  GptEntry *entries[] = {
    (GptEntry*)gpt->primary_entries,
    (GptEntry*)gpt->secondary_entries,
  };
  uint32_t number_of_entries, size_of_entry;
  uint64_t first_usable_lba, last_usable_lba;
  int copy, entry_index;
  GptEntry *entry;

  if (gpt->valid_headers & MASK_PRIMARY)
    copy = PRIMARY;
  else
    copy = SECONDARY;
  number_of_entries = headers[copy]->number_of_entries;
  size_of_entry = headers[copy]->size_of_entry;
  first_usable_lba = headers[copy]->first_usable_lba;
  last_usable_lba = headers[copy]->last_usable_lba;

  for (copy = PRIMARY; copy <= SECONDARY; ++copy) {
    for (entry_index = 0;
         entry_index < number_of_entries;
         ++entry_index) {
      entry = (GptEntry*)&(((uint8_t*)entries[copy])
          [entry_index * size_of_entry]);
      if (NonZeroGuid(&entry->type)) {
        if ((entry->starting_lba < first_usable_lba) ||
            (entry->ending_lba > last_usable_lba) ||
            (entry->ending_lba < entry->starting_lba))
          INVALIDATE_ENTRIES(valid_entries, copy);
      }
    }
  }
  return valid_entries;
}

static pair_t pairs[MAX_NUMBER_OF_ENTRIES];
/* Callback function for QuickSort(). Returns 1 if 'a_' should precede 'b_'. */
int compare_pair(const void *a_, const void *b_) {
  const pair_t *a = a_;
  const pair_t *b = b_;
  if (a->starting <= b->starting) return 1;
  return 0;
}

/* First sorts by starting_lba, and traverse everyone once if its starting_lba
 * is between previous starting_lba and ending_lba. If yes, overlapped.
 * Returns 1 if overlap is found. */
int OverlappedEntries(GptEntry *entries, uint32_t number_of_entries) {
  int i, num_of_pair = 0;
  for (i = 0; i < number_of_entries; ++i) {
    if (NonZeroGuid(&entries[i].type)) {
      pairs[num_of_pair].starting = entries[i].starting_lba;
      pairs[num_of_pair].ending = entries[i].ending_lba;
      ++num_of_pair;
    }
  }
  QuickSort(&pairs, num_of_pair, sizeof(pair_t), compare_pair);

  for (i = 1; i < num_of_pair; ++i) {
    if ((pairs[i].starting >= pairs[i-1].starting) &&
        (pairs[i].starting <= pairs[i-1].ending))
      return 1;
  }

  return 0;
}

/* Checks if any two partitions are overlapped in primary and secondary entries.
 */
uint32_t CheckOverlappedPartition(GptData *gpt) {
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  GptEntry *entries[] = {
    (GptEntry*)gpt->primary_entries,
    (GptEntry*)gpt->secondary_entries,
  };
  int i;
  uint32_t number_of_entries;

  if (gpt->valid_headers & MASK_PRIMARY)
    number_of_entries = headers[PRIMARY]->number_of_entries;
  else
    number_of_entries = headers[SECONDARY]->number_of_entries;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (OverlappedEntries(entries[i], number_of_entries))
      INVALIDATE_ENTRIES(gpt->valid_entries, i);
  }
  return gpt->valid_entries;
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

/* This function only checks GptData.
 * valid_headers and valid_entries are used to store the checking results.
 *
 * Returns:
 *   GPT_ERROR_INVALID_HEADERS -- both headers are invalid.
 *   GPT_ERROR_INVALID_ENTRIES -- both entries are invalid.
 *   GPT_SUCCESS -- everything looks fine.
 */
int GptSanityCheck(GptData *gpt) {
  int retval;

  assert(gpt);

  retval = CheckParameters(gpt);
  if (retval != GPT_SUCCESS)
    return retval;

  /* Initialize values */
  gpt->valid_headers = MASK_BOTH;
  gpt->valid_entries = MASK_BOTH;

  /* Start checking if header parameters are valid. */
  CheckHeaderSignature(gpt);
  CheckRevision(gpt);
  CheckSize(gpt);
  CheckReservedFields(gpt);
  CheckMyLba(gpt);
  CheckSizeOfPartitionEntry(gpt);
  CheckNumberOfEntries(gpt);
  CheckEntriesLba(gpt);
  CheckValidUsableLbas(gpt);
  CheckHeaderCrc(gpt);

  /* Returns error if we don't have any valid header to use. */
  if (!gpt->valid_headers)
    return GPT_ERROR_INVALID_HEADERS;

  /* Checks if entries are valid. */
  CheckEntriesCrc(gpt);
  CheckValidEntries(gpt);
  CheckOverlappedPartition(gpt);

  /* Returns error if we don't have any valid entries to use. */
  if (!gpt->valid_entries)
    return GPT_ERROR_INVALID_ENTRIES;

  return GPT_SUCCESS;
}

void GptRepair(GptData *gpt) {
  gpt->modified |= RepairHeader(gpt, gpt->valid_headers);
  gpt->modified |= RepairEntries(gpt, gpt->valid_entries);
  UpdateCrc(gpt);
}

/* Does every sanity check, and returns if any header/entries needs to be
 * written back. */
int GptInit(GptData *gpt) {
  int retval;

  retval = GptSanityCheck(gpt);
  if (GPT_SUCCESS != retval) return retval;

  gpt->modified = 0;
  GptRepair(gpt);

  gpt->current_kernel = CGPT_KERNEL_ENTRY_NOT_FOUND;

  return GPT_SUCCESS;
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

  assert(bad >= 0 && bad <= CGPT_ATTRIBUTE_MAX_BAD);
  entry->attributes &= ~CGPT_ATTRIBUTE_BAD_MASK;
  entry->attributes |= (uint64_t)bad << CGPT_ATTRIBUTE_BAD_OFFSET;
}

int GetBad(GptData *gpt, int secondary, int entry_index) {
  GptEntry *entry;
  entry = GetEntry(gpt, secondary, entry_index);
  return (entry->attributes & CGPT_ATTRIBUTE_BAD_MASK) >>
         CGPT_ATTRIBUTE_BAD_OFFSET;
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


/* Compare two priority values. Actually it is a circular priority, which is:
 * 3 > 2 > 1 > 0, but 0 > 3.  (-1 means very low, and anyone is higher than -1)
 *
 * Return 1 if 'a' has higher priority than 'b'.
 */
int IsHigherPriority(int a, int b) {
  if ((a == 0) && (b == CGPT_ATTRIBUTE_MAX_PRIORITY))
    return 1;
  else if ((a == CGPT_ATTRIBUTE_MAX_PRIORITY) && (b == 0))
    return 0;
  else
    return (a > b) ? 1 : 0;
}

/* This function walks through the whole partition table (see note below),
 * and pick up the active and valid (not marked as bad) kernel entry with
 * *highest* priority (except gpt->current_kernel itself).
 *
 * Returns start_sector and its size if a candidate kernel is found.
 *
 * Note: in the first walk (gpt->current_kernel==CGPT_KERNEL_ENTRY_NOT_FOUND),
 *       the scan range is whole table. But in later scans, we only scan
 *       (header->number_of_entries - 1) entries because we are looking for
 *       next kernel with lower priority (consider the case that highest
 *       priority kernel is still active and valid).
 */
int GptNextKernelEntry(GptData *gpt, uint64_t *start_sector, uint64_t *size) {
  GptHeader *header;
  GptEntry *entry;
  int scan, current_priority;
  int begin, end;  /* [begin, end], which end is included. */
  Guid chromeos_kernel = GPT_ENT_TYPE_CHROMEOS_KERNEL;

  header = (GptHeader*)gpt->primary_header;
  current_priority = -1;  /* pretty low priority */
  if (gpt->current_kernel == CGPT_KERNEL_ENTRY_NOT_FOUND) {
    begin = 0;
    end = header->number_of_entries - 1;
  } else {
    begin = (gpt->current_kernel + 1) % header->number_of_entries;
    end = (gpt->current_kernel - 1 + header->number_of_entries) %
          header->number_of_entries;
  }

  scan = begin;
  do {
    entry = GetEntry(gpt, PRIMARY, scan);
    if (!Memcmp(&entry->type, &chromeos_kernel, sizeof(Guid)) &&
        !GetBad(gpt, PRIMARY, scan) &&
        ((gpt->current_kernel == CGPT_KERNEL_ENTRY_NOT_FOUND) ||
         (IsHigherPriority(GetPriority(gpt, PRIMARY, scan),
                           current_priority)))) {
      gpt->current_kernel = scan;
      current_priority = GetPriority(gpt, PRIMARY, gpt->current_kernel);
    }

    if (scan == end) break;
    scan = (scan + 1) % header->number_of_entries;
  } while (1);

  if (gpt->current_kernel == CGPT_KERNEL_ENTRY_NOT_FOUND)
    return GPT_ERROR_NO_VALID_KERNEL;

  entry = GetEntry(gpt, PRIMARY, gpt->current_kernel);
  assert(entry->starting_lba <= entry->ending_lba);

  if (start_sector) *start_sector = entry->starting_lba;
  if (size) *size = entry->ending_lba - entry->starting_lba + 1;

  return GPT_SUCCESS;
}

/* Given a update_type, this function updates the corresponding bits in GptData.
 *
 * Returns GPT_SUCCESS if no error. gpt->modified is set if any header and
 *                     entries needs to be updated to hard drive.
 *         GPT_ERROR_INVALID_UPDATE_TYPE if given an invalid update_type.
 */
int GptUpdateKernelEntry(GptData *gpt, uint32_t update_type) {
  Guid chromeos_type = GPT_ENT_TYPE_CHROMEOS_KERNEL;
  int primary_is_modified = 0;

  assert(gpt->current_kernel != CGPT_KERNEL_ENTRY_NOT_FOUND);
  assert(!Memcmp(&(GetEntry(gpt, PRIMARY, gpt->current_kernel)->type),
                 &chromeos_type, sizeof(Guid)));

  /* Modify primary entries first, then copy to secondary later. */
  switch (update_type) {
    case GPT_UPDATE_ENTRY_TRY: {
        /* Increase tries value until CGPT_ATTRIBUTE_MAX_TRIES. */
        int tries;
        tries = GetTries(gpt, PRIMARY, gpt->current_kernel);
        if (tries < CGPT_ATTRIBUTE_MAX_TRIES) {
          ++tries;
          SetTries(gpt, PRIMARY, gpt->current_kernel, tries);
          primary_is_modified = 1;
        }
      break;
    }
    case GPT_UPDATE_ENTRY_BAD: {
      GetEntry(gpt, PRIMARY, gpt->current_kernel)->attributes |=
          CGPT_ATTRIBUTE_BAD_MASK;
      primary_is_modified = 1;
      break;
    }
    default: {
      return GPT_ERROR_INVALID_UPDATE_TYPE;
    }
  }

  if (primary_is_modified) {
    /* Claim only primary is valid so that secondary is overwritten. */
    RepairEntries(gpt, MASK_PRIMARY);
    /* Actually two entries are dirty now.
    * Also two headers are dirty because entries_crc32 has been updated. */
    gpt->modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                      GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
    UpdateCrc(gpt);
  }

  return GPT_SUCCESS;
}
