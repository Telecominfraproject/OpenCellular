/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for ChromeOS-specific GPT partitions, Please see corresponding .c
 * files for more details.
 */

#include "cgpt.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


const char* progname;
const char* command;

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
};


void Usage(void) {
  int i;

  printf("Usage: %s COMMAND [OPTIONS] DRIVE\n\n"
         "Supported COMMANDs:\n\n",
         progname);

  for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    printf("    %-10s  %s\n", cmds[i].name, cmds[i].comment);
  }
  printf("\nFor more detailed usage, use %s COMMAND -h\n\n", progname);
}



int main(int argc, char *argv[]) {
  int i;

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
    if (0 == strcmp(cmds[i].name, command)) {
      return cmds[i].fp(argc, argv);
    }
  }

  // Couldn't find the command.
  Usage();

  return CGPT_FAILED;
}
