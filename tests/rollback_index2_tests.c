/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for rollback_index functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "rollback_index.h"
#include "test_common.h"
#include "tlcl.h"
#include "utility.h"
#include "vboot_common.h"

static char calls[16384];
static char *cnext = calls;
static int ccount = 0;
static int cfail = 0;
static uint32_t cfail_err = TPM_SUCCESS;

static TPM_PERMANENT_FLAGS mock_pflags;
static RollbackSpaceFirmware mock_rsf;
static RollbackSpaceKernel mock_rsk;

static void ResetMocks(int fail_on_call, uint32_t fail_with_err) {
  cnext = calls;
  ccount = 0;
  cfail = fail_on_call;
  cfail_err = fail_with_err;

  Memset(&mock_pflags, 0, sizeof(mock_pflags));
  Memset(&mock_rsf, 0, sizeof(mock_rsf));
  Memset(&mock_rsk, 0, sizeof(mock_rsk));
}

/****************************************************************************/
/* Mocks for tlcl functions which log the calls made to calls[]. */

uint32_t TlclForceClear(void) {
  cnext += sprintf(cnext, "TlclForceClear()\n");
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclSetEnable(void) {
  cnext += sprintf(cnext, "TlclSetEnable()\n");
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclSetDeactivated(uint8_t flag) {
  cnext += sprintf(cnext, "TlclSetDeactivated(%d)\n", flag);
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void* data, uint32_t length) {
  cnext += sprintf(cnext, "TlclWrite(0x%x, %d)\n", index, length);

  if (FIRMWARE_NV_INDEX == index) {
    TEST_EQ(length, sizeof(mock_rsf), "TlclWrite rsf size");
    Memcpy(&mock_rsf, data, length);
  } else if (KERNEL_NV_INDEX == index) {
    TEST_EQ(length, sizeof(mock_rsk), "TlclWrite rsk size");
    Memcpy(&mock_rsk, data, length);
  }

  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  cnext += sprintf(cnext, "TlclDefineSpace(0x%x, 0x%x, %d)\n",
                   index, perm, size);
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclSelfTestFull(void) {
  cnext += sprintf(cnext, "TlclSelfTestFull()\n");
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS* pflags) {
  cnext += sprintf(cnext, "TlclGetPermanentFlags()\n");
  Memcpy(pflags, &mock_pflags, sizeof(mock_pflags));
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclFinalizePhysicalPresence(void) {
  cnext += sprintf(cnext, "TlclFinalizePhysicalPresence()\n");
  mock_pflags.physicalPresenceLifetimeLock = 1;
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

uint32_t TlclSetNvLocked(void) {
  cnext += sprintf(cnext, "TlclSetNvLocked()\n");
  mock_pflags.nvLocked = 1;
  return (++ccount == cfail) ? cfail_err : TPM_SUCCESS;
}

/****************************************************************************/
/* Tests for misc helper functions */

static void MiscTest(void) {
  uint8_t buf[8];

  ResetMocks(0, 0);
  TEST_EQ(TPMClearAndReenable(), 0, "TPMClearAndReenable()");
  TEST_STR_EQ(calls,
              "TlclForceClear()\n"
              "TlclSetEnable()\n"
              "TlclSetDeactivated(0)\n",
              "tlcl calls");

  ResetMocks(0, 0);
  TEST_EQ(SafeWrite(0x123, buf, 8), 0, "SafeWrite()");
  TEST_STR_EQ(calls,
              "TlclWrite(0x123, 8)\n",
              "tlcl calls");

  ResetMocks(1, TPM_E_BADINDEX);
  TEST_EQ(SafeWrite(0x123, buf, 8), TPM_E_BADINDEX, "SafeWrite() bad");
  TEST_STR_EQ(calls,
              "TlclWrite(0x123, 8)\n",
              "tlcl calls");

  ResetMocks(1, TPM_E_MAXNVWRITES);
  TEST_EQ(SafeWrite(0x123, buf, 8), 0, "SafeWrite() retry max writes");
  TEST_STR_EQ(calls,
              "TlclWrite(0x123, 8)\n"
              "TlclForceClear()\n"
              "TlclSetEnable()\n"
              "TlclSetDeactivated(0)\n"
              "TlclWrite(0x123, 8)\n",
              "tlcl calls");

  ResetMocks(0, 0);
  TEST_EQ(SafeDefineSpace(0x123, 6, 8), 0, "SafeDefineSpace()");
  TEST_STR_EQ(calls,
              "TlclDefineSpace(0x123, 0x6, 8)\n",
              "tlcl calls");

  ResetMocks(1, TPM_E_BADINDEX);
  TEST_EQ(SafeDefineSpace(0x123, 6, 8), TPM_E_BADINDEX,
          "SafeDefineSpace() bad");
  TEST_STR_EQ(calls,
              "TlclDefineSpace(0x123, 0x6, 8)\n",
              "tlcl calls");

  ResetMocks(1, TPM_E_MAXNVWRITES);
  TEST_EQ(SafeDefineSpace(0x123, 6, 8), 0,
          "SafeDefineSpace() retry max writes");
  TEST_STR_EQ(calls,
              "TlclDefineSpace(0x123, 0x6, 8)\n"
              "TlclForceClear()\n"
              "TlclSetEnable()\n"
              "TlclSetDeactivated(0)\n"
              "TlclDefineSpace(0x123, 0x6, 8)\n",
              "tlcl calls");
}

/****************************************************************************/

/* Tests for one-time initialization */
static void OneTimeInitTest(void) {
  RollbackSpaceFirmware rsf;
  RollbackSpaceKernel rsk;

  /* Complete initialization */
  ResetMocks(0, 0);
  TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), 0, "OneTimeInitializeTPM()");
  TEST_STR_EQ(calls,
              "TlclSelfTestFull()\n"
              "TlclGetPermanentFlags()\n"
              "TlclFinalizePhysicalPresence()\n"
              "TlclSetNvLocked()\n"
              "TlclForceClear()\n"
              "TlclSetEnable()\n"
              "TlclSetDeactivated(0)\n"
              /* kernel space */
              "TlclDefineSpace(0x1008, 0x1, 13)\n"
              "TlclWrite(0x1008, 13)\n"
              /* firmware space */
              "TlclDefineSpace(0x1007, 0x8001, 10)\n"
              "TlclWrite(0x1007, 10)\n",
              "tlcl calls");
  TEST_EQ(mock_rsf.struct_version, ROLLBACK_SPACE_FIRMWARE_VERSION, "rsf ver");
  TEST_EQ(mock_rsf.flags, 0, "rsf flags");
  TEST_EQ(mock_rsf.fw_versions, 0, "rsf fw_versions");
  TEST_EQ(mock_rsk.struct_version, ROLLBACK_SPACE_KERNEL_VERSION, "rsk ver");
  TEST_EQ(mock_rsk.uid, ROLLBACK_SPACE_KERNEL_UID, "rsk uid");
  TEST_EQ(mock_rsk.kernel_versions, 0, "rsk kernel_versions");

  /* Physical presence already initialized */
  ResetMocks(0, 0);
  mock_pflags.physicalPresenceLifetimeLock = 1;
  TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), 0, "OneTimeInitializeTPM()");
  TEST_STR_EQ(calls,
              "TlclSelfTestFull()\n"
              "TlclGetPermanentFlags()\n"
              "TlclSetNvLocked()\n"
              "TlclForceClear()\n"
              "TlclSetEnable()\n"
              "TlclSetDeactivated(0)\n"
              /* kernel space */
              "TlclDefineSpace(0x1008, 0x1, 13)\n"
              "TlclWrite(0x1008, 13)\n"
              /* firmware space */
              "TlclDefineSpace(0x1007, 0x8001, 10)\n"
              "TlclWrite(0x1007, 10)\n",
              "tlcl calls");

  /* NV locking already initialized */
  ResetMocks(0, 0);
  mock_pflags.nvLocked = 1;
  TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), 0, "OneTimeInitializeTPM()");
  TEST_STR_EQ(calls,
              "TlclSelfTestFull()\n"
              "TlclGetPermanentFlags()\n"
              "TlclFinalizePhysicalPresence()\n"
              "TlclForceClear()\n"
              "TlclSetEnable()\n"
              "TlclSetDeactivated(0)\n"
              /* kernel space */
              "TlclDefineSpace(0x1008, 0x1, 13)\n"
              "TlclWrite(0x1008, 13)\n"
              /* firmware space */
              "TlclDefineSpace(0x1007, 0x8001, 10)\n"
              "TlclWrite(0x1007, 10)\n",
              "tlcl calls");

  /* Self test error */
  ResetMocks(1, TPM_E_IOERROR);
  TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), TPM_E_IOERROR,
          "OneTimeInitializeTPM() selftest");
  TEST_STR_EQ(calls,
              "TlclSelfTestFull()\n",
              "tlcl calls");
}


/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  MiscTest();
  OneTimeInitTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
