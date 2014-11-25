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
