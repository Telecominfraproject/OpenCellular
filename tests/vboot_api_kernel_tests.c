/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for VbTryLoadKernel()
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "test_common.h"
#include "utility.h"
#include "vboot_api.h"


#define MAX_TEST_DISKS 10
#define DEFAULT_COUNT -1

typedef struct {
    uint64_t bytes_per_lba;
    uint64_t lba_count;
    uint32_t flags;
    const char *diskname;
} disk_desc_t;

typedef struct {
  char *name;

  /* inputs for test case */
  uint32_t want_flags;
  VbError_t diskgetinfo_return_val;
  disk_desc_t disks_to_provide[MAX_TEST_DISKS];
  int disk_count_to_return;
  VbError_t loadkernel_return_val[MAX_TEST_DISKS];

  /* outputs from test */
  uint32_t expected_recovery_request_val;
  const char *expected_to_find_disk;
  const char *expected_to_load_disk;
  uint32_t expected_return_val;

} test_case_t;

/****************************************************************************/
/* Test cases */

static const char pickme[] = "correct choice";
#define DONT_CARE ((const char *)42)

test_case_t test[] = {

  {
    .name = "first removable drive",
    .want_flags = VB_DISK_FLAG_REMOVABLE,
    .disks_to_provide = {
      {512,   10,  VB_DISK_FLAG_REMOVABLE, 0 }, /* too small */
      {2048, 100,  VB_DISK_FLAG_REMOVABLE, 0 }, /* wrong LBA */
      {512,  100,  VB_DISK_FLAG_FIXED, 0 },     /* wrong type */
      {512,  100,  0, 0 },                      /* wrong flags */
      {512,  100,  -1, 0 },                     /* still wrong flags */
      {512,  100,  VB_DISK_FLAG_REMOVABLE, pickme},
      {512,  100,  VB_DISK_FLAG_REMOVABLE, "holygrail"}, /* already got one */
    },
    .disk_count_to_return = DEFAULT_COUNT,
    .diskgetinfo_return_val = VBERROR_SUCCESS,
    .loadkernel_return_val = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, },

    .expected_recovery_request_val = VBNV_RECOVERY_NOT_REQUESTED,
    .expected_to_find_disk = pickme,
    .expected_to_load_disk = pickme,
    .expected_return_val = VBERROR_SUCCESS
  },

  {
    .name = "second removable drive",
    .want_flags = VB_DISK_FLAG_REMOVABLE,
    .disks_to_provide = {
      {512,  100,  0, 0 },                      /* wrong flags */
      {512,  100,  VB_DISK_FLAG_REMOVABLE, "not yet"},
      {512,  100,  VB_DISK_FLAG_REMOVABLE, pickme},
    },
    .disk_count_to_return = DEFAULT_COUNT,
    .diskgetinfo_return_val = VBERROR_SUCCESS,
    .loadkernel_return_val = { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, },

    .expected_recovery_request_val = VBNV_RECOVERY_NOT_REQUESTED,
    .expected_to_find_disk = pickme,
    .expected_to_load_disk = pickme,
    .expected_return_val = VBERROR_SUCCESS
  },

  {
    .name = "first fixed drive",
    .want_flags = VB_DISK_FLAG_FIXED,
    .disks_to_provide = {
      {512,   10,  VB_DISK_FLAG_FIXED, 0 }, /* too small */
      {2048, 100,  VB_DISK_FLAG_FIXED, 0 }, /* wrong LBA */
      {512,  100,  VB_DISK_FLAG_REMOVABLE, 0 }, /* wrong type */
      {512,  100,  0, 0 },                      /* wrong flags */
      {512,  100,  -1, 0 },                     /* still wrong flags */
      {512,  100,  VB_DISK_FLAG_REMOVABLE|VB_DISK_FLAG_FIXED, 0 }, /* flags */
      {512,  100,  VB_DISK_FLAG_FIXED, pickme},
      {512,  100,  VB_DISK_FLAG_FIXED, "holygrail"}, /* already got one */
    },
    .disk_count_to_return = DEFAULT_COUNT,
    .diskgetinfo_return_val = VBERROR_SUCCESS,
    .loadkernel_return_val = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, },

    .expected_recovery_request_val = VBNV_RECOVERY_NOT_REQUESTED,
    .expected_to_find_disk = pickme,
    .expected_to_load_disk = pickme,
    .expected_return_val = VBERROR_SUCCESS
  },

  {
    .name = "no drives at all",
    .want_flags = VB_DISK_FLAG_FIXED,
    .disks_to_provide = {
    },
    .disk_count_to_return = DEFAULT_COUNT,
    .diskgetinfo_return_val = VBERROR_SUCCESS,
    .loadkernel_return_val = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },

    .expected_recovery_request_val = VBNV_RECOVERY_RW_NO_DISK,
    .expected_to_find_disk = 0,
    .expected_to_load_disk = 0,
    .expected_return_val = VBERROR_NO_DISK_FOUND
  },

  {
    .name = "no valid drives",
    .want_flags = VB_DISK_FLAG_FIXED,
    .disks_to_provide = {
      {512,   10,  VB_DISK_FLAG_FIXED, 0 }, /* too small */
      {2048, 100,  VB_DISK_FLAG_FIXED, 0 }, /* wrong LBA */
      {512,  100,  VB_DISK_FLAG_REMOVABLE, 0 }, /* wrong type */
      {512,  100,  0, 0 },                      /* wrong flags */
      {512,  100,  -1, 0 },                     /* still wrong flags */
      {512,  100,  VB_DISK_FLAG_FIXED, "bad1"}, /* doesn't load */
      {512,  100,  VB_DISK_FLAG_FIXED, "bad2"}, /* doesn't load */
    },
    .disk_count_to_return = DEFAULT_COUNT,
    .diskgetinfo_return_val = VBERROR_SUCCESS,
    .loadkernel_return_val = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },

    .expected_recovery_request_val = VBNV_RECOVERY_RW_NO_DISK,
    .expected_to_find_disk = DONT_CARE,
    .expected_to_load_disk = 0,
    .expected_return_val = 1
  },

};


/****************************************************************************/

/* Mock data */
static LoadKernelParams lkparams;
static VbDiskInfo mock_disks[MAX_TEST_DISKS];
static test_case_t *t;
static int load_kernel_calls;
static uint32_t got_recovery_request_val;
static const char *got_find_disk;
static const char *got_load_disk;
static uint32_t got_return_val;

/* Reset mock data (for use before each test) */
static void ResetMocks(int i) {
  Memset(&lkparams, 0, sizeof(lkparams));
  Memset(&mock_disks, 0, sizeof(mock_disks));
  load_kernel_calls = 0;

  got_recovery_request_val = VBNV_RECOVERY_NOT_REQUESTED;
  got_find_disk = 0;
  got_load_disk = 0;
  got_return_val = 0xdeadbeef;

  t = test+i;
}

int is_nonzero(const void *vptr, size_t count) {
  const char *p = (const char *)vptr;
  while (count--)
    if (*p++)
      return 1;

  return 0;
}

int CheckResults(void) {
  VBDEBUG(("%s()\n", __FUNCTION__));
  VBDEBUG(("  recovery_request: %x %x\n",
           t->expected_recovery_request_val, got_recovery_request_val));
  VBDEBUG(("  find_disk: (%s) (%s)\n",
           (t->expected_to_find_disk == DONT_CARE
            ? "DONT CARE"
            : t->expected_to_find_disk),
           got_find_disk));
  VBDEBUG(("  load_disk: (%s) (%s)\n",
           (t->expected_to_load_disk == DONT_CARE
            ? "DONT CARE"
            : t->expected_to_load_disk),
           got_load_disk));
  VBDEBUG(("  return_val: %x %x\n",
           t->expected_return_val, got_return_val));
  return (t->expected_recovery_request_val == got_recovery_request_val &&
          (t->expected_to_find_disk == DONT_CARE ||
           t->expected_to_find_disk == got_find_disk) &&
          (t->expected_to_load_disk == DONT_CARE ||
           t->expected_to_load_disk == got_load_disk) &&
          t->expected_return_val == got_return_val);
}

/****************************************************************************/
/* Mocked verification functions */

VbError_t VbExDiskGetInfo(VbDiskInfo** infos_ptr, uint32_t* count,
                          uint32_t disk_flags) {
  int i;
  int num_disks = 0;

  VBDEBUG(("My %s\n", __FUNCTION__));

  *infos_ptr = mock_disks;

  for(i=0; i<MAX_TEST_DISKS; i++) {
    if (is_nonzero(&t->disks_to_provide[i], sizeof(t->disks_to_provide[i]))) {
      mock_disks[num_disks].bytes_per_lba =
        t->disks_to_provide[i].bytes_per_lba;
      mock_disks[num_disks].lba_count = t->disks_to_provide[i].lba_count;
      mock_disks[num_disks].flags = t->disks_to_provide[i].flags;
      mock_disks[num_disks].handle =
        (VbExDiskHandle_t)t->disks_to_provide[i].diskname;
      VBDEBUG(("  mock_disk[%d] %Ld %Ld 0x%x %s\n", i,
               mock_disks[num_disks].bytes_per_lba,
               mock_disks[num_disks].lba_count,
               mock_disks[num_disks].flags,
               (mock_disks[num_disks].handle
                ? (char *)mock_disks[num_disks].handle
                : "0")));
      num_disks++;
    } else {
      mock_disks[num_disks].handle = (VbExDiskHandle_t)"INVALID";
    }
  }

  if (t->disk_count_to_return >= 0)
    *count = t->disk_count_to_return;
  else
    *count = num_disks;

  VBDEBUG(("  *count=%Ld\n", *count));
  VBDEBUG(("  return 0x%x\n", t->diskgetinfo_return_val));

  return t->diskgetinfo_return_val;
}

VbError_t VbExDiskFreeInfo(VbDiskInfo* infos,
                           VbExDiskHandle_t preserve_handle) {
  got_load_disk = (const char *)preserve_handle;
  VBDEBUG(("%s(): got_load_disk = %s\n", __FUNCTION__,
           got_load_disk ? got_load_disk : "0"));
  return VBERROR_SUCCESS;
}

VbError_t LoadKernel(LoadKernelParams* params) {
  got_find_disk = (const char *)params->disk_handle;
  VBDEBUG(("%s(%d): got_find_disk = %s\n", __FUNCTION__,
           load_kernel_calls,
           got_find_disk ? got_find_disk : "0"));
  return t->loadkernel_return_val[load_kernel_calls++];
}

int VbNvSet(VbNvContext* context, VbNvParam param, uint32_t value) {
  VBDEBUG(("%s(): got_recovery_request_val = %d (0x%x)\n", __FUNCTION__,
           value, value));
  got_recovery_request_val = value;
  return 0;
}

/****************************************************************************/



/* This is not declared in any headers, so declare it here. */
uint32_t VbTryLoadKernel(VbCommonParams* cparams, LoadKernelParams* p,
                         uint32_t get_info_flags);


static void VbTryLoadKernelTest(void) {
  int i;
  int num_tests =  sizeof(test) / sizeof(test[0]);

  for (i=0; i<num_tests; i++) {
    VBDEBUG(("STARTING %s ...\n", test[i].name));
    ResetMocks(i);
    got_return_val = VbTryLoadKernel(0, &lkparams, test[i].want_flags);
    VBDEBUG(("VbTryLoadKernel(): got_return_val = 0x%x\n", got_return_val));
    TEST_TRUE(CheckResults(), test[i].name);
  }
}


/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  VbTryLoadKernelTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
