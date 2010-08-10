/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TPM command utility.  Runs simple TPM commands.  Mostly useful when physical
 * presence has not been locked.
 */

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "tlcl.h"
#include "tpm_error_messages.h"

typedef struct command_record {
  const char* name;
  const char* abbr;
  const char* description;
  uint32_t (*handler)(void);
} command_record;

/* Handler functions.  These wouldn't exist if C had closures.
 */

static uint32_t HandlerGetFlags(void) {
  uint8_t disabled;
  uint8_t deactivated;
  uint8_t nvlocked;
  uint32_t result = TlclGetFlags(&disabled, &deactivated, &nvlocked);
  if (result == 0) {
    printf("disabled: %d\ndeactivated: %d\nnvlocked: %d\n",
           disabled, deactivated, nvlocked);
  }
  return result;
}

static uint32_t HandlerActivate(void) {
  return TlclSetDeactivated(0);
}

static uint32_t HandlerDeactivate(void) {
  return TlclSetDeactivated(1);
}

/* Table of TPM commands.
 */
command_record command_table[] = {
  { "getflags", "getf", "read and print the value of selected flags",
    HandlerGetFlags },
  { "startup", "sta", "issue a Startup command", TlclStartup },
  { "selftestfull", "test", "issue a SelfTestFull command", TlclSelfTestFull },
  { "continueselftest", "ctest", "issue a ContinueSelfTest command",
    TlclContinueSelfTest },
  { "assertphysicalpresence", "ppon", "assert Physical Presence",
    TlclAssertPhysicalPresence },
  { "enable", "ena", "enable the TPM (needs PP)", TlclSetEnable },
  { "disable", "dis", "disable the TPM (needs PP)", TlclClearEnable },
  { "activate", "act", "activate the TPM (needs PP, maybe reboot)",
    HandlerActivate },
  { "deactivate", "deact", "deactivate the TPM (needs PP, maybe reboot)",
    HandlerDeactivate },
};

static int n_commands = sizeof(command_table) / sizeof(command_table[0]);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <TPM command>\n   or: %s help\n",
            argv[0], argv[0]);
    exit(1);
  } else {
    command_record* c;
    const char* cmd = argv[1];

    if (strcmp(cmd, "help") == 0) {
      printf("%23s %7s  %s\n\n", "command", "abbr.", "description");
      for (c = command_table; c < command_table + n_commands; c++) {
        printf("%23s %7s  %s\n", c->name, c->abbr, c->description);
      }
      return 0;
    }

    TlclLibInit();

    for (c = command_table; c < command_table + n_commands; c++) {
      if (strcmp(cmd, c->name) == 0 || strcmp(cmd, c->abbr) == 0) {
        uint32_t result;
        result = c->handler();
        if (result == 0) {
          return 0;
        } else {
          int i;
          int n = sizeof(tpm_error_table) / sizeof(tpm_error_table[0]);
          fprintf(stderr, "command \"%s\" failed with code 0x%x\n",
                  cmd, result);
          for (i = 0; i < n; i++) {
             if (tpm_error_table[i].code == result) {
               fprintf(stderr, "%s\n%s\n", tpm_error_table[i].name,
                       tpm_error_table[i].description);
               return 1;
             }
          }
          fprintf(stderr, "the error code is unknown to this program\n");
          return 1;
        }
      }
    }

    /* No command matched. */
    fprintf(stderr, "%s: unknown command: %s\n", argv[0], cmd);
    return 1;
  }
}
