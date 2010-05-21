/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Update GPT attribute bits.
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgpt.h"
#include "cgptlib_internal.h"
#include "utility.h"

/* Integers to store parsed argument. */
static int help, raw;

/* The structure for getopt_long(). When you add/delete any line, please refine
 * attribute_comments[] and third parameter of getopt_long() too.  */
static struct option show_options[] = {
  {.name = "help", .has_arg = no_argument, .flag = 0, .val = 'h'},
  {.name = "raw", .has_arg = no_argument, .flag = 0, .val = 'r'},
};

/* Extra information than struct option, please update this structure if you
 * add/remove any line in attribute_options[]. */
static struct option_details show_options_details[] = {
  /* help */
  { .comment = "print this help",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &help},
  /* raw */
  { .comment = "print raw data (byte-by-byte)",
    .validator = AssignTrue,
    .valid_range = 0,
    .parsed = &raw},
};

void ShowHelp() {
  printf("\nUsage: %s show [OPTIONS] device_name\n\n", progname);
  ShowOptions(show_options, show_options_details, ARRAY_COUNT(show_options));
  printf("\n");
}

/* Generate output like:
 *
 *  {AB-CD-EF-01}
 *
 * Needs (size*3-1+3) bytes of space in 'buf'.
 */
static short Uint8To2Chars(const uint8_t t) {
  int h = t >> 4;
  int l = t & 0xf;
  h = (h >= 0xA) ? h - 0xA + 'A' : h + '0';
  l = (l >= 0xA) ? l - 0xA + 'A' : l + '0';
  return (h << 8) + l;
}
static void RawDump(const uint8_t *memory, const int size, char *buf) {
  int i;
  buf[0] = '{';
  for (i = 0; i < size; ++i) {
    short c2 = Uint8To2Chars(memory[i]);
    buf[i * 3 + 1] = c2 >> 8;
    buf[i * 3 + 2] = c2 & 0xff;
    if (i != (size - 1))
      buf[i * 3 + 3] = '-';
  }
  buf[i * 3 + 0] = '}';
  buf[i * 3 + 1] = '\0';
}

/* Parses all options (and validates them), then opens the drive and sets
 * corresponding bits in GPT entry. */
int CgptShow(int argc, char *argv[]) {
  struct drive drive;
  int i;

  /* I know this is NOT the perfect place to put code to make options[] and
   * details[] are synced. But this is the best place we have right now since C
   * preprocessor doesn't know sizeof() for #if directive. */
  assert(ARRAY_COUNT(show_options) ==
         ARRAY_COUNT(show_options_details));

  help = raw = NOT_INITED;

  if (CGPT_OK != HandleOptions(argc, argv,
                     "hr",
                     ARRAY_COUNT(show_options),
                     show_options,
                     show_options_details))
    return CGPT_FAILED;
  if (help != NOT_INITED) {
    ShowHelp();
    return CGPT_FAILED;
  }

  if (CGPT_OK != OpenDriveInLastArgument(argc, argv, &drive))
    return CGPT_FAILED;

  #define TITLE_FMT      "%7s%7s%7s  %s\n"
  #define GPT_FMT        "%7d%7d%7s  %s\n"
  #define GPT_MORE       "%7s%7s%7s  %s\n", "", "", ""
  #define PARTITION_FMT  "%7d%7d%7d  %s\n"
  #define PARTITION_MORE "%7s%7s%7s  %s%s\n", "", "", ""
  printf(TITLE_FMT, "start", "size", "index", "contents");
  printf(GPT_FMT, 0, GPT_PMBR_SECTOR, "", "PMBR");
  printf(GPT_FMT, (int)GPT_PMBR_SECTOR,
         (int)GPT_HEADER_SECTOR, "", "Pri GPT header");
  printf(GPT_FMT, (int)(GPT_PMBR_SECTOR + GPT_HEADER_SECTOR),
         (int)GPT_ENTRIES_SECTORS, "", "Pri GPT table");

  for (i = 0; i < GetNumberOfEntries(&drive.gpt); ++i) {
    static Guid zero = {{{0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}}}};
    char contents[128];
    GptEntry *entry;
    entry = GetEntry(&drive.gpt, PRIMARY, i);

    if (!Memcmp(&zero, &entry->type, sizeof(zero))) continue;

    if (raw == NOT_INITED) {
      /* TODO(yjlou): support pretty dump */
      snprintf(contents, sizeof(contents),
          "* Not supported yet *");
      printf(PARTITION_FMT, (int)entry->starting_lba,
             (int)(entry->ending_lba - entry->starting_lba + 1),
             i, contents);
    } else {
      char type[50], unique[50], attributes[26];

      snprintf(contents, sizeof(contents),
          "%s", "");
      printf(PARTITION_FMT, (int)entry->starting_lba,
             (int)(entry->ending_lba - entry->starting_lba + 1),
             i, contents);
      RawDump((uint8_t*)&entry->type, 16, type);
      printf(PARTITION_MORE, "type: ", type);
      RawDump((uint8_t*)&entry->unique, 16, unique);
      printf(PARTITION_MORE, "uuid: ", unique);
      RawDump((uint8_t*)&entry->attributes, 8, attributes);
      printf(PARTITION_MORE, "attr: ", attributes);
    }
  }

  printf(GPT_FMT, (int)(drive.gpt.drive_sectors - GPT_HEADER_SECTOR -
                        GPT_ENTRIES_SECTORS),
         (int)GPT_ENTRIES_SECTORS, "", "Sec GPT table");
  printf(GPT_FMT, (int)(drive.gpt.drive_sectors - GPT_HEADER_SECTOR),
         (int)GPT_HEADER_SECTOR, "", "Sec GPT header");

  DriveClose(&drive);

  return CGPT_OK;
}
