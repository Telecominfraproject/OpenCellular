/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CGPTLIB_INTERNAL_H_
#define VBOOT_REFERENCE_CGPTLIB_INTERNAL_H_

#include <stdint.h>
#include "cgptlib.h"

int CheckParameters(GptData *gpt);
uint32_t CheckHeaderSignature(GptData *gpt);
uint32_t CheckRevision(GptData *gpt);
uint32_t CheckSize(GptData *gpt);
uint32_t CheckReservedFields(GptData *gpt);
uint32_t CheckMyLba(GptData *gpt);
uint32_t CheckSizeOfPartitionEntry(GptData *gpt);
uint32_t CheckNumberOfEntries(GptData *gpt);
uint32_t CheckEntriesLba(GptData *gpt);
uint32_t CheckValidUsableLbas(GptData *gpt);
uint32_t CheckHeaderCrc(GptData *gpt);
uint32_t CheckEntriesCrc(GptData *gpt);
uint32_t CheckValidEntries(GptData *gpt);
typedef struct {
  uint64_t starting;
  uint64_t ending;
} pair_t;
int OverlappedEntries(GptEntry *entries, uint32_t number_of_entries);
uint32_t CheckOverlappedPartition(GptData *gpt);
int IsSynonymous(const GptHeader* a, const GptHeader* b);
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries);
uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers);
void UpdateCrc(GptData *gpt);
int GptSanityCheck(GptData *gpt);
void GptRepair(GptData *gpt);

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

/* If gpt->current_kernel is this value, means either:
 *   1. an initial value before scanning GPT entries,
 *   2. after scanning, no any valid kernel is found.
 */
#define CGPT_KERNEL_ENTRY_NOT_FOUND (-1)

/* Bit definitions and masks for GPT attributes.
 *
 *     63  -- do not automounting
 *     62  -- hidden
 *     60  -- read-only
 *      :
 *     57  -- bad kernel entry
 *     56  -- success
 *  55,52  -- tries
 *  51,48  -- priority
 *      0  -- system partition
 */
#define CGPT_ATTRIBUTE_BAD_OFFSET 57
#define CGPT_ATTRIBUTE_MAX_BAD (1ULL)
#define CGPT_ATTRIBUTE_BAD_MASK (CGPT_ATTRIBUTE_MAX_BAD << \
                                 CGPT_ATTRIBUTE_BAD_OFFSET)

#define CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET 56
#define CGPT_ATTRIBUTE_MAX_SUCCESSFUL (1ULL)
#define CGPT_ATTRIBUTE_SUCCESSFUL_MASK (CGPT_ATTRIBUTE_MAX_SUCCESSFUL << \
                                     CGPT_ATTRIBUTE_SUCCESSFUL_OFFSET)

#define CGPT_ATTRIBUTE_TRIES_OFFSET 52
#define CGPT_ATTRIBUTE_MAX_TRIES (15ULL)
#define CGPT_ATTRIBUTE_TRIES_MASK (CGPT_ATTRIBUTE_MAX_TRIES << \
                                   CGPT_ATTRIBUTE_TRIES_OFFSET)

#define CGPT_ATTRIBUTE_PRIORITY_OFFSET 48
#define CGPT_ATTRIBUTE_MAX_PRIORITY (15ULL)
#define CGPT_ATTRIBUTE_PRIORITY_MASK (CGPT_ATTRIBUTE_MAX_PRIORITY << \
                                      CGPT_ATTRIBUTE_PRIORITY_OFFSET)

#endif /* VBOOT_REFERENCE_CGPTLIB_INTERNAL_H_ */
