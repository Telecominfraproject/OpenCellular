/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Show GPT details.
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "cgptlib_internal.h"
#include "cgpt_tofix.h"
#include "utility.h"

/* Integers to store parsed argument. */
static int help, number, verbose;

/* The structure for getopt_long(). When you add/delete any line, please refine
 * attribute_comments[] and third parameter of getopt_long() too.  */
static struct option show_options[] = {
  {.name = "help", .has_arg = no_argument, .flag = 0, .val = 'h'},
  {.name = "number", .has_arg = no_argument, .flag = 0, .val = 'n'},
  {.name = "verbose", .has_arg = no_argument, .flag = 0, .val = 'v'},
  { /* last element, which should be zero. */ }
};

/* Extra information than struct option, please update this structure if you
 * add/remove any line in attribute_options[]. */
static struct option_details show_options_details[] = {
  /* help */
  { .comment = "print this help",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &help},
  /* number */
  { .comment = "print raw numbers (don't interpret)",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &number},
  /* verbose */
  { .comment = "verbose print",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &verbose},
  { /* last element, which should be zero. */ }
};

void ShowHelp() {
  printf("\nUsage: %s show [OPTIONS] device_name\n\n", progname);
  ShowOptions(show_options, show_options_details, ARRAY_COUNT(show_options));
  printf("\n");
}

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

/* Outpur formatters */
#define TITLE_FMT      "%10s%10s%8s  %s\n"
#define GPT_FMT        "%10d%10d%8s  %s\n"
#define GPT_MORE       "%10s%10s%8s  ", "", "", ""
#define PARTITION_FMT  "%10d%10d%8d  %s\n"
#define PARTITION_MORE "%10s%10s%8s  %s%s\n", "", "", ""

static void HeaderDetails(GptHeader *header, const char *indent) {
  int i;

  printf("%sSig: ", indent);
  if (number == NOT_INITED) {
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
  printf("%sHeader CRC: 0x%08x\n", indent, header->header_crc32);
  printf("%sMy LBA: %lld\n", indent, (long long)header->my_lba);
  printf("%sAlter LBA: %lld\n", indent, (long long)header->alternate_lba);
  printf("%sFirst LBA: %lld\n", indent, (long long)header->first_usable_lba);
  printf("%sLast LBA: %lld\n", indent, (long long)header->last_usable_lba);

  {  /* For disk guid */
    char buf[GUID_STRLEN];
    GuidToStr(&header->disk_uuid, buf);
    printf("%sDisk UUID: %s\n", indent, buf);
  }

  printf("%sEntries LBA: %lld\n", indent, (long long)header->entries_lba);
  printf("%sNumber of entries: %d\n", indent, header->number_of_entries);
  printf("%sSize of entry: %d\n", indent, header->size_of_entry);
  printf("%sEntries CRC: 0x%08x\n", indent, header->entries_crc32);
}

void EntriesDetails(GptData *gpt, const int secondary) {
  int i;

  for (i = 0; i < GetNumberOfEntries(gpt); ++i) {
    static Guid unused = GPT_ENT_TYPE_UNUSED;
    char contents[256];

    GptEntry *entry;
    entry = GetEntry(gpt, secondary, i);

    if (!Memcmp(&unused, &entry->type, sizeof(unused))) continue;

    if (number == NOT_INITED) {
      uint8_t label[sizeof(entry->name) * 3 / 2];
      char type[GUID_STRLEN], unique[GUID_STRLEN];;

      UTF16ToUTF8(entry->name, label);
      snprintf(contents, sizeof(contents), "Label: \"%s\"", label);
      printf(PARTITION_FMT, (int)entry->starting_lba,
             (int)(entry->ending_lba - entry->starting_lba + 1),
             i, contents);
      if (CGPT_OK == ResolveType(&entry->type, type)) {
        printf(PARTITION_MORE, "Type: ", type);
      } else {
        GuidToStr(&entry->type, type);
        printf(PARTITION_MORE, "Type: ", type);
      }
      GuidToStr(&entry->unique, unique);
      printf(PARTITION_MORE, "UUID: ", unique);
    } else {
      char label[BUFFER_SIZE(sizeof(entry->name))];
      char type[GUID_STRLEN], unique[GUID_STRLEN],
           attributes[BUFFER_SIZE(sizeof(uint64_t))];

      RawDump((void*)entry->name, sizeof(entry->name), label, 2);
      snprintf(contents, sizeof(contents), "Label: %s", label);
      printf(PARTITION_FMT, (int)entry->starting_lba,
             (int)(entry->ending_lba - entry->starting_lba + 1),
             i, contents);
      GuidToStr(&entry->type, type);
      printf(PARTITION_MORE, "Type: ", type);
      GuidToStr(&entry->unique, unique);
      printf(PARTITION_MORE, "UUID: ", unique);
      RawDump((uint8_t*)&entry->attributes, 8, attributes, 4);
      printf(PARTITION_MORE, "Attr: ", attributes);
    }
  }
}

/* Parses all options (and validates them), then opens the drive.
 * Show GPT information in following order:
 *
 *   Primary header sector
 *     details (if -v applied)
 *
 *   Primary table sectors
 *
 *   1st partition
 *     details (if -v applied)
 *   :
 *   last partition
 *     details (if -v applied)
 *
 *   Secondary table sectors
 *
 *   Secondary header sector
 *     details (if -v applied)
 */
int CgptShow(int argc, char *argv[]) {
  struct drive drive;

  /* I know this is NOT the perfect place to put code to make options[] and
   * details[] are synced. But this is the best place we have right now since C
   * preprocessor doesn't know sizeof() for #if directive. */
  assert(ARRAY_COUNT(show_options) ==
         ARRAY_COUNT(show_options_details));

  help = number = NOT_INITED;

  if (CGPT_OK != HandleOptions(argc, argv,
                     "hnv",
                     ARRAY_COUNT(show_options),
                     show_options,
                     show_options_details))
    return CGPT_FAILED;
  if (help != NOT_INITED) {
    ShowHelp();
    return CGPT_FAILED;
  }

  OpenDriveInLastArgument(argc, argv, &drive);
  if (CGPT_OK != OpenDriveInLastArgument(argc, argv, &drive))
    return CGPT_FAILED;

  printf(TITLE_FMT, "start", "size", "index", "contents");
  printf(GPT_FMT, 0, GPT_PMBR_SECTOR, "", "PMBR");

  if (drive.gpt.valid_headers & MASK_PRIMARY) {
    printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
           (int)GPT_HEADER_SECTOR, "", "Pri GPT header");
    if (verbose) {
      GptHeader *header;
      char indent[64];

      snprintf(indent, sizeof(indent), GPT_MORE);
      header = (GptHeader*)drive.gpt.primary_header;
      HeaderDetails(header, indent);
    }
  } else {
    printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
           (int)GPT_HEADER_SECTOR, "INVALID", "Pri GPT header");
  }

  printf(GPT_FMT, (int)(GPT_PMBR_SECTOR + GPT_HEADER_SECTOR),
         (int)GPT_ENTRIES_SECTORS,
         drive.gpt.valid_entries & MASK_PRIMARY ? "" : "INVALID",
         "Pri GPT table");

  if (drive.gpt.valid_entries & MASK_PRIMARY)
    EntriesDetails(&drive.gpt, PRIMARY);

  printf(GPT_FMT, (int)(drive.gpt.drive_sectors - GPT_HEADER_SECTOR -
                        GPT_ENTRIES_SECTORS),
         (int)GPT_ENTRIES_SECTORS,
         drive.gpt.valid_entries & MASK_SECONDARY ? "" : "INVALID",
         "Sec GPT table");
  /* We show secondary table details if any of following is true.
   *   1. only secondary is valid.
   *   2. secondary is not identical to promary.
   */
  if ((drive.gpt.valid_entries & MASK_SECONDARY) &&
      (!(drive.gpt.valid_entries & MASK_PRIMARY) ||
       Memcmp(drive.gpt.primary_entries, drive.gpt.secondary_entries,
              TOTAL_ENTRIES_SIZE))) {
    EntriesDetails(&drive.gpt, SECONDARY);
  }

  if (drive.gpt.valid_headers & MASK_SECONDARY)
    printf(GPT_FMT, (int)(drive.gpt.drive_sectors - GPT_HEADER_SECTOR),
           (int)GPT_HEADER_SECTOR, "", "Sec GPT header");
  else
    printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
           (int)GPT_HEADER_SECTOR, "INVALID", "Sec GPT header");
  /* We show secondary header if any of following is true:
   *   1. only secondary is valid.
   *   2. secondary is not synonymous to primary.
   */
  if ((drive.gpt.valid_headers & MASK_SECONDARY) &&
      (!(drive.gpt.valid_headers & MASK_PRIMARY) ||
       !IsSynonymous((GptHeader*)drive.gpt.primary_header,
                     (GptHeader*)drive.gpt.secondary_header))) {
    if (verbose) {
      GptHeader *header;
      char indent[64];

      snprintf(indent, sizeof(indent), GPT_MORE);
      header = (GptHeader*)drive.gpt.secondary_header;
      HeaderDetails(header, indent);
    }
  }

  CheckValid(&drive);
  DriveClose(&drive);

  return CGPT_OK;
}
