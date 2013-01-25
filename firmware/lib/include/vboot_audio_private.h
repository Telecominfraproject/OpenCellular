/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Private declarations for vboot_audio.c. Defined here for easier testing.
 */

#ifndef VBOOT_REFERENCE_VBOOT_AUDIO_PRIVATE_H_
#define VBOOT_REFERENCE_VBOOT_AUDIO_PRIVATE_H_

#include "vboot_api.h"
#include "vboot_audio.h"

typedef struct VbDevMusicNote {
	uint16_t msec;
	uint16_t frequency;
} __attribute__((packed)) VbDevMusicNote;

typedef struct VbDevMusic {
	uint8_t sig[4];			/* "$SND" */
	uint32_t checksum;              /* crc32 over count & all notes */
	uint32_t count;                 /* number of notes */
	VbDevMusicNote notes[1];        /* gcc allows [0], MSVC doesn't */
	/* more VbDevMusicNotes follow immediately */
} __attribute__((packed)) VbDevMusic;

struct VbAudioContext {
	/* note tracking */
	VbDevMusicNote *music_notes;
	uint32_t note_count;
	uint32_t next_note;

	/* implementation flags */
	int background_beep;
	int free_notes_when_done;

	/* sound tracking */
	uint16_t current_frequency;
	uint64_t play_until;
	uint64_t last_time;
};

#ifdef FOR_TEST
#define CUSTOM_MUSIC
#endif

#ifdef CUSTOM_MUSIC
void *VbExGetMusicPtr(void);
uint32_t VbExMaxMusicSize(void);
#define CUSTOM_MUSIC_NOTES VbExGetMusicPtr()
#define CUSTOM_MUSIC_MAXSIZE VbExMaxMusicSize()
#else
#define CUSTOM_MUSIC_NOTES 0
#define CUSTOM_MUSIC_MAXSIZE 0
#endif

#endif /* VBOOT_REFERENCE_VBOOT_AUDIO_PRIVATE_H_ */
