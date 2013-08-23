/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TPM command utility.  Runs simple TPM commands.  Mostly useful when physical
 * presence has not been locked.
 *
 * The exit code is 0 for success, the TPM error code for TPM errors, and 255
 * for other errors.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "tlcl.h"
#include "tpm_error_messages.h"
#include "tss_constants.h"

#define OTHER_ERROR 255  /* OTHER_ERROR must be the largest uint8_t value. */

typedef struct command_record {
  const char* name;
  const char* abbr;
  const char* description;
  uint32_t (*handler)(void);
} command_record;

/* Set in main, consumed by handler functions below.  We use global variables
 * so we can also choose to call Tlcl*() functions directly; they don't take
 * argv/argc.
 */
int nargs;
char** args;

/* Converts a string in the form 0x[0-9a-f]+ to a 32-bit value.  Returns 0 for
 * success, non-zero for failure.
 */
int HexStringToUint32(const char* string, uint32_t* value) {
  char tail[1];
  /* strtoul is not as good because it overflows silently */
  char* format = strncmp(string, "0x", 2) ? "%8x%s" : "0x%8x%s";
  int n = sscanf(string, format, value, tail);
  return n != 1;
}

/* Converts a string in the form [0-9a-f]+ to an 8-bit value.  Returns 0 for
 * success, non-zero for failure.
 */
int HexStringToUint8(const char* string, uint8_t* value) {
  char* end;
  uint32_t large_value = strtoul(string, &end, 16);
  if (*end != '\0' || large_value > 0xff) {
    return 1;
  }
  *value = large_value;
  return 0;
}

/* TPM error check and reporting.  Returns 0 if |result| is 0 (TPM_SUCCESS).
 * Otherwise looks up a TPM error in the error table and prints the error if
 * found.  Then returns min(result, OTHER_ERROR) since some error codes, such
 * as TPM_E_RETRY, do not fit in a byte.
 */
uint8_t ErrorCheck(uint32_t result, const char* cmd) {
  uint8_t exit_code = result > OTHER_ERROR ? OTHER_ERROR : result;
  if (result == 0) {
    return 0;
  } else {
    int i;
    int n = sizeof(tpm_error_table) / sizeof(tpm_error_table[0]);
    fprintf(stderr, "command \"%s\" failed with code 0x%x\n", cmd, result);
    for (i = 0; i < n; i++) {
      if (tpm_error_table[i].code == result) {
        fprintf(stderr, "%s\n%s\n", tpm_error_table[i].name,
                tpm_error_table[i].description);
        return exit_code;
      }
    }
    fprintf(stderr, "the TPM error code is unknown to this program\n");
    return exit_code;
  }
}

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

static uint32_t HandlerDefineSpace(void) {
  uint32_t index, size, perm;
  if (nargs != 5) {
    fprintf(stderr, "usage: tpmc def <index> <size> <perm>\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &index) != 0 ||
      HexStringToUint32(args[3], &size) != 0 ||
      HexStringToUint32(args[4], &perm) != 0) {
    fprintf(stderr, "<index>, <size>, and <perm> must be "
            "32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  return TlclDefineSpace(index, perm, size);
}

static uint32_t HandlerWrite(void) {
  uint32_t index, size;
  uint8_t value[TPM_MAX_COMMAND_SIZE];
  char** byteargs;
  int i;
  if (nargs < 3) {
    fprintf(stderr, "usage: tpmc write <index> [<byte0> <byte1> ...]\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &index) != 0) {
    fprintf(stderr, "<index> must be 32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  size = nargs - 3;
  if (size > sizeof(value)) {
    fprintf(stderr, "byte array too large\n");
    exit(OTHER_ERROR);
  }

  byteargs = args + 3;
  for (i = 0; i < size; i++) {
    if (HexStringToUint8(byteargs[i], &value[i]) != 0) {
      fprintf(stderr, "invalid byte %s, should be [0-9a-f][0-9a-f]?\n",
              byteargs[i]);
      exit(OTHER_ERROR);
    }
  }

  if (size == 0) {
    if (index == TPM_NV_INDEX_LOCK) {
      fprintf(stderr, "This would set the nvLocked bit. "
              "Use \"tpmc setnv\" instead.\n");
      exit(OTHER_ERROR);
    }
    printf("warning: zero-length write\n");
  } else {
    printf("writing %d byte%s\n", size, size > 1 ? "s" : "");
  }

  return TlclWrite(index, value, size);
}

static uint32_t HandlerPCRRead(void) {
  uint32_t index;
  uint8_t value[TPM_PCR_DIGEST];
  uint32_t result;
  int i;
  if (nargs != 3) {
    fprintf(stderr, "usage: tpmc pcrread <index>\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &index) != 0) {
    fprintf(stderr, "<index> must be 32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  result = TlclPCRRead(index, value, sizeof(value));
  if (result == 0) {
    for (i = 0; i < TPM_PCR_DIGEST; i++) {
      printf("%02x", value[i]);
    }
    printf("\n");
  }
  return result;
}

static uint32_t HandlerRead(void) {
  uint32_t index, size;
  uint8_t value[4096];
  uint32_t result;
  int i;
  if (nargs != 4) {
    fprintf(stderr, "usage: tpmc read <index> <size>\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &index) != 0 ||
      HexStringToUint32(args[3], &size) != 0) {
    fprintf(stderr, "<index> and <size> must be 32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  if (size > sizeof(value)) {
    fprintf(stderr, "size of read (0x%x) is too big\n", size);
    exit(OTHER_ERROR);
  }
  result = TlclRead(index, value, size);
  if (result == 0 && size > 0) {
    for (i = 0; i < size - 1; i++) {
      printf("%x ", value[i]);
    }
    printf("%x\n", value[i]);
  }
  return result;
}

static uint32_t HandlerGetPermissions(void) {
  uint32_t index, permissions, result;
  if (nargs != 3) {
    fprintf(stderr, "usage: tpmc getp <index>\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &index) != 0) {
    fprintf(stderr, "<index> must be 32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  result = TlclGetPermissions(index, &permissions);
  if (result == 0) {
    printf("space 0x%x has permissions 0x%x\n", index, permissions);
  }
  return result;
}

static uint32_t HandlerGetOwnership(void) {
  uint8_t owned = 0;
  uint32_t result;
  if (nargs != 2) {
    fprintf(stderr, "usage: tpmc getownership\n");
    exit(OTHER_ERROR);
  }
  result = TlclGetOwnership(&owned);
  if (result == 0) {
    printf("Owned: %s\n", owned ? "yes" : "no");
  }
  return result;
}

static uint32_t HandlerGetRandom(void) {
  uint32_t length, size;
  uint8_t* bytes;
  uint32_t result;
  int i;
  if (nargs != 3) {
    fprintf(stderr, "usage: tpmc getrandom <size>\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &length) != 0) {
    fprintf(stderr, "<size> must be 32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  bytes = calloc(1, length);
  if (bytes == NULL) {
    perror("calloc");
    exit(OTHER_ERROR);
  }
  result = TlclGetRandom(bytes, length, &size);
  if (result == 0 && size > 0) {
    for (i = 0; i < size; i++) {
      printf("%02x", bytes[i]);
    }
    printf("\n");
  }
  free(bytes);
  return result;
}

static uint32_t HandlerGetPermanentFlags(void) {
  TPM_PERMANENT_FLAGS pflags;
  uint32_t result = TlclGetPermanentFlags(&pflags);
  if (result == 0) {
#define P(name) printf("%s %d\n", #name, pflags.name)
    P(disable);
    P(ownership);
    P(deactivated);
    P(readPubek);
    P(disableOwnerClear);
    P(allowMaintenance);
    P(physicalPresenceLifetimeLock);
    P(physicalPresenceHWEnable);
    P(physicalPresenceCMDEnable);
    P(CEKPUsed);
    P(TPMpost);
    P(TPMpostLock);
    P(FIPS);
    P(Operator);
    P(enableRevokeEK);
    P(nvLocked);
    P(readSRKPub);
    P(tpmEstablished);
    P(maintenanceDone);
    P(disableFullDALogicInfo);
#undef P
  }
  return result;
}

static uint32_t HandlerGetSTClearFlags(void) {
  TPM_STCLEAR_FLAGS vflags;
  uint32_t result = TlclGetSTClearFlags(&vflags);
  if (result == 0) {
#define P(name) printf("%s %d\n", #name, vflags.name)
  P(deactivated);
  P(disableForceClear);
  P(physicalPresence);
  P(physicalPresenceLock);
  P(bGlobalLock);
#undef P
  }
  return result;
}


static uint32_t HandlerSendRaw(void) {
  uint8_t request[4096];
  uint8_t response[4096];
  uint32_t result;
  int size;
  int i;
  if (nargs == 2) {
    fprintf(stderr, "usage: tpmc sendraw <hex byte 0> ... <hex byte N>\n");
    exit(OTHER_ERROR);
  }
  for (i = 0; i < nargs - 2 && i < sizeof(request); i++) {
    if (HexStringToUint8(args[2 + i], &request[i]) != 0) {
      fprintf(stderr, "bad byte value \"%s\"\n", args[2 + i]);
      exit(OTHER_ERROR);
    }
  }
  size = TlclPacketSize(request);
  if (size != i) {
    fprintf(stderr, "bad request: size field is %d, but packet has %d bytes\n",
            size, i);
    exit(OTHER_ERROR);
  }
  bzero(response, sizeof(response));
  result = TlclSendReceive(request, response, sizeof(response));
  if (result != 0) {
    fprintf(stderr, "request failed with code %d\n", result);
  }
  size = TlclPacketSize(response);
  if (size < 10 || size > sizeof(response)) {
    fprintf(stderr, "unexpected response size %d\n", size);
    exit(OTHER_ERROR);
  }
  for (i = 0; i < size; i++) {
    printf("0x%02x ", response[i]);
    if (i == size - 1 || (i + 1) % 8 == 0) {
      printf("\n");
    }
  }
  return result;
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
  { "physicalpresencecmdenable", "ppcmd", "turn on software PP",
    TlclPhysicalPresenceCMDEnable },
  { "enable", "ena", "enable the TPM (needs PP)", TlclSetEnable },
  { "disable", "dis", "disable the TPM (needs PP)", TlclClearEnable },
  { "activate", "act", "activate the TPM (needs PP, maybe reboot)",
    HandlerActivate },
  { "deactivate", "deact", "deactivate the TPM (needs PP, maybe reboot)",
    HandlerDeactivate },
  { "clear", "clr", "clear the TPM owner (needs PP)", TlclForceClear },
  { "setnvlocked", "setnv", "set the nvLocked flag permanently (IRREVERSIBLE!)",
    TlclSetNvLocked },
  { "lockphysicalpresence", "pplock", "lock (turn off) PP until reboot",
    TlclLockPhysicalPresence },
  { "setbgloballock", "block", "set the bGlobalLock until reboot",
    TlclSetGlobalLock },
  { "definespace", "def", "define a space (def <index> <size> <perm>)",
    HandlerDefineSpace },
  { "write", "write", "write to a space (write <index> [<byte0> <byte1> ...])",
    HandlerWrite },
  { "read", "read", "read from a space (read <index> <size>)",
    HandlerRead },
  { "pcrread", "pcr", "read from a PCR (pcrread <index>)",
    HandlerPCRRead },
  { "getownership", "geto", "print state of TPM ownership",
    HandlerGetOwnership },
  { "getpermissions", "getp", "print space permissions (getp <index>)",
    HandlerGetPermissions },
  { "getpermanentflags", "getpf", "print all permanent flags",
    HandlerGetPermanentFlags },
  { "getrandom", "rand", "read bytes from RNG (rand <size>)",
    HandlerGetRandom },
  { "getstclearflags", "getvf", "print all volatile (ST_CLEAR) flags",
    HandlerGetSTClearFlags },
  { "resume", "res", "execute TPM_Startup(ST_STATE)", TlclResume },
  { "savestate", "save", "execute TPM_SaveState", TlclSaveState },
  { "sendraw", "raw", "send a raw request and print raw response",
    HandlerSendRaw },
};

static int n_commands = sizeof(command_table) / sizeof(command_table[0]);

int main(int argc, char* argv[]) {
  char *progname;
  progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  if (argc < 2) {
    fprintf(stderr, "usage: %s <TPM command> [args]\n   or: %s help\n",
            progname, progname);
    return OTHER_ERROR;
  } else {
    command_record* c;
    const char* cmd = argv[1];
    nargs = argc;
    args = argv;

    if (strcmp(cmd, "help") == 0) {
      printf("%26s %7s  %s\n\n", "command", "abbr.", "description");
      for (c = command_table; c < command_table + n_commands; c++) {
        printf("%26s %7s  %s\n", c->name, c->abbr, c->description);
      }
      return 0;
    }

    TlclLibInit();

    for (c = command_table; c < command_table + n_commands; c++) {
      if (strcmp(cmd, c->name) == 0 || strcmp(cmd, c->abbr) == 0) {
        return ErrorCheck(c->handler(), cmd);
      }
    }

    /* No command matched. */
    fprintf(stderr, "%s: unknown command: %s\n", progname, cmd);
    return OTHER_ERROR;
  }
}
