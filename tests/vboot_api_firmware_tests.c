/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_firmware
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbb_header.h"
#include "host_common.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Flags for mock_*_got_flags variables */
#define MOCK_DEV_FLAG 0x01     /* Developer parameter non-zero */
#define MOCK_REC_FLAG 0x02     /* Recovery parameter non-zero */

/* Mock data */
static VbCommonParams cparams;
static VbSelectFirmwareParams fparams;
static GoogleBinaryBlockHeader gbb;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader* shared = (VbSharedDataHeader*)shared_data;
static uint64_t mock_timer;
static int nv_write_called;
/* Mock TPM versions */
static uint32_t mock_tpm_version;
static uint32_t mock_lf_tpm_version;  /* TPM version set by LoadFirmware() */
/* Variables for tracking params passed to mock functions */
static uint32_t mock_stbms_got_flags;
static uint64_t mock_stbms_got_fw_flags;
static int mock_rfl_called;
/* Mock return values, so we can simulate errors */
static VbError_t mock_rfw_retval;
static VbError_t mock_rfl_retval;
static VbError_t mock_lf_retval;
static VbError_t mock_stbms_retval;

/* Reset mock data (for use before each test) */
static void ResetMocks(void) {
  Memset(&cparams, 0, sizeof(cparams));
  cparams.shared_data_size = sizeof(shared_data);
  cparams.shared_data_blob = shared_data;

  Memset(&fparams, 0, sizeof(fparams));

  Memset(&gbb, 0, sizeof(gbb));
  cparams.gbb_data = &gbb;
  cparams.gbb_size = sizeof(gbb);
  cparams.gbb = &gbb;

  Memset(&vnc, 0, sizeof(vnc));
  VbNvSetup(&vnc);
  VbNvTeardown(&vnc);  /* So CRC gets generated */

  Memset(&shared_data, 0, sizeof(shared_data));
  VbSharedDataInit(shared, sizeof(shared_data));
  shared->fw_keyblock_flags = 0xABCDE0;

  mock_timer = 10;
  nv_write_called = mock_rfl_called = 0;

  mock_stbms_got_flags = 0;
  mock_stbms_got_fw_flags = 0;

  mock_tpm_version = mock_lf_tpm_version = 0x20004;
  shared->fw_version_tpm_start = mock_tpm_version;
  mock_rfw_retval = mock_rfl_retval = 0;
  mock_lf_retval = mock_stbms_retval = 0;
}

/****************************************************************************/
/* Mocked verification functions */

VbError_t VbExNvStorageRead(uint8_t* buf) {
  Memcpy(buf, vnc.raw, sizeof(vnc.raw));
  return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageWrite(const uint8_t* buf) {
  nv_write_called = 1;
  Memcpy(vnc.raw, buf, sizeof(vnc.raw));
  return VBERROR_SUCCESS;
}

uint64_t VbExGetTimer(void) {
  /* Exponential-ish rather than linear time, so that subtracting any
   * two mock values will yield a unique result. */
  uint64_t new_timer = mock_timer * 2 + 1;
  VbAssert(new_timer > mock_timer);  /* Make sure we don't overflow */
  mock_timer = new_timer;
  return mock_timer;
}

uint32_t RollbackFirmwareWrite(uint32_t version) {
  mock_tpm_version = version;
  return mock_rfw_retval;
}

uint32_t RollbackFirmwareLock(void) {
  mock_rfl_called = 1;
  return mock_rfl_retval;
}

uint32_t SetTPMBootModeState(int developer_mode, int recovery_mode,
			     uint64_t fw_keyblock_flags,
			     GoogleBinaryBlockHeader *gbb) {
  if (recovery_mode)
    mock_stbms_got_flags |= MOCK_REC_FLAG;
  if (developer_mode)
    mock_stbms_got_flags |= MOCK_DEV_FLAG;

  mock_stbms_got_fw_flags = fw_keyblock_flags;

  return mock_stbms_retval;
}

int LoadFirmware(VbCommonParams* cparams, VbSelectFirmwareParams* fparams,
                 VbNvContext* vnc) {
  shared->fw_version_tpm = mock_lf_tpm_version;
  return mock_lf_retval;
}


/****************************************************************************/
/* Test VbSelectFirmware() and check expected return value and
 * recovery reason */
static void TestVbSf(VbError_t expected_retval,
                     uint8_t expected_recovery, const char* desc) {
  uint32_t rr = 256;

  TEST_EQ(VbSelectFirmware(&cparams, &fparams), expected_retval, desc);
  VbNvGet(&vnc, VBNV_RECOVERY_REQUEST, &rr);
  TEST_EQ(rr, expected_recovery, "  recovery request");
}

/****************************************************************************/

static void VbSelectFirmwareTest(void) {
  /* Normal call */
  ResetMocks();
  TestVbSf(0, 0, "Normal call");
  TEST_EQ(shared->timer_vb_select_firmware_enter, 21, "  time enter");
  TEST_EQ(shared->timer_vb_select_firmware_exit, 43, "  time exit");
  TEST_EQ(nv_write_called, 0, "  NV write not called since nothing changed");
  TEST_EQ(mock_stbms_got_flags, 0, "  SetTPMBootModeState() flags");
  TEST_EQ(mock_stbms_got_fw_flags, 0xABCDE0, "  fw keyblock flags");
  TEST_EQ(mock_rfl_called, 1, "  RollbackFirmwareLock() called");

  /* Developer mode call */
  ResetMocks();
  shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
  TestVbSf(0, 0, "Developer mode");
  TEST_EQ(mock_stbms_got_flags, MOCK_DEV_FLAG, "  SetTPMBootModeState() flags");
  TEST_EQ(mock_rfl_called, 1, "  RollbackFirmwareLock() called");

  /* Recovery mode doesn't call LoadFirmware(),
   * RollbackFirmwareWrite(), or RollbackFirmwareLock(). */
  ResetMocks();
  shared->recovery_reason = VBNV_RECOVERY_US_TEST;
  mock_lf_retval = VBERROR_UNKNOWN;
  mock_rfw_retval = mock_rfl_retval = TPM_E_IOERROR;
  TestVbSf(0, 0, "Recovery mode");
  TEST_EQ(fparams.selected_firmware, VB_SELECT_FIRMWARE_RECOVERY,
          "  select recovery");
  TEST_EQ(mock_stbms_got_flags, MOCK_REC_FLAG, "  SetTPMBootModeState() flags");
  TEST_EQ(mock_rfl_called, 0, "  RollbackFirmwareLock() not called");

  /* Dev + recovery */
  ResetMocks();
  shared->recovery_reason = VBNV_RECOVERY_US_TEST;
  shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
  TestVbSf(0, 0, "Recovery+developer mode");
  TEST_EQ(fparams.selected_firmware, VB_SELECT_FIRMWARE_RECOVERY,
          "  select recovery");
  TEST_EQ(mock_stbms_got_flags, MOCK_DEV_FLAG|MOCK_REC_FLAG,
          "  SetTPMBootModeState() flags");
  TEST_EQ(mock_rfl_called, 0, "  RollbackFirmwareLock() not called");

  /* LoadFirmware() error code passed through */
  ResetMocks();
  mock_lf_retval = 0x12345;
  TestVbSf(0x12345, 0, "LoadFirmware() error");

  /* Select different firmware paths based on LoadFirmware() result */
  ResetMocks();
  shared->flags |= VBSD_LF_USE_RO_NORMAL;
  TestVbSf(0, 0, "LoadFirmware() RO-normal");
  TEST_EQ(fparams.selected_firmware, VB_SELECT_FIRMWARE_READONLY,
          "  select RO normal");
  ResetMocks();
  shared->firmware_index = 0;
  TestVbSf(0, 0, "LoadFirmware() A");
  TEST_EQ(fparams.selected_firmware, VB_SELECT_FIRMWARE_A, "  select A");
  ResetMocks();
  shared->firmware_index = 1;
  TestVbSf(0, 0, "LoadFirmware() B");
  TEST_EQ(fparams.selected_firmware, VB_SELECT_FIRMWARE_B, "  select B");

  /* Handle TPM version updates */
  ResetMocks();
  mock_lf_tpm_version = 0x30005;
  TestVbSf(0, 0, "TPM version update");
  TEST_EQ(shared->fw_version_tpm_start, 0x20004, "  TPM version start");
  TEST_EQ(shared->fw_version_tpm, 0x30005, "  TPM version");
  TEST_EQ(mock_tpm_version, 0x30005, "  TPM version written back");

  /* Check error writing TPM version */
  ResetMocks();
  mock_lf_tpm_version = 0x30005;
  mock_rfw_retval = TPM_E_IOERROR;
  TestVbSf(VBERROR_TPM_WRITE_FIRMWARE, VBNV_RECOVERY_RO_TPM_W_ERROR,
           "TPM version update failure");

  /* If no change to TPM version, RollbackFirmwareWrite() not called */
  ResetMocks();
  mock_rfw_retval = TPM_E_IOERROR;
  TestVbSf(0, 0, "LoadFirmware() TPM version not updated");
  TEST_EQ(shared->fw_version_tpm_start, 0x20004, "  TPM version start");
  TEST_EQ(shared->fw_version_tpm, 0x20004, "  TPM version");
  TEST_EQ(mock_tpm_version, 0x20004, "  TPM version (not) written back");

  /* Check errors from SetTPMBootModeState() */
  ResetMocks();
  mock_stbms_retval = TPM_E_IOERROR;
  TestVbSf(VBERROR_TPM_SET_BOOT_MODE_STATE, VBNV_RECOVERY_RO_TPM_U_ERROR,
           "TPM set boot mode state failure");
  ResetMocks();
  mock_stbms_retval = TPM_E_IOERROR;
  shared->recovery_reason = VBNV_RECOVERY_US_TEST;
  TestVbSf(0, 0, "TPM set boot mode state failure ignored in recovery");

  /* Handle RollbackFirmwareLock() errors */
  ResetMocks();
  mock_rfl_retval = TPM_E_IOERROR;
  TestVbSf(VBERROR_TPM_LOCK_FIRMWARE, VBNV_RECOVERY_RO_TPM_L_ERROR,
           "TPM lock firmware failure");
}


int main(int argc, char* argv[]) {
  int error_code = 0;

  VbSelectFirmwareTest();

  if (vboot_api_stub_check_memory())
    error_code = 255;
  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
