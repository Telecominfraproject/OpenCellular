/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_init
 */

#include <stdio.h>
#include <stdlib.h>

#include "host_common.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Mock data */
static VbCommonParams cparams;
static VbInitParams iparams;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader* shared = (VbSharedDataHeader*)shared_data;
static uint64_t mock_timer;
static int rollback_s3_retval;
static int nv_write_called;

/* Reset mock data (for use before each test) */
static void ResetMocks(void) {
  Memset(&cparams, 0, sizeof(cparams));
  cparams.shared_data_size = sizeof(shared_data);
  cparams.shared_data_blob = shared_data;

  Memset(&iparams, 0, sizeof(iparams));

  Memset(&vnc, 0, sizeof(vnc));
  VbNvSetup(&vnc);
  VbNvTeardown(&vnc);  /* So CRC gets generated */

  Memset(&shared_data, 0, sizeof(shared_data));
  VbSharedDataInit(shared, sizeof(shared_data));

  mock_timer = 10;
  rollback_s3_retval = TPM_SUCCESS;
  nv_write_called = 0;
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

uint32_t RollbackS3Resume(void) {
  return rollback_s3_retval;
}

/****************************************************************************/
/* Test VbInit() and check expected return value and recovery reason */
static void TestVbInit(VbError_t expected_retval,
                       uint8_t expected_recovery, const char* desc) {
  uint32_t rr = 256;

  TEST_EQ(VbInit(&cparams, &iparams), expected_retval, desc);
  VbNvGet(&vnc, VBNV_RECOVERY_REQUEST, &rr);
  TEST_EQ(rr, expected_recovery, "  recovery request");
}

/****************************************************************************/

static void VbInitTest(void) {
  uint32_t u;

  /* Test passing in too small a shared data area */
  ResetMocks();
  cparams.shared_data_size = VB_SHARED_DATA_MIN_SIZE - 1;
  TestVbInit(VBERROR_INIT_SHARED_DATA, 0, "Shared data too small");

  /* Normal call; dev=0 rec=0 */
  ResetMocks();
  TestVbInit(0, 0, "Normal call");
  TEST_EQ(shared->timer_vb_init_enter, 21, "  time enter");
  TEST_EQ(shared->timer_vb_init_exit, 43, "  time exit");
  TEST_EQ(shared->flags, 0, "  shared flags");
  TEST_EQ(iparams.out_flags, 0, "  out flags");
  TEST_EQ(nv_write_called, 0, "  NV write not called since nothing changed");

  /* If NV data is trashed, we initialize it */
  ResetMocks();
  VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
  /* Note that we're not doing a VbNvTeardown(), so the CRC hasn't
   * been regenerated yet.  So VbInit() should ignore the corrupted
   * recovery value and boot normally. */
  TestVbInit(0, 0, "NV data trashed");
  TEST_EQ(nv_write_called, 1, "  NV write called");

  /* Test boot switch flags which are just passed through to shared
   * flags, and don't have an effect on VbInit(). */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_WP_ENABLED;
  TestVbInit(0, 0, "Flags test WP");
  TEST_EQ(shared->flags, VBSD_BOOT_FIRMWARE_WP_ENABLED, "  shared flags WP");

  ResetMocks();
  iparams.flags = VB_INIT_FLAG_RO_NORMAL_SUPPORT;
  TestVbInit(0, 0, "  flags test RO normal");
  TEST_EQ(shared->flags, VBSD_BOOT_RO_NORMAL_SUPPORT,
          "  shared flags RO normal");

  /* S3 resume */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_S3_RESUME;
  VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
  VbNvTeardown(&vnc);
  /* S3 resume doesn't clear the recovery request (or act on it) */
  TestVbInit(0, 123, "S3 resume");
  TEST_EQ(shared->flags, VBSD_BOOT_S3_RESUME, "  shared flags S3");
  TEST_EQ(iparams.out_flags, 0, "  out flags");
  TEST_EQ(shared->recovery_reason, 0, "  S3 doesn't look at recovery request");

  /* S3 resume with TPM resume error */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_S3_RESUME;
  rollback_s3_retval = 1;
  /* S3 resume doesn't clear the recovery request (or act on it) */
  TestVbInit(VBERROR_TPM_S3_RESUME, 0, "S3 resume rollback error");

  /* Normal boot doesn't care about TPM resume error because it
   * doesn't call RollbackS3Resume() */
  ResetMocks();
  rollback_s3_retval = 1;
  TestVbInit(0, 0, "Normal doesn't S3 resume");

  /* S3 resume with debug reset */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_S3_RESUME;
  VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 1);
  VbNvTeardown(&vnc);
  TestVbInit(0, 0, "S3 debug reset");
  TEST_EQ(iparams.out_flags, VB_INIT_OUT_S3_DEBUG_BOOT, "  out flags");
  VbNvGet(&vnc, VBNV_DEBUG_RESET_MODE, &u);
  TEST_EQ(u, 0, "  S3 clears nv debug reset mode");

  /* Normal boot clears S3 debug reset mode, but doesn't set output flag */
  ResetMocks();
  VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 1);
  VbNvTeardown(&vnc);
  TestVbInit(0, 0, "Normal with debug reset mode");
  TEST_EQ(iparams.out_flags, 0, "  out flags");
  VbNvGet(&vnc, VBNV_DEBUG_RESET_MODE, &u);
  TEST_EQ(u, 0, "  normal clears nv debug reset mode");

  /* S3 resume with debug reset is a normal boot, so doesn't resume the TPM */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_S3_RESUME;
  rollback_s3_retval = 1;
  VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 1);
  VbNvTeardown(&vnc);
  TestVbInit(0, 0, "S3 debug reset rollback error");

  /* Developer mode */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON;
  TestVbInit(0, 0, "Dev mode on");
  TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
  TEST_EQ(iparams.out_flags,
          VB_INIT_OUT_CLEAR_RAM |
          VB_INIT_OUT_ENABLE_DISPLAY |
          VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
  TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");

  /* Recovery mode from NV storage */
  ResetMocks();
  VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
  VbNvTeardown(&vnc);
  TestVbInit(0, 0, "Recovery mode - from nv");
  TEST_EQ(shared->recovery_reason, 123, "  recovery reason");
  TEST_EQ(iparams.out_flags,
          VB_INIT_OUT_ENABLE_RECOVERY |
          VB_INIT_OUT_CLEAR_RAM |
          VB_INIT_OUT_ENABLE_DISPLAY |
          VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
  TEST_EQ(shared->flags, 0, "  shared flags");

  /* Recovery mode from recovery button */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED;
  TestVbInit(0, 0, "Recovery mode - button");
  TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_MANUAL,
          "  recovery reason");
  TEST_EQ(iparams.out_flags,
          VB_INIT_OUT_ENABLE_RECOVERY |
          VB_INIT_OUT_CLEAR_RAM |
          VB_INIT_OUT_ENABLE_DISPLAY |
          VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
  TEST_EQ(shared->flags, VBSD_BOOT_REC_SWITCH_ON, "  shared flags");

  /* Recovery button reason supersedes NV reason */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED;
  VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
  VbNvTeardown(&vnc);
  TestVbInit(0, 0, "Recovery mode - button AND nv");
  TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_MANUAL,
          "  recovery reason");

  /* Recovery mode from previous boot fail */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_PREVIOUS_BOOT_FAIL;
  TestVbInit(0, 0, "Recovery mode - previous boot fail");
  TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_FIRMWARE,
          "  recovery reason");
  TEST_EQ(iparams.out_flags,
          VB_INIT_OUT_ENABLE_RECOVERY |
          VB_INIT_OUT_CLEAR_RAM |
          VB_INIT_OUT_ENABLE_DISPLAY |
          VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
  TEST_EQ(shared->flags, 0, "  shared flags");

  /* Recovery mode from NV supersedes previous boot fail */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_PREVIOUS_BOOT_FAIL;
  VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
  VbNvTeardown(&vnc);
  TestVbInit(0, 0, "Recovery mode - previous boot fail AND nv");
  TEST_EQ(shared->recovery_reason, 123, "  recovery reason");

  /* Dev + recovery = recovery */
  ResetMocks();
  iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED | VB_INIT_FLAG_DEV_SWITCH_ON;
  TestVbInit(0, 0, "Recovery mode - button");
  TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_MANUAL,
          "  recovery reason");
  TEST_EQ(iparams.out_flags,
          VB_INIT_OUT_ENABLE_RECOVERY |
          VB_INIT_OUT_CLEAR_RAM |
          VB_INIT_OUT_ENABLE_DISPLAY |
          VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
  TEST_EQ(shared->flags,
          VBSD_BOOT_REC_SWITCH_ON | VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");
}


/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  VbInitTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
