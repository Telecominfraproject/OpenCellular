/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for ChromeOS-specific GPT partitions, Please see corresponding .c
 * files for more details.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "cgpt.h"
#include "vboot_host.h"

const char* progname;

int GenerateGuid(Guid *newguid)
{
  /* From libuuid */
  uuid_generate(newguid->u.raw);
  return CGPT_OK;
}

struct {
  const char *name;
  int (*fp)(int argc, char *argv[]);
  const char *comment;
} cmds[] = {
  {"create", cmd_create, "Create or reset GPT headers and tables"},
  {"add", cmd_add, "Add, edit or remove a partition entry"},
  {"show", cmd_show, "Show partition table and entries"},
  {"repair", cmd_repair, "Repair damaged GPT headers and tables"},
  {"boot", cmd_boot, "Edit the PMBR sector for legacy BIOSes"},
  {"find", cmd_find, "Locate a partition by its GUID"},
  {"prioritize", cmd_prioritize,
   "Reorder the priority of all kernel partitions"},
  {"legacy", cmd_legacy, "Switch between GPT and Legacy GPT"},
};

void Usage(void) {
  int i;

  printf("\nUsage: %s COMMAND [OPTIONS] DRIVE\n\n"
         "Supported COMMANDs:\n\n",
         progname);

  for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    printf("    %-15s  %s\n", cmds[i].name, cmds[i].comment);
  }
  printf("\nFor more detailed usage, use %s COMMAND -h\n\n", progname);
}

static int is_pow2(size_t v) {
  return v && (v & (v - 1)) == 0;
}

static int parse_nand_option(const char *arg) {
  int bytes_per_page, pages_per_block, fts_block_offset, fts_block_size;

  if ('=' != arg[0])
    return -1;

  arg++;
  bytes_per_page = atoi(arg);
  arg = strchr(arg, ',');
  if (!arg)
    return -1;

  arg++;
  pages_per_block = atoi(arg);
  arg = strchr(arg, ',');
  if (!arg)
    return -1;

  arg++;
  fts_block_offset = atoi(arg);
  arg = strchr(arg, ',');
  if (!arg)
    return -1;

  arg++;
  fts_block_size = atoi(arg);
  if (fts_block_size == 0 || !is_pow2(pages_per_block) ||
      !is_pow2(bytes_per_page) || bytes_per_page < 512) {
    return -1;
  }
  EnableNandImage(bytes_per_page, pages_per_block, fts_block_offset,
                  fts_block_size);
  return 0;
}

int main(int argc, char *argv[]) {
  int i;
  int match_count = 0;
  int match_index = 0;
  char* command;

  progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];


  for (i = 1; i < argc; ++i) {
    if (0 == strncmp(argv[i], "-N", 2)) {
      if (!parse_nand_option(argv[i] + 2)) {
        int j;

        // Remove it form the list.
        for (j = i; j < argc - 1; j++)
          argv[j] = argv[j + 1];
        argc--;
        break;
      }
      // Bad nand config.
      printf("Nand option must fit: -N=<bytes_per_page>,<pages_per_block>,"
             "<block_offset_of_partition>,<block_size_of_partition>\n");
      return CGPT_FAILED;
    }
  }

  if (argc < 2) {
    Usage();
    return CGPT_FAILED;
  }

  // increment optind now, so that getopt skips argv[0] in command function
  command = argv[optind++];

  // Find the command to invoke.
  for (i = 0; command && i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    // exact match?
    if (0 == strcmp(cmds[i].name, command)) {
      match_index = i;
      match_count = 1;
      break;
    }
    // unique match?
    else if (0 == strncmp(cmds[i].name, command, strlen(command))) {
      match_index = i;
      match_count++;
    }
  }

  if (match_count == 1)
    return cmds[match_index].fp(argc, argv);

  // Couldn't find a single matching command.
  Usage();

  return CGPT_FAILED;
}
