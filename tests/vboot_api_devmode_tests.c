/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_firmware
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "crc32.h"
#include "gbb_header.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"


/* Expected results */

#define MAX_NOTE_EVENTS 10
#define TICKS_PER_MSEC 1900ULL
#define TIME_FUZZ 500
#define KBD_READ_TIME 60

typedef struct {
  uint16_t msec;
  uint16_t freq;
  int time;
} note_event_t;

typedef struct {
  char *name;
  uint32_t gbb_flags;
  VbError_t beep_return;
  uint32_t keypress_key;
  int keypress_at_count;
  int num_events;
  note_event_t notes[MAX_NOTE_EVENTS];
} test_case_t;

test_case_t test[] = {

  { "VbBootDeveloperSoundTest( fast, background )",
    0x00000001, VBERROR_SUCCESS,
    0, 0,
    2,
    {
      {0, 0, 0},                        // probing for capability
      {0, 0, 2000},                     // off and return at 2 seconds
    }},

  { "VbBootDeveloperSoundTest( normal, background )",
    0x00000000, VBERROR_SUCCESS,
    0, 0,
    6,
    {
      {0, 0, 0},                        // probing for capability
      {0, 400, 20000},                  // starts first beep at 20 seconds
      {0, 0, 20250},                    // stops 250ms later
      {0, 400, 20500},                  // starts second beep
      {0, 0, 20750},                    // stops 250ms later
      {0, 0, 30000},                    // off and return at 30 seconds
    }},

  { "VbBootDeveloperSoundTest( fast, no background )",
    0x00000001, VBERROR_NO_BACKGROUND_SOUND,
    0, 0,
    2,
    {
      {0, 0, 0},                        // probing for capability
      {0, 0, 2000},                     // off and return at 2 seconds
    }},

  { "VbBootDeveloperSoundTest( normal, no background )",
    0x00000000, VBERROR_NO_BACKGROUND_SOUND,
    0, 0,
    4,
    {
      {0, 0, 0},                        // probing for capability
      {250, 400, 20000},                // first beep at 20 seconds
      {250, 400, 20510},                // second beep shortly after
      {0, 0, 30020},                    // off and return at 30 seconds
    }},

  // Now with some keypresses

  { "VbBootDeveloperSoundTest( normal, background, Ctrl-D )",
    0x00000000, VBERROR_SUCCESS,
    4, 10000,                           // Ctrl-D at 10 seconds
    2,
    {
      {0, 0, 0},                        // probing for capability
      {0, 0, 10000},                    // sees Ctrl-D, sound off, return
    }},

  { "VbBootDeveloperSoundTest( normal, no background, Ctrl-D )",
    0x00000000, VBERROR_NO_BACKGROUND_SOUND,
    4, 20400,                           // Ctrl-D between beeps
    3,
    {
      {0, 0, 0},                        // probing for capability
      {250, 400, 20000},                // first beep at 20 seconds
      {0, 0, 20400},                    // sees Ctrl-D, sound off, return
    }},

  { "VbBootDeveloperSoundTest( normal, background, Ctrl-U not allowed )",
    0x00000000, VBERROR_SUCCESS,
    21, 10000,                          // Ctrl-U at 10 seconds
    8,
    {
      {0, 0, 0},                        // probing for capability
      {120, 400, 10000},                // complains about Ctrl-U (one beep)
                                        // waits 120ms...
      {120, 400, 10240},                // complains about Ctrl-U (two beeps)
                                        // original sequence is now shifted...
      {0, 400, 20360},                  // starts first beep at 20 seconds
      {0, 0, 20610},                    // stops 250ms later
      {0, 400, 20860},                  // starts second beep
      {0, 0, 21110},                    // stops 250ms later
      {0, 0, 30360},                    // returns at 30 seconds + 360ms
    }},

};

/* Mock data */
static VbCommonParams cparams;
static LoadKernelParams lkparams;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader* shared = (VbSharedDataHeader*)shared_data;
static GoogleBinaryBlockHeader gbb;
static int current_time;
static uint64_t current_ticks;
static int current_event;
static int max_events;
static int matched_events;
static int kbd_fire_at;
static uint32_t kbd_fire_key;
static VbError_t beep_return;
static note_event_t *expected_event;

/* Reset mock data (for use before each test) */
static void ResetMocks(void) {

  Memset(&cparams, 0, sizeof(cparams));
  cparams.shared_data_size = sizeof(shared_data);
  cparams.shared_data_blob = shared_data;
  cparams.gbb_data = &gbb;
  cparams.gbb = &gbb;

  Memset(&lkparams, 0, sizeof(lkparams));

  Memset(&vnc, 0, sizeof(vnc));
  VbNvSetup(&vnc);
  VbNvTeardown(&vnc);  /* So CRC gets generated */

  Memset(&shared_data, 0, sizeof(shared_data));
  VbSharedDataInit(shared, sizeof(shared_data));
  shared->fw_keyblock_flags = 0xABCDE0;

  Memset(&gbb, 0, sizeof(gbb));
  gbb.major_version = GBB_MAJOR_VER;
  gbb.minor_version = GBB_MINOR_VER;
  gbb.flags = 0;

  current_ticks = 0;
  current_time = 0;

  current_event = 0;
  kbd_fire_at = 0;
  kbd_fire_key = 0;

  beep_return = VBERROR_SUCCESS;

  matched_events = 0;
  max_events = 0;
}

/****************************************************************************/
/* Mocked verification functions */

VbError_t VbExNvStorageRead(uint8_t* buf) {
  Memcpy(buf, vnc.raw, sizeof(vnc.raw));
  return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageWrite(const uint8_t* buf) {
  Memcpy(vnc.raw, buf, sizeof(vnc.raw));
  return VBERROR_SUCCESS;
}

VbError_t VbExDiskGetInfo(VbDiskInfo** infos_ptr, uint32_t* count,
                          uint32_t disk_flags) {
  return VBERROR_UNKNOWN;
}

VbError_t VbExDiskFreeInfo(VbDiskInfo* infos,
                           VbExDiskHandle_t preserve_handle) {
  return VBERROR_SUCCESS;
}

VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void* buffer) {
  return VBERROR_UNKNOWN;
}

VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
                        uint64_t lba_count, const void* buffer) {
  return VBERROR_UNKNOWN;
}

uint32_t VbExIsShutdownRequested(void) {
  return 0;
}

uint32_t VbExKeyboardRead(void) {
  uint32_t tmp;
  uint32_t now;

  VbExSleepMs(KBD_READ_TIME);
  now = current_time;

  if (kbd_fire_key && now >= kbd_fire_at) {
    VBDEBUG(("  VbExKeyboardRead() - returning %d at %d msec\n",
             kbd_fire_key, now));
    tmp = kbd_fire_key;
    kbd_fire_key = 0;
    return tmp;
  }
  VBDEBUG(("  VbExKeyboardRead() - returning %d at %d msec\n",
           0, now));
  return 0;
}

void VbExSleepMs(uint32_t msec) {
  current_ticks += (uint64_t)msec * TICKS_PER_MSEC;
  current_time = current_ticks / TICKS_PER_MSEC;
  VBDEBUG(("VbExSleepMs(%d) -> %d\n", msec, current_time));
}

uint64_t VbExGetTimer(void) {
  return current_ticks;
}

VbError_t VbExBeep(uint32_t msec, uint32_t frequency) {
  VBDEBUG(("VbExBeep(%d, %d) at %d msec\n", msec, frequency, current_time));

  if (current_event < max_events &&
      msec == expected_event[current_event].msec &&
      frequency == expected_event[current_event].freq &&
      abs(current_time - expected_event[current_event].time) < TIME_FUZZ ) {
    matched_events++;
  }

  if (msec)
    VbExSleepMs(msec);
  current_event++;
  return beep_return;
}

VbError_t VbExDisplayScreen(uint32_t screen_type) {
  switch(screen_type) {
  case VB_SCREEN_BLANK:
    VBDEBUG(("VbExDisplayScreen(BLANK)\n"));
    break;
  case VB_SCREEN_DEVELOPER_WARNING:
    VBDEBUG(("VbExDisplayScreen(DEV)\n"));
    break;
  case VB_SCREEN_DEVELOPER_EGG:
    VBDEBUG(("VbExDisplayScreen(EGG)\n"));
    break;
  case VB_SCREEN_RECOVERY_REMOVE:
    VBDEBUG(("VbExDisplayScreen(REMOVE)\n"));
    break;
  case VB_SCREEN_RECOVERY_INSERT:
    VBDEBUG(("VbExDisplayScreen(INSERT)\n"));
    break;
  case VB_SCREEN_RECOVERY_NO_GOOD:
    VBDEBUG(("VbExDisplayScreen(NO_GOOD)\n"));
    break;
  default:
    VBDEBUG(("VbExDisplayScreen(%d)\n", screen_type));
  }

  VBDEBUG(("  current_time is %d msec\n", current_time));

  return VBERROR_SUCCESS;
}

/****************************************************************************/

VbError_t VbBootDeveloper(VbCommonParams* cparams, LoadKernelParams* p);


static void VbBootDeveloperSoundTest(void) {
  int i;
  int num_tests =  sizeof(test) / sizeof(test_case_t);

  for (i=0; i<num_tests; i++) {
    VBDEBUG(("STARTING %s ...\n", test[i].name));
    ResetMocks();
    gbb.flags = test[i].gbb_flags;
    beep_return = test[i].beep_return;
    kbd_fire_key = test[i].keypress_key;
    kbd_fire_at = test[i].keypress_at_count;
    max_events = test[i].num_events;
    expected_event = test[i].notes;
    (void) VbBootDeveloper(&cparams, &lkparams);
    VBDEBUG(("INFO: matched %d total %d expected %d\n",
             matched_events, current_event, test[i].num_events));
    TEST_TRUE(matched_events == test[i].num_events &&
              current_event == test[i].num_events, test[i].name);
  }
}


int main(int argc, char* argv[]) {
  int error_code = 0;

  VbBootDeveloperSoundTest();

  if (!gTestSuccess)
    error_code = 255;
  if (vboot_api_stub_check_memory())
    error_code = 255;

  return error_code;
}
