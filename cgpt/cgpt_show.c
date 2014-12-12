// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define __STDC_FORMAT_MACROS

#include <string.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "vboot_host.h"

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

void PrintSignature(const char *indent, const char *sig, size_t n, int raw) {
  size_t i;
  printf("%sSig: ", indent);
  if (!raw) {
    printf("[");
    for (i = 0; i < n; ++i)
      printf("%c", sig[i]);
    printf("]");
  } else {
    char *buf = malloc(BUFFER_SIZE(n));
    RawDump((uint8_t *)sig, n, buf, 1);
    printf("%s", buf);
    free(buf);
  }
  printf("\n");
}

static void HeaderDetails(GptHeader *header, GptEntry *entries,
                          const char *indent, int raw) {
  PrintSignature(indent, header->signature, sizeof(header->signature), raw);

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
  char type[GUID_STRLEN], unique[GUID_STRLEN];

  UTF16ToUTF8(entry->name, sizeof(entry->name) / sizeof(entry->name[0]),
              label, sizeof(label));
  require(snprintf(contents, sizeof(contents),
                   "Label: \"%s\"", label) < sizeof(contents));
  printf(PARTITION_FMT, (int)entry->starting_lba,
         (int)(entry->ending_lba - entry->starting_lba + 1),
         index+1, contents);

  if (!raw && CGPT_OK == ResolveType(&entry->type, type)) {
    printf(PARTITION_MORE, "Type: ", type);
  } else {
    GuidToStr(&entry->type, type, GUID_STRLEN);
    printf(PARTITION_MORE, "Type: ", type);
  }
  GuidToStr(&entry->unique, unique, GUID_STRLEN);
  printf(PARTITION_MORE, "UUID: ", unique);

  if (!raw) {
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
    require(snprintf(contents, sizeof(contents),
                     "[%x]", entry->attrs.fields.gpt_att) < sizeof(contents));
    printf(PARTITION_MORE, "Attr: ", contents);
  }
}

void EntriesDetails(struct drive *drive, const int secondary, int raw) {
  uint32_t i;

  for (i = 0; i < GetNumberOfEntries(drive); ++i) {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, i);

    if (GuidIsZero(&entry->type))
      continue;

    EntryDetails(entry, i, raw);
  }
}

static int GptShow(struct drive *drive, CgptShowParams *params) {
  int gpt_retval;
  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive->gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    return CGPT_FAILED;
  }

  if (params->partition) {                      // show single partition

    if (params->partition > GetNumberOfEntries(drive)) {
      Error("invalid partition number: %d\n", params->partition);
      return CGPT_FAILED;
    }

    uint32_t index = params->partition - 1;
    GptEntry *entry = GetEntry(&drive->gpt, ANY_VALID, index);
    char buf[256];                      // scratch buffer for string conversion

    if (params->single_item) {
      switch(params->single_item) {
      case 'b':
        printf("%" PRId64 "\n", entry->starting_lba);
        break;
      case 's': {
        uint64_t size = 0;
        // If these aren't actually defined, don't show anything
        if (entry->ending_lba || entry->starting_lba)
          size = entry->ending_lba - entry->starting_lba + 1;
        printf("%" PRId64 "\n", size);
        break;
      }
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
        printf("%d\n", GetSuccessful(drive, ANY_VALID, index));
        break;
      case 'T':
        printf("%d\n", GetTries(drive, ANY_VALID, index));
        break;
      case 'P':
        printf("%d\n", GetPriority(drive, ANY_VALID, index));
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

    for (i = 0; i < GetNumberOfEntries(drive); ++i) {
      entry = GetEntry(&drive->gpt, ANY_VALID, i);

      if (GuidIsZero(&entry->type))
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

    if (CGPT_OK != ReadPMBR(drive)) {
      Error("Unable to read PMBR\n");
      return CGPT_FAILED;
    }

    printf(TITLE_FMT, "start", "size", "part", "contents");
    char buf[256];                      // buffer for formatted PMBR content
    PMBRToStr(&drive->pmbr, buf, sizeof(buf)); // will exit if buf is too small
    printf(GPT_FMT, 0, GPT_PMBR_SECTORS, "", buf);

    if (drive->gpt.valid_headers & MASK_PRIMARY) {
      printf(GPT_FMT, (int)GPT_PMBR_SECTORS,
             (int)GPT_HEADER_SECTORS, "", "Pri GPT header");
    } else {
      printf(GPT_FMT, (int)GPT_PMBR_SECTORS,
             (int)GPT_HEADER_SECTORS, "INVALID", "Pri GPT header");
    }

    if (params->debug ||
        ((drive->gpt.valid_headers & MASK_PRIMARY) && params->verbose)) {
      GptHeader *header;
      char indent[64];

      require(snprintf(indent, sizeof(indent), GPT_MORE) < sizeof(indent));
      header = (GptHeader*)drive->gpt.primary_header;
      entries = (GptEntry*)drive->gpt.primary_entries;
      HeaderDetails(header, entries, indent, params->numeric);
    }

    GptHeader* primary_header = (GptHeader*)drive->gpt.primary_header;
    printf(GPT_FMT, (int)primary_header->entries_lba,
           (int)CalculateEntriesSectors(primary_header),
           drive->gpt.valid_entries & MASK_PRIMARY ? "" : "INVALID",
           "Pri GPT table");

    if (params->debug ||
        (drive->gpt.valid_entries & MASK_PRIMARY))
      EntriesDetails(drive, PRIMARY, params->numeric);

    /****************************** Secondary *************************/
    GptHeader* secondary_header = (GptHeader*)drive->gpt.secondary_header;
    printf(GPT_FMT, (int)secondary_header->entries_lba,
           (int)CalculateEntriesSectors(secondary_header),
           drive->gpt.valid_entries & MASK_SECONDARY ? "" : "INVALID",
           "Sec GPT table");
    /* We show secondary table details if any of following is true.
     *   1. in debug mode.
     *   2. only secondary is valid.
     *   3. secondary is not identical to promary.
     */
    if (params->debug ||
        ((drive->gpt.valid_entries & MASK_SECONDARY) &&
         (!(drive->gpt.valid_entries & MASK_PRIMARY) ||
          memcmp(drive->gpt.primary_entries, drive->gpt.secondary_entries,
                 secondary_header->number_of_entries *
                 secondary_header->size_of_entry)))) {
      EntriesDetails(drive, SECONDARY, params->numeric);
    }

    if (drive->gpt.valid_headers & MASK_SECONDARY)
      printf(GPT_FMT, (int)(drive->gpt.gpt_drive_sectors - GPT_HEADER_SECTORS),
             (int)GPT_HEADER_SECTORS, "", "Sec GPT header");
    else
      printf(GPT_FMT, (int)GPT_PMBR_SECTORS,
             (int)GPT_HEADER_SECTORS, "INVALID", "Sec GPT header");
    /* We show secondary header if any of following is true:
     *   1. in debug mode.
     *   2. only secondary is valid.
     *   3. secondary is not synonymous to primary.
     */
    if (params->debug ||
        ((drive->gpt.valid_headers & MASK_SECONDARY) &&
         (!(drive->gpt.valid_headers & MASK_PRIMARY) ||
          !IsSynonymous((GptHeader*)drive->gpt.primary_header,
                        (GptHeader*)drive->gpt.secondary_header)) &&
         params->verbose)) {
      GptHeader *header;
      char indent[64];

      require(snprintf(indent, sizeof(indent), GPT_MORE) < sizeof(indent));
      header = (GptHeader*)drive->gpt.secondary_header;
      entries = (GptEntry*)drive->gpt.secondary_entries;
      HeaderDetails(header, entries, indent, params->numeric);
    }
  }

  CheckValid(drive);

  return CGPT_OK;
}

int CgptShow(CgptShowParams *params) {
  struct drive drive;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDONLY,
                           params->drive_size))
    return CGPT_FAILED;

  if (GptShow(&drive, params))
    return CGPT_FAILED;

  DriveClose(&drive, 0);
  return CGPT_OK;
}
