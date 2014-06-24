/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "mtdlib.h"

#include "cgptlib.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "utility.h"
#include "vboot_api.h"

const int kSectorShift = 9; /* 512 bytes / sector. */

int MtdGetEntryPriority(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_PRIORITY_MASK) >>
          MTD_ATTRIBUTE_PRIORITY_OFFSET);
}

int MtdGetEntryTries(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_TRIES_MASK) >>
          MTD_ATTRIBUTE_TRIES_OFFSET);
}

int MtdGetEntrySuccessful(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_SUCCESSFUL_MASK) >>
          MTD_ATTRIBUTE_SUCCESSFUL_OFFSET);
}

int MtdGetEntryType(const MtdDiskPartition *e) {
  return ((e->flags & MTD_ATTRIBUTE_TYPE_MASK) >> MTD_ATTRIBUTE_TYPE_OFFSET);
}

static void SetBitfield(MtdDiskPartition *e,
                        uint32_t offset, uint32_t mask, uint32_t v) {
  e->flags = (e->flags & ~mask) | ((v << offset) & mask);
}
void MtdSetEntrySuccessful(MtdDiskPartition *e, int successful) {
  SetBitfield(e, MTD_ATTRIBUTE_SUCCESSFUL_OFFSET,
              MTD_ATTRIBUTE_SUCCESSFUL_MASK, successful);
}
void MtdSetEntryPriority(MtdDiskPartition *e, int priority) {
  SetBitfield(e, MTD_ATTRIBUTE_PRIORITY_OFFSET, MTD_ATTRIBUTE_PRIORITY_MASK,
              priority);
}
void MtdSetEntryTries(MtdDiskPartition *e, int tries) {
  SetBitfield(e, MTD_ATTRIBUTE_TRIES_OFFSET, MTD_ATTRIBUTE_TRIES_MASK, tries);
}
void MtdSetEntryType(MtdDiskPartition *e, int type) {
  SetBitfield(e, MTD_ATTRIBUTE_TYPE_OFFSET, MTD_ATTRIBUTE_TYPE_MASK, type);
}

uint32_t MtdHeaderCrc(MtdDiskLayout *h) {
  uint32_t crc32, original_crc32;

  /* Original CRC is calculated with the CRC field 0. */
  original_crc32 = h->crc32;
  h->crc32 = 0;
  crc32 = Crc32((const uint8_t *)h, h->size);
  h->crc32 = original_crc32;

  return crc32;
}

void MtdGetPartitionSize(const MtdDiskPartition *e,
                         uint64_t *start, uint64_t *end, uint64_t *size) {
  uint64_t start_tmp, end_tmp;
  if (!start)
    start = &start_tmp;
  if (!end)
    end = &end_tmp;

  Memcpy(start, &e->starting_offset, sizeof(e->starting_offset));
  Memcpy(end, &e->ending_offset, sizeof(e->ending_offset));
  if (size) {
    *size = *end - *start + 1;
  }
}

void MtdGetPartitionSizeInSectors(const MtdDiskPartition *e, uint64_t *start,
                                  uint64_t *end, uint64_t *size) {
  MtdGetPartitionSize(e, start, end, size);
  if (start)
    *start >>= kSectorShift;
  if (end)
    *end >>= kSectorShift;
  if (size)
    *size >>= kSectorShift;
}



