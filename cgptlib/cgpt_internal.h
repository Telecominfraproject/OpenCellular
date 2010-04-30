/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_CGPT_INTERNAL_H_
#define VBOOT_REFERENCE_CGPT_INTERNAL_H_

#include <stdint.h>
#include "cgpt.h"

/* Internal use only.
 * Don't use them unless you know what you are doing. */
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
int OverlappedEntries(GptEntry *entries, uint32_t number_of_entries);
uint32_t CheckOverlappedPartition(GptData *gpt);
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries);
uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers);
typedef struct {
  uint64_t starting;
  uint64_t ending;
} pair_t;

#endif /* VBOOT_REFERENCE_CGPT_INTERNAL_H_ */
