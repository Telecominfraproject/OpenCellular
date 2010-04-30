/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgpt.h"
#include <string.h>
#include "cgpt_internal.h"
#include "crc32.h"
#include "gpt.h"
#include "quick_sort.h"
#include "utility.h"

/* Macro to invalidate a GPT header/entries */
#define INVALIDATE_HEADER(valid_headers, index) \
  do { \
    valid_headers &= ~(1<<index); \
  } while (0)
#define INVALIDATE_ENTRIES(valid_entries, index) \
  do { \
    valid_entries &= ~(1<<index); \
  } while (0)

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
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (Memcmp(headers[i]->signature,
               GPT_HEADER_SIGNATURE,
               GPT_HEADER_SIGNATURE_SIZE)) {
      INVALIDATE_HEADER(valid_headers, i);
    }
  }
  return valid_headers;
}

/* The header revision should be GPT_HEADER_REVISION. */
uint32_t CheckRevision(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (headers[i]->revision != GPT_HEADER_REVISION)
      INVALIDATE_HEADER(valid_headers, i);
  }
  return valid_headers;
}

/* A valid header size should be between MIN_SIZE_OF_HEADER and
 * MAX_SIZE_OF_HEADER. */
uint32_t CheckSize(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if ((headers[i]->size < MIN_SIZE_OF_HEADER) ||
        (headers[i]->size > MAX_SIZE_OF_HEADER))
      INVALIDATE_HEADER(valid_headers, i);
  }
  return valid_headers;
}

/* Reserved and padding fields should be zero. */
uint32_t CheckReservedFields(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (headers[i]->reserved || headers[i]->padding)
      INVALIDATE_HEADER(valid_headers, i);
  }
  return valid_headers;
}

/* my_lba field points to the header itself.
 * So that the my_lba of primary header should be 1 (right after PMBR).
 * The my_lba of secondary header should be the last secotr on drive. */
uint32_t CheckMyLba(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  if (primary_header->my_lba != GPT_PMBR_SECTOR)  /* 2nd sector on drive */
    INVALIDATE_HEADER(valid_headers, PRIMARY);
  if (secondary_header->my_lba != (gpt->drive_sectors - 1))  /* last sector */
    INVALIDATE_HEADER(valid_headers, SECONDARY);
  return valid_headers;
}

/* SizeOfPartitionEntry must be between MIN_SIZE_OF_ENTRY and
 * MAX_SIZE_OF_ENTRY, and a multiple of SIZE_OF_ENTRY_MULTIPLE. */
uint32_t CheckSizeOfPartitionEntry(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
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
      INVALIDATE_HEADER(valid_headers, i);
  }
  return valid_headers;
}

/* number_of_entries must be between MIN_NUMBER_OF_ENTRIES and
 * MAX_NUMBER_OF_ENTRIES, and size_of_entry * number_of_entries must be
 * equal to TOTAL_ENTRIES_SIZE. */
uint32_t CheckNumberOfEntries(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
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
      INVALIDATE_HEADER(valid_headers, i);
  }
  return valid_headers;
}

/* Make sure entries_lba is correct.
 *   2 for primary entries
 *   drive_sectors-1-GPT_ENTRIES_SECTORS for secondary entries. */
uint32_t CheckEntriesLba(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  /* We assume the primary partition entry table is located at the sector
   * right after primary partition header. */
  if (primary_header->entries_lba != (GPT_PMBR_SECTOR + GPT_HEADER_SECTOR))
    INVALIDATE_HEADER(valid_headers, PRIMARY);
  /* We assume the secondary partition entry table is the 32 sectors
   * right before the secondary partition header. */
  if (secondary_header->entries_lba !=
      (gpt->drive_sectors - 1 - GPT_ENTRIES_SECTORS))
    INVALIDATE_HEADER(valid_headers, SECONDARY);
  return valid_headers;
}

/* FirstUsableLBA must be after the end of the primary GPT table array.
 * LastUsableLBA must be before the start of the secondary GPT table array.
 * FirstUsableLBA <= LastUsableLBA. */
uint32_t CheckValidUsableLbas(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
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
      INVALIDATE_HEADER(valid_headers, i);
    if (headers[i]->last_usable_lba >= start_of_secondary_entries)
      INVALIDATE_HEADER(valid_headers, i);
    if (headers[i]->first_usable_lba > headers[i]->last_usable_lba)
      INVALIDATE_HEADER(valid_headers, i);
  }

  if (headers[PRIMARY]->first_usable_lba - headers[PRIMARY]->entries_lba <
      GPT_ENTRIES_SECTORS)
    INVALIDATE_HEADER(valid_headers, PRIMARY);
  if (headers[SECONDARY]->last_usable_lba >= headers[SECONDARY]->entries_lba)
    INVALIDATE_HEADER(valid_headers, SECONDARY);

  return valid_headers;
}

/* Checks header CRC */
uint32_t CheckHeaderCrc(GptData *gpt) {
  uint32_t crc32, original_crc32;
  uint32_t valid_headers = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    original_crc32 = headers[i]->header_crc32;
    headers[i]->header_crc32 = 0;
    crc32 = Crc32((const uint8_t *)headers[i], headers[i]->size);
    headers[i]->header_crc32 = original_crc32;
    if (crc32 != original_crc32)
      INVALIDATE_HEADER(valid_headers, i);
  }
  return valid_headers;
}

/* Checks entries CRC */
uint32_t CheckEntriesCrc(GptData *gpt) {
  uint32_t crc32;
  uint32_t valid_entries = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  GptEntry *entries[] = {
    (GptEntry*)gpt->primary_entries,
    (GptEntry*)gpt->secondary_entries,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    crc32 = Crc32((const uint8_t *)entries[i], TOTAL_ENTRIES_SIZE);
    if (crc32 != headers[i]->entries_crc32)
      INVALIDATE_HEADER(valid_entries, i);
  }
  return valid_entries;
}

/* Returns non-zero if the given GUID is non-zero. */
static int NonZeroGuid(const Guid *guid) {
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
  int copy, entry_index;
  GptEntry *entry;

  for (copy = PRIMARY; copy <= SECONDARY; ++copy) {
    for (entry_index = 0;
         entry_index < headers[copy]->number_of_entries;
         ++entry_index) {
      entry = (GptEntry*)&(((uint8_t*)entries[copy])
          [entry_index * headers[copy]->size_of_entry]);
      if (NonZeroGuid(&entry->type)) {
        if ((entry->starting_lba < headers[copy]->first_usable_lba) ||
            (entry->ending_lba > headers[copy]->last_usable_lba) ||
            (entry->ending_lba < entry->starting_lba))
          INVALIDATE_HEADER(valid_entries, copy);
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
  uint32_t valid_entries = MASK_BOTH;
  GptHeader *headers[] = {
    (GptHeader*)gpt->primary_header,
    (GptHeader*)gpt->secondary_header,
  };
  GptEntry *entries[] = {
    (GptEntry*)gpt->primary_entries,
    (GptEntry*)gpt->secondary_entries,
  };
  int i;

  for (i = PRIMARY; i <= SECONDARY; ++i) {
    if (OverlappedEntries(entries[i], headers[i]->number_of_entries))
      INVALIDATE_ENTRIES(valid_entries, i);
  }
  return valid_entries;
}

/* Primary entries and secondary entries should be bitwise identical.
 * If two entries tables are valid, compare them. If not the same,
 * overwrites secondary with primary (primary always has higher priority),
 * and marks secondary as modified.
 * If only one is valid, overwrites invalid one.
 * If all are invalid, does nothing.
 * This function returns bit masks for GptData.modified field. */
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
 * Note that CRC value is not re-computed in this function. UpdateCrc() will
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
  if (gpt->modified & GPT_MODIFIED_ENTRIES1) {
    primary_header->entries_crc32 =
        Crc32(gpt->primary_entries, TOTAL_ENTRIES_SIZE);
  }
  if (gpt->modified & GPT_MODIFIED_ENTRIES2) {
    secondary_header->entries_crc32 =
        Crc32(gpt->secondary_entries, TOTAL_ENTRIES_SIZE);
  }
}

/* Does every sanity check, and returns if any header/entries needs to be
 * written back. */
int GptInit(GptData *gpt) {
  uint32_t valid_headers = MASK_BOTH;
  uint32_t valid_entries = MASK_BOTH;
  int retval;

  retval = CheckParameters(gpt);
  if (retval != GPT_SUCCESS)
    return retval;

  /* Initialize values */
  gpt->modified = 0;

  /* Start checking if header parameters are valid. */
  valid_headers &= CheckHeaderSignature(gpt);
  valid_headers &= CheckRevision(gpt);
  valid_headers &= CheckSize(gpt);
  valid_headers &= CheckReservedFields(gpt);
  valid_headers &= CheckMyLba(gpt);
  valid_headers &= CheckSizeOfPartitionEntry(gpt);
  valid_headers &= CheckNumberOfEntries(gpt);
  valid_headers &= CheckEntriesLba(gpt);
  valid_headers &= CheckValidUsableLbas(gpt);

  /* Checks if headers are valid. */
  valid_headers &= CheckHeaderCrc(gpt);
  gpt->modified |= RepairHeader(gpt, valid_headers);

  /* Checks if entries are valid. */
  valid_entries &= CheckEntriesCrc(gpt);
  valid_entries &= CheckValidEntries(gpt);
  valid_entries &= CheckOverlappedPartition(gpt);
  gpt->modified |= RepairEntries(gpt, valid_entries);

  /* Returns error if we don't have any valid header/entries to use. */
  if (!valid_headers)
    return GPT_ERROR_INVALID_HEADERS;
  if (!valid_entries)
    return GPT_ERROR_INVALID_ENTRIES;

  UpdateCrc(gpt);

  /* FIXME: will remove the next line soon. */
  gpt->current_kernel = 1;
  return GPT_SUCCESS;
}

/* stub code */
static int start[] = { 34, 10034 };

int GptNextKernelEntry(GptData *gpt, uint64_t *start_sector, uint64_t *size) {
  /* FIXME: the following code is not really code, just returns anything */
  gpt->current_kernel ^= 1;
  if (start_sector) *start_sector = start[gpt->current_kernel];
  if (size) *size = 10000;
  return GPT_SUCCESS;
}

int GptUpdateKernelEntry(GptData *gpt, uint32_t update_type) {
  /* FIXME: the following code is not really code, just return anything */
  gpt->modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1) <<
                   gpt->current_kernel;
  return GPT_SUCCESS;
}
