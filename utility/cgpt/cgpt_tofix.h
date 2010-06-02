/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CGPT_TOFIX_H_
#define CGPT_TOFIX_H_

#include <stdint.h>
#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "gpt.h"

/* TODO: This is stuff copied out of cgptlib.  cgptlib doesn't need it anymore,but currently the cgpt tool does. */

const char *GptError(int errno);

int IsSynonymous(const GptHeader* a, const GptHeader* b);
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries);
uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers);
void UpdateCrc(GptData *gpt);
int NonZeroGuid(const Guid *guid);
uint32_t CheckValidEntries(GptData *gpt);
uint32_t CheckOverlappedPartition(GptData *gpt);

GptEntry *GetEntry(GptData *gpt, int secondary, int entry_index);
void SetPriority(GptData *gpt, int secondary, int entry_index, int priority);
int GetPriority(GptData *gpt, int secondary, int entry_index);
void SetBad(GptData *gpt, int secondary, int entry_index, int bad);
int GetBad(GptData *gpt, int secondary, int entry_index);
void SetTries(GptData *gpt, int secondary, int entry_index, int tries);
int GetTries(GptData *gpt, int secondary, int entry_index);
void SetSuccessful(GptData *gpt, int secondary, int entry_index, int success);
int GetSuccessful(GptData *gpt, int secondary, int entry_index);

/* Get number of entries value in primary header */
uint32_t GetNumberOfEntries(const GptData *gpt);


#endif /* CGPT_TOFIX_H_ */
