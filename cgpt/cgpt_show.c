// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cgpt.h"

#define __STDC_FORMAT_MACROS

#include <string.h>

#include "cgptlib_internal.h"
#include "cgpt_params.h"
#include "crc32.h"

/* Generate output like:
 *
 *  [AB-CD-EF-01]   for group = 1
 *  [ABCD-EF01]     for group = 3  (low byte first)
 *
 * Needs (size*3-1+3) bytes of space in 'buf' (included the tailing '\0').
 */
#define BUFFER_SIZE(size) (size *3 - 1 + 3)
static short Uint8To2Chars(const uint8_t t) {
  int h = t >> 4;
  int l = t & 0xf;
  h = (h >= 0xA) ? h - 0xA + 'A' : h + '0';
  l = (l >= 0xA) ? l - 0xA + 'A' : l + '0';
  return (h << 8) + l;
}

static void RawDump(const uint8_t *memory, const int size,
                    char *buf, int group) {
  int i, outlen = 0;
  buf[outlen++] = '[';
  for (i = 0; i < size; ++i) {
    short c2 = Uint8To2Chars(memory[i]);
    buf[outlen++] = c2 >> 8;
    buf[outlen++] = c2 & 0xff;
    if (i != (size - 1) && ((i + 1) % group) == 0)
      buf[outlen++] = '-';
  }
  buf[outlen++] = ']';
  buf[outlen++] = '\0';
}

/* Output formatters */
#define TITLE_FMT      "%12s%12s%8s  %s\n"
#define GPT_FMT        "%12d%12d%8s  %s\n"
#define GPT_MORE       "%12s%12s%8s  ", "", "", ""
#define PARTITION_FMT  "%12d%12d%8d  %s\n"
#define PARTITION_MORE "%12s%12s%8s  %s%s\n", "", "", ""

static void HeaderDetails(GptHeader *header, GptEntry *entries,
                          const char *indent, int raw) {
  int i;

  printf("%sSig: ", indent);
  if (!raw) {
    printf("[");
    for (i = 0; i < sizeof(header->signature); ++i)
      printf("%c", header->signature[i]);
    printf("]");
  } else {
    char buf[BUFFER_SIZE(sizeof(header->signature))];
    RawDump((uint8_t *)header->signature, sizeof(header->signature), buf, 1);
    printf("%s", buf);
  }
  printf("\n");

  printf("%sRev: 0x%08x\n", indent, header->revision);
  printf("%sSize: %d\n", indent, header->size);
  printf("%sHeader CRC: 0x%08x %s\n", indent, header->header_crc32,
         (HeaderCrc(header) != header->header_crc32) ? "(INVALID)" : "");
  printf("%sMy LBA: %lld\n", indent, (long long)header->my_lba);
  printf("%sAlternate LBA: %lld\n", indent, (long long)header->alternate_lba);
  printf("%sFirst LBA: %lld\n", indent, (long long)header->first_usable_lba);
  printf("%sLast LBA: %lld\n", indent, (long long)header->last_usable_lba);

  {  /* For disk guid */
    char buf[GUID_STRLEN];
    GuidToStr(&header->disk_uuid, buf, GUID_STRLEN);
    printf("%sDisk UUID: %s\n", indent, buf);
  }

  printf("%sEntries LBA: %lld\n", indent, (long long)header->entries_lba);
  printf("%sNumber of entries: %d\n", indent, header->number_of_entries);
  printf("%sSize of entry: %d\n", indent, header->size_of_entry);
  printf("%sEntries CRC: 0x%08x %s\n", indent, header->entries_crc32,
         header->entries_crc32 !=
             Crc32((const uint8_t *)entries,header->size_of_entry *
                                            header->number_of_entries)
             ? "INVALID" : ""
         );
}

void EntryDetails(GptEntry *entry, uint32_t index, int raw) {
  char contents[256];                   // scratch buffer for formatting output
  uint8_t label[GPT_PARTNAME_LEN];

  if (!raw) {
    char type[GUID_STRLEN], unique[GUID_STRLEN];

    UTF16ToUTF8(entry->name, sizeof(entry->name) / sizeof(entry->name[0]),
                label, sizeof(label));
    require(snprintf(contents, sizeof(contents),
                     "Label: \"%s\"", label) < sizeof(contents));
    printf(PARTITION_FMT, (int)entry->starting_lba,
           (int)(entry->ending_lba - entry->starting_lba + 1),
           index+1, contents);
    if (CGPT_OK == ResolveType(&entry->type, type)) {
      printf(PARTITION_MORE, "Type: ", type);
    } else {
      GuidToStr(&entry->type, type, GUID_STRLEN);
      printf(PARTITION_MORE, "Type: ", type);
    }
    GuidToStr(&entry->unique, unique, GUID_STRLEN);
    printf(PARTITION_MORE, "UUID: ", unique);
    if (GuidEqual(&guid_chromeos_kernel, &entry->type)) {
      int tries = (entry->attrs.fields.gpt_att &
                   CGPT_ATTRIBUTE_TRIES_MASK) >>
          CGPT_ATTRIBUTE_TRIES_OFFSET;
      int successful = (entry->attrs.fields.gpt_att &
                        CGPT_ATTRIBUTE_SUCCESSFUL_MASK) >>
          CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET;
      int priority = (entry->attrs.fields.gpt_att &
                      CGPT_ATTRIBUTE_PRIORITY_MASK) >>
          CGPT_ATTRIBUTE_PRIORITY_OFFSET;
      require(snprintf(contents, sizeof(contents),
                       "priority=%d tries=%d successful=%d",
                       priority, tries, successful) < sizeof(contents));
      printf(PARTITION_MORE, "Attr: ", contents);
    }
  } else {
    char type[GUID_STRLEN], unique[GUID_STRLEN];

    UTF16ToUTF8(entry->name, sizeof(entry->name) / sizeof(entry->name[0]),
                label, sizeof(label));
    require(snprintf(contents, sizeof(contents),
                     "Label: \"%s\"", label) < sizeof(contents));
    printf(PARTITION_FMT, (int)entry->starting_lba,
           (int)(entry->ending_lba - entry->starting_lba + 1),
           index+1, contents);
    GuidToStr(&entry->type, type, GUID_STRLEN);
    printf(PARTITION_MORE, "Type: ", type);
    GuidToStr(&entry->unique, unique, GUID_STRLEN);
    printf(PARTITION_MORE, "UUID: ", unique);
    require(snprintf(contents, sizeof(contents),
                     "[%x]", entry->attrs.fields.gpt_att) < sizeof(contents));
    printf(PARTITION_MORE, "Attr: ", contents);
  }
}


void EntriesDetails(GptData *gpt, const int secondary, int raw) {
  uint32_t i;

  for (i = 0; i < GetNumberOfEntries(gpt); ++i) {
    GptEntry *entry;
    entry = GetEntry(gpt, secondary, i);

    if (IsZero(&entry->type))
      continue;

    EntryDetails(entry, i, raw);
  }
}

int cgpt_get_num_non_empty_partitions(CgptShowParams *params) {
  struct drive drive;
  int gpt_retval;
  int retval;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDONLY))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    retval = CGPT_FAILED;
    goto done;
  }

  params->num_partitions = 0;
  int numEntries = GetNumberOfEntries(&drive.gpt);
  int i;
  for(i = 0; i < numEntries; i++) {
      GptEntry *entry = GetEntry(&drive.gpt, ANY_VALID, i);
      if (IsZero(&entry->type))
        continue;

      params->num_partitions++;
  }

  retval = CGPT_OK;

done:
  DriveClose(&drive, 0);
  return retval;
}

int cgpt_show(CgptShowParams *params) {
  struct drive drive;
  int gpt_retval;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDONLY))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    return CGPT_FAILED;
  }

  if (params->partition) {                      // show single partition

    if (params->partition > GetNumberOfEntries(&drive.gpt)) {
      Error("invalid partition number: %d\n", params->partition);
      return CGPT_FAILED;
    }

    uint32_t index = params->partition - 1;
    GptEntry *entry = GetEntry(&drive.gpt, ANY_VALID, index);
    char buf[256];                      // scratch buffer for string conversion

    if (params->single_item) {
      switch(params->single_item) {
      case 'b':
        printf("%" PRId64 "\n", entry->starting_lba);
        break;
      case 's':
        printf("%" PRId64 "\n", entry->ending_lba - entry->starting_lba + 1);
        break;
      case 't':
        GuidToStr(&entry->type, buf, sizeof(buf));
        printf("%s\n", buf);
        break;
      case 'u':
        GuidToStr(&entry->unique, buf, sizeof(buf));
        printf("%s\n", buf);
        break;
      case 'l':
        UTF16ToUTF8(entry->name, sizeof(entry->name) / sizeof(entry->name[0]),
                    (uint8_t *)buf, sizeof(buf));
        printf("%s\n", buf);
        break;
      case 'S':
        printf("%d\n", GetSuccessful(&drive.gpt, ANY_VALID, index));
        break;
      case 'T':
        printf("%d\n", GetTries(&drive.gpt, ANY_VALID, index));
        break;
      case 'P':
        printf("%d\n", GetPriority(&drive.gpt, ANY_VALID, index));
        break;
      case 'A':
        printf("0x%x\n", entry->attrs.fields.gpt_att);
        break;
      }
    } else {
      printf(TITLE_FMT, "start", "size", "part", "contents");
      EntryDetails(entry, index, params->numeric);
    }

  } else if (params->quick) {                   // show all partitions, quickly
    uint32_t i;
    GptEntry *entry;
    char type[GUID_STRLEN];

    for (i = 0; i < GetNumberOfEntries(&drive.gpt); ++i) {
      entry = GetEntry(&drive.gpt, ANY_VALID, i);

      if (IsZero(&entry->type))
        continue;

      if (!params->numeric && CGPT_OK == ResolveType(&entry->type, type)) {
      } else {
        GuidToStr(&entry->type, type, GUID_STRLEN);
      }
      printf(PARTITION_FMT, (int)entry->starting_lba,
             (int)(entry->ending_lba - entry->starting_lba + 1),
             i+1, type);
    }
  } else {                              // show all partitions
    GptEntry *entries;

    if (CGPT_OK != ReadPMBR(&drive)) {
      Error("Unable to read PMBR\n");
      return CGPT_FAILED;
    }

    printf(TITLE_FMT, "start", "size", "part", "contents");
    char buf[256];                      // buffer for formatted PMBR content
    PMBRToStr(&drive.pmbr, buf, sizeof(buf)); // will exit if buf is too small
    printf(GPT_FMT, 0, GPT_PMBR_SECTOR, "", buf);

    if (drive.gpt.valid_headers & MASK_PRIMARY) {
      printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
             (int)GPT_HEADER_SECTOR, "", "Pri GPT header");
    } else {
      printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
             (int)GPT_HEADER_SECTOR, "INVALID", "Pri GPT header");
    }

    if (params->debug ||
        ((drive.gpt.valid_headers & MASK_PRIMARY) && params->verbose)) {
      GptHeader *header;
      char indent[64];

      require(snprintf(indent, sizeof(indent), GPT_MORE) < sizeof(indent));
      header = (GptHeader*)drive.gpt.primary_header;
      entries = (GptEntry*)drive.gpt.primary_entries;
      HeaderDetails(header, entries, indent, params->numeric);
    }

    printf(GPT_FMT, (int)(GPT_PMBR_SECTOR + GPT_HEADER_SECTOR),
           (int)GPT_ENTRIES_SECTORS,
           drive.gpt.valid_entries & MASK_PRIMARY ? "" : "INVALID",
           "Pri GPT table");

    if (params->debug ||
        (drive.gpt.valid_entries & MASK_PRIMARY))
      EntriesDetails(&drive.gpt, PRIMARY, params->numeric);

    /****************************** Secondary *************************/
    printf(GPT_FMT, (int)(drive.gpt.drive_sectors - GPT_HEADER_SECTOR -
                          GPT_ENTRIES_SECTORS),
           (int)GPT_ENTRIES_SECTORS,
           drive.gpt.valid_entries & MASK_SECONDARY ? "" : "INVALID",
           "Sec GPT table");
    /* We show secondary table details if any of following is true.
     *   1. in debug mode.
     *   2. only secondary is valid.
     *   3. secondary is not identical to promary.
     */
    if (params->debug ||
        ((drive.gpt.valid_entries & MASK_SECONDARY) &&
         (!(drive.gpt.valid_entries & MASK_PRIMARY) ||
          memcmp(drive.gpt.primary_entries, drive.gpt.secondary_entries,
                 TOTAL_ENTRIES_SIZE)))) {
      EntriesDetails(&drive.gpt, SECONDARY, params->numeric);
    }

    if (drive.gpt.valid_headers & MASK_SECONDARY)
      printf(GPT_FMT, (int)(drive.gpt.drive_sectors - GPT_HEADER_SECTOR),
             (int)GPT_HEADER_SECTOR, "", "Sec GPT header");
    else
      printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
             (int)GPT_HEADER_SECTOR, "INVALID", "Sec GPT header");
    /* We show secondary header if any of following is true:
     *   1. in debug mode.
     *   2. only secondary is valid.
     *   3. secondary is not synonymous to primary.
     */
    if (params->debug ||
        ((drive.gpt.valid_headers & MASK_SECONDARY) &&
         (!(drive.gpt.valid_headers & MASK_PRIMARY) ||
          !IsSynonymous((GptHeader*)drive.gpt.primary_header,
                        (GptHeader*)drive.gpt.secondary_header)) &&
         params->verbose)) {
      GptHeader *header;
      char indent[64];

      require(snprintf(indent, sizeof(indent), GPT_MORE) < sizeof(indent));
      header = (GptHeader*)drive.gpt.secondary_header;
      entries = (GptEntry*)drive.gpt.secondary_entries;
      HeaderDetails(header, entries, indent, params->numeric);
    }
  }

  CheckValid(&drive);
  DriveClose(&drive, 0);

  return CGPT_OK;
}
