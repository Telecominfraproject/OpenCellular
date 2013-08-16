/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_audio
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
#include "vboot_audio.h"
#include "vboot_audio_private.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"


/* Builtin notes */
extern VbDevMusicNote default_notes_[], short_notes_[];
extern uint32_t default_count_, short_count_;

/* Mock data */
static VbCommonParams cparams;
static GoogleBinaryBlockHeader gbb;
static VbDevMusicNote good_notes[] = { {100, 100},
                                       {100, 0},
                                       {200, 200},
                                       {100, 0},
                                       {300, 300},
                                       {100, 0},
                                       {400, 400},
                                       {30000, 0} };
static VbDevMusic good_header =
{ .sig = { '$', 'S', 'N', 'D' },
  .count = sizeof(good_notes) / sizeof(VbDevMusicNote),
};

static uint8_t notebuf[sizeof(good_header) +
                       sizeof(good_notes) - sizeof(VbDevMusicNote)];

static VbDevMusic *use_hdr;
static VbDevMusicNote *use_notes;
static uint32_t use_size;

/* Set correct checksum for custom notes */
void FixChecksum(VbDevMusic *hdr) {
  hdr->checksum = Crc32(&(hdr->count), sizeof(hdr->count) +
                        hdr->count * sizeof(hdr->notes[0]));
}

/* Reset mock data (for use before each test) */
static void ResetMocks(void) {
  VBDEBUG(("ResetMocks()\n"));
  Memset(&cparams, 0, sizeof(cparams));
  cparams.gbb_data = &gbb;
  cparams.gbb = &gbb;
  Memset(&gbb, 0, sizeof(gbb));
  gbb.major_version = GBB_MAJOR_VER;
  gbb.minor_version = GBB_MINOR_VER;
  gbb.flags = 0;
  use_hdr = (VbDevMusic *)notebuf;
  use_notes = use_hdr->notes;
  Memcpy(use_hdr, &good_header, sizeof(good_header));
  Memcpy(use_notes, good_notes, sizeof(good_notes));
  FixChecksum(use_hdr);
  use_size = sizeof(notebuf);
}

/* Compare two sets of notes */
static int NotesMatch(VbDevMusicNote *a, VbDevMusicNote *b, uint32_t count) {
  int i;
  if (!a || !b)
    return 0;

  for ( i=0; i<count; i++) {
    if ( a[i].msec != b[i].msec || a[i].frequency != b[i].frequency)
      return 0;
  }

  return count;
}




/****************************************************************************/
/* Mocked verification functions */

void *VbExGetMusicPtr(void) {
  return use_hdr;
}

uint32_t VbExMaxMusicSize(void) {
  return use_size;
}


/****************************************************************************/

static void VbAudioTest(void) {
  VbAudioContext* a = 0;

  /* default is okay */
  ResetMocks();
  use_hdr = 0;
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes ==  default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( default )");
  VbAudioClose(a);

  /* short is okay */
  ResetMocks();
  use_hdr = 0;
  gbb.flags = 0x00000001;
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == short_notes_ &&
            a->note_count == short_count_,
            "VbAudioTest( short )");
  VbAudioClose(a);

  /* good custom is okay */
  ResetMocks();
  a = VbAudioOpen(&cparams);
  TEST_TRUE(NotesMatch(a->music_notes, good_notes, good_header.count) &&
            a->note_count == good_header.count,
            "VbAudioTest( custom good )");
  VbAudioClose(a);

  /* good custom is rejected when short flag is set */
  ResetMocks();
  gbb.flags = 0x00000001;
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == short_notes_ &&
            a->note_count == short_count_,
            "VbAudioTest( short has priority )");
  VbAudioClose(a);

  /* too short gets extended */
  ResetMocks();
  use_hdr->count--;
  FixChecksum(use_hdr);
  a = VbAudioOpen(&cparams);
  TEST_TRUE(NotesMatch(a->music_notes, use_notes, use_hdr->count) &&
            a->note_count == use_hdr->count + 1 &&
            a->music_notes[use_hdr->count].msec == 28700 &&
            a->music_notes[use_hdr->count].frequency == 0,
            "VbAudioTest( too short )");
  VbAudioClose(a);

  /* too quiet is rejected */
  ResetMocks();
  use_notes[6].msec = 10;
  FixChecksum(use_hdr);
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( too quiet )");
  VbAudioClose(a);

  /* inaudible is rejected */
  ResetMocks();
  use_notes[0].frequency = 99;
  use_notes[2].frequency = 2001;
  FixChecksum(use_hdr);
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( inaudible )");
  VbAudioClose(a);

  /* bad signature is rejected */
  ResetMocks();
  use_hdr->sig[0] = 'C';
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( bad signature )");
  VbAudioClose(a);

  /* count == 0 is rejected */
  ResetMocks();
  use_hdr->count = 0;
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( count == 0 )");
  VbAudioClose(a);

  /* too big is rejected */
  ResetMocks();
  use_hdr->count = 999;
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( count too big )");
  VbAudioClose(a);

  /* bad checksum is rejected */
  ResetMocks();
  use_hdr->checksum++;
  a = VbAudioOpen(&cparams);
  TEST_TRUE(a->music_notes == default_notes_ &&
            a->note_count == default_count_,
            "VbAudioTest( count too big )");
  VbAudioClose(a);
}


int main(int argc, char* argv[]) {
  int error_code = 0;

  VbAudioTest();

  if (!gTestSuccess)
    error_code = 255;

  if (vboot_api_stub_check_memory())
    error_code = 255;

  return error_code;
}
