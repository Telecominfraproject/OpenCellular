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

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "tlcl.h"
#include "tpm_error_messages.h"
#include "tss_constants.h"

#define OTHER_ERROR 255  /* OTHER_ERROR must be the largest uint8_t value. */

#ifdef TPM2_MODE
#define TPM_MODE_SELECT(_, tpm20_ver) tpm20_ver
#else
#define TPM_MODE_SELECT(tpm12_ver, _) tpm12_ver
#endif

#define TPM_MODE_STRING TPM_MODE_SELECT("1.2", "2.0")
#define TPM12_NEEDS_PP TPM_MODE_SELECT(" (needs PP)", "")
#define TPM12_NEEDS_PP_REBOOT TPM_MODE_SELECT(" (needs PP, maybe reboot)", "")

#define TPM20_NOT_IMPLEMENTED_DESCR(descr) \
  descr TPM_MODE_SELECT("", " [not-implemented for TPM2.0]")
#define TPM20_NOT_IMPLEMENTED_HANDLER(handler) \
  TPM_MODE_SELECT(handler, HandlerNotImplementedForTPM2)
#define TPM20_NOT_IMPLEMENTED(descr, handler) \
  TPM20_NOT_IMPLEMENTED_DESCR(descr), \
  TPM20_NOT_IMPLEMENTED_HANDLER(handler)

#define TPM20_DOES_NOTHING_DESCR(descr) \
  descr TPM_MODE_SELECT("", " [no-op for TPM2.0]")
#define TPM20_DOES_NOTHING_HANDLER(handler) \
  TPM_MODE_SELECT(handler, HandlerDoNothingForTPM2)
#define TPM20_DOES_NOTHING(descr, handler) \
  TPM20_DOES_NOTHING_DESCR(descr), \
  TPM20_DOES_NOTHING_HANDLER(handler)

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

int HexStringToArray(const char* string, uint8_t* value, int num_bytes) {
  int len = strlen(string);
  if (!strncmp(string, "0x", 2)) {
    string += 2;
    len -= 2;
  }
  if (len != num_bytes * 2) {
    return 1;
  }
  for (; len > 0; string += 2, len -= 2, value++) {
    if (sscanf(string, "%2hhx", value) != 1) {
      return 1;
    }
  }
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
static uint32_t HandlerTpmVersion(void) {
  puts(TPM_MODE_STRING);
  return 0;
}

/* TODO(apronin): stub for selected flags for TPM2 */
#ifdef TPM2_MODE
static uint32_t HandlerGetFlags(void) {
  fprintf(stderr, "getflags not implemented for TPM2\n");
  exit(OTHER_ERROR);
}
#else
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
#endif

#ifndef TPM2_MODE
static uint32_t HandlerActivate(void) {
  return TlclSetDeactivated(0);
}

static uint32_t HandlerDeactivate(void) {
  return TlclSetDeactivated(1);
}
#endif

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
#ifndef TPM2_MODE
    if (index == TPM_NV_INDEX_LOCK) {
      fprintf(stderr, "This would set the nvLocked bit. "
              "Use \"tpmc setnv\" instead.\n");
      exit(OTHER_ERROR);
    }
#endif
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

static uint32_t HandlerPCRExtend(void) {
  uint32_t index;
  uint8_t value[TPM_PCR_DIGEST];
  if (nargs != 4) {
    fprintf(stderr, "usage: tpmc pcrextend <index> <extend_hash>\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToUint32(args[2], &index) != 0) {
    fprintf(stderr, "<index> must be 32-bit hex (0x[0-9a-f]+)\n");
    exit(OTHER_ERROR);
  }
  if (HexStringToArray(args[3], value, TPM_PCR_DIGEST)) {
    fprintf(stderr, "<extend_hash> must be a 20-byte hex string\n");
    exit(OTHER_ERROR);
  }
  return TlclExtend(index, value, value);
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
  uint32_t length, size = 0;
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
#ifdef TPM2_MODE
    P(ownerAuthSet);
    P(endorsementAuthSet);
    P(lockoutAuthSet);
    P(disableClear);
    P(inLockout);
    P(tpmGeneratedEPS);
#else
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
#endif
#undef P
  }
  return result;
}

static uint32_t HandlerGetSTClearFlags(void) {
  TPM_STCLEAR_FLAGS vflags;
  uint32_t result = TlclGetSTClearFlags(&vflags);
  if (result == 0) {
#define P(name) printf("%s %d\n", #name, vflags.name)
#ifdef TPM2_MODE
  P(phEnable);
  P(shEnable);
  P(ehEnable);
  P(phEnableNV);
  P(orderly);
#else
  P(deactivated);
  P(disableForceClear);
  P(physicalPresence);
  P(physicalPresenceLock);
  P(bGlobalLock);
#endif
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

static uint32_t HandlerGetVersion(void) {
  uint32_t vendor;
  uint64_t firmware_version;
  uint8_t vendor_specific[32];
  size_t vendor_specific_size = sizeof(vendor_specific);
  uint32_t result = TlclGetVersion(&vendor, &firmware_version, vendor_specific,
                                   &vendor_specific_size);
  if (result == 0) {
    printf("vendor %08x\nfirmware_version %016" PRIx64 "\nvendor_specific ",
           vendor, firmware_version);
    size_t n;
    for (n = 0; n < vendor_specific_size; ++n) {
      printf("%02x", vendor_specific[n]);
    }
    printf("\n");
  }
  return result;
}

#ifndef TPM2_MODE
static void PrintIFXFirmwarePackage(TPM_IFX_FIRMWAREPACKAGE* firmware_package,
                                    const char* prefix) {
  printf("%s_package_id %08x\n", prefix,
         firmware_package->FwPackageIdentifier);
  printf("%s_version %08x\n", prefix, firmware_package->Version);
  printf("%s_stale_version %08x\n", prefix, firmware_package->StaleVersion);
}

static uint32_t HandlerIFXFieldUpgradeInfo(void) {
  TPM_IFX_FIELDUPGRADEINFO info;
  uint32_t result = TlclIFXFieldUpgradeInfo(&info);
  if (result == 0) {
    printf("max_data_size %u\n", info.wMaxDataSize);
    PrintIFXFirmwarePackage(&info.sBootloaderFirmwarePackage, "bootloader");
    PrintIFXFirmwarePackage(&info.sFirmwarePackages[0], "fw0");
    PrintIFXFirmwarePackage(&info.sFirmwarePackages[1], "fw1");
    printf("status %04x\n", info.wSecurityModuleStatus);
    PrintIFXFirmwarePackage(&info.sProcessFirmwarePackage, "process_fw");
    printf("field_upgrade_counter %u\n", info.wFieldUpgradeCounter);
  }
  return result;
}
#endif

#ifdef TPM2_MODE
static uint32_t HandlerDoNothingForTPM2(void) {
  return 0;
}

static uint32_t HandlerNotImplementedForTPM2(void) {
  fprintf(stderr, "%s: not implemented for TPM2.0\n", args[1]);
  exit(OTHER_ERROR);
}
#endif

/* Table of TPM commands.
 */
command_record command_table[] = {
  { "tpmversion", "tpmver", "print TPM version: 1.2 or 2.0",
    HandlerTpmVersion },
  { "getflags", "getf", "read and print the value of selected flags",
    HandlerGetFlags },
  { "startup", "sta", "issue a Startup command", TlclStartup },
  { "selftestfull", "test", "issue a SelfTestFull command", TlclSelfTestFull },
  { "continueselftest", "ctest", "issue a ContinueSelfTest command",
    TlclContinueSelfTest },
  { "assertphysicalpresence", "ppon",
    TPM20_DOES_NOTHING("assert Physical Presence",
      TlclAssertPhysicalPresence) },
  { "physicalpresencecmdenable", "ppcmd",
    TPM20_NOT_IMPLEMENTED("turn on software PP",
      TlclPhysicalPresenceCMDEnable) },
  { "enable", "ena",
    TPM20_DOES_NOTHING("enable the TPM" TPM12_NEEDS_PP,
      TlclSetEnable) },
  { "disable", "dis",
    TPM20_NOT_IMPLEMENTED("disable the TPM" TPM12_NEEDS_PP,
      TlclClearEnable) },
  { "activate", "act",
    TPM20_DOES_NOTHING("activate the TPM" TPM12_NEEDS_PP_REBOOT,
      HandlerActivate) },
  { "deactivate", "deact",
    TPM20_NOT_IMPLEMENTED("deactivate the TPM" TPM12_NEEDS_PP_REBOOT,
      HandlerDeactivate) },
  { "clear", "clr",
    "clear the TPM owner" TPM12_NEEDS_PP,
    TlclForceClear },
  { "setnvlocked", "setnv",
    TPM20_NOT_IMPLEMENTED("set the nvLocked flag permanently (IRREVERSIBLE!)",
      TlclSetNvLocked) },
  { "lockphysicalpresence", "pplock",
    TPM_MODE_SELECT("lock (turn off) PP until reboot",
      "set rollback protection lock for kernel image until reboot"),
    TlclLockPhysicalPresence },
  { "setbgloballock", "block",
    TPM_MODE_SELECT("set the bGlobalLock until reboot",
      "set rollback protection lock for R/W firmware until reboot"),
    TlclSetGlobalLock },
  { "definespace", "def", "define a space (def <index> <size> <perm>)",
    HandlerDefineSpace },
  { "write", "write", "write to a space (write <index> [<byte0> <byte1> ...])",
    HandlerWrite },
  { "read", "read", "read from a space (read <index> <size>)",
    HandlerRead },
  { "pcrread", "pcr", "read from a PCR (pcrread <index>)",
    HandlerPCRRead },
  { "pcrextend", "extend", "extend a PCR (extend <index> <extend_hash>)",
    HandlerPCRExtend },
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
  { "getversion", "getver", "get TPM vendor and firmware version",
    HandlerGetVersion },
  { "ifxfieldupgradeinfo", "ifxfui",
    TPM20_NOT_IMPLEMENTED("read and print IFX field upgrade info",
      HandlerIFXFieldUpgradeInfo) },
};

static int n_commands = sizeof(command_table) / sizeof(command_table[0]);

int main(int argc, char* argv[]) {
  char *progname;
  uint32_t result;

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
      printf("tpmc mode: TPM%s\n", TPM_MODE_STRING);
      printf("%26s %7s  %s\n\n", "command", "abbr.", "description");
      for (c = command_table; c < command_table + n_commands; c++) {
        printf("%26s %7s  %s\n", c->name, c->abbr, c->description);
      }
      return 0;
    }
    if (!strcmp(cmd, "tpmversion") || !strcmp(cmd, "tpmver")) {
      return HandlerTpmVersion();
    }

    result = TlclLibInit();
    if (result) {
      fprintf(stderr, "initialization failed with code %d\n", result);
      return result > OTHER_ERROR ? OTHER_ERROR : result;
    }

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
