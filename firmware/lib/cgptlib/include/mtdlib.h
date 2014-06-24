/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_MTDLIB_H_
#define VBOOT_REFERENCE_MTDLIB_H_

#include "cgptlib.h"
#include "sysincludes.h"


#define MTD_DRIVE_SIGNATURE "CrOSPart" /* This must be exactly 8 chars */

/*
 * Bit definitions and masks for MTD attributes.
 *
 * 13-16 -- partition number
 *  9-12 -- partition type
 *    8  -- success
 *  7-4  -- tries
 *  3-0  -- priority
 */
#define MTD_ATTRIBUTE_PRIORITY_OFFSET (0)
#define MTD_ATTRIBUTE_MAX_PRIORITY (15UL)
#define MTD_ATTRIBUTE_PRIORITY_MASK (MTD_ATTRIBUTE_MAX_PRIORITY << \
                                     MTD_ATTRIBUTE_PRIORITY_OFFSET)

#define MTD_ATTRIBUTE_TRIES_OFFSET (4)
#define MTD_ATTRIBUTE_MAX_TRIES (15UL)
#define MTD_ATTRIBUTE_TRIES_MASK (MTD_ATTRIBUTE_MAX_TRIES << \
                                  MTD_ATTRIBUTE_TRIES_OFFSET)

#define MTD_ATTRIBUTE_SUCCESSFUL_OFFSET (8)
#define MTD_ATTRIBUTE_MAX_SUCCESSFUL (1UL)
#define MTD_ATTRIBUTE_SUCCESSFUL_MASK (MTD_ATTRIBUTE_MAX_SUCCESSFUL << \
                                       MTD_ATTRIBUTE_SUCCESSFUL_OFFSET)

#define MTD_ATTRIBUTE_TYPE_OFFSET (9)
#define MTD_ATTRIBUTE_MAX_TYPE (15UL)
#define MTD_ATTRIBUTE_TYPE_MASK (MTD_ATTRIBUTE_MAX_TYPE << \
                                 MTD_ATTRIBUTE_TYPE_OFFSET)

#define MTD_ATTRIBUTE_NUMBER_OFFSET (13)
#define MTD_ATTRIBUTE_MAX_NUMBER (15UL)
#define MTD_ATTRIBUTE_NUMBER_MASK (MTD_ATTRIBUTE_MAX_NUMBER << \
                                   MTD_ATTRIBUTE_NUMBER_OFFSET)


#define MTD_PARTITION_TYPE_UNUSED             0
#define MTD_PARTITION_TYPE_CHROMEOS_KERNEL    1
#define MTD_PARTITION_TYPE_CHROMEOS_FIRMWARE  2
#define MTD_PARTITION_TYPE_CHROMEOS_ROOTFS    3
#define MTD_PARTITION_TYPE_CHROMEOS_RESERVED  4
#define MTD_PARTITION_TYPE_CHROMEOS_FLAGSTORE 5
#define MTD_PARTITION_TYPE_LINUX_DATA         6
#define MTD_PARTITION_TYPE_EFI                7
#define MTD_PARTITION_TYPE_OTHER              8

/* This is mostly arbitrary at the moment, but gives a little room to expand. */
#define MTD_MAX_PARTITIONS 16



typedef struct {
  uint64_t starting_offset;
  uint64_t ending_offset;
  uint32_t flags;

  /* 28 characters is a balance between GPT parity and size constraints, at
   * current sizes this table occupies 10% of the FTS data store.
   */
  char label[28];
} __attribute__((packed)) MtdDiskPartition;

typedef struct {
  unsigned char signature[8];
  /* For compatibility, this is only ever the CRC of the first
   * MTD_DRIVE_V1_SIZE bytes. Further extensions must include their own CRCs,
   * so older FW can boot newer layouts if we expand in the future.
   */
  uint32_t crc32;
  uint32_t size;
  uint64_t first_offset;
  uint64_t last_offset;
  MtdDiskPartition partitions[MTD_MAX_PARTITIONS];
} __attribute__((packed)) MtdDiskLayout;

#define MTD_DRIVE_V1_SIZE (32 + 16*48)

#define MTDENTRY_EXPECTED_SIZE (48)
#define MTDLAYOUT_EXPECTED_SIZE (32 + 16 * MTDENTRY_EXPECTED_SIZE)


typedef struct {
  /* Specifies the flash geometry, in erase blocks & write pages */
  uint32_t flash_block_bytes;
  uint32_t flash_page_bytes;

  /* Location, in blocks, of FTS partition */
  uint32_t fts_block_offset;
  /* Size, in blocks, of FTS partition */
  uint32_t fts_block_size;

  /* Size of a LBA sector, in bytes */
  uint32_t sector_bytes;
  /* Size of drive in LBA sectors, in sectors */
  uint64_t drive_sectors;

  /*
   * The current chromeos kernel index in partition table.  -1 means not
   * found on drive.
   */
  int current_kernel;
  int current_priority;

  /* If set, the flags partition has been modified and needs to be flushed */
  int modified;

  /* Internal variables */
  MtdDiskLayout primary;
} MtdData;


/* APIs are documented in cgptlib.h & cgptlib_internal.h */
int MtdGetEntryPriority(const MtdDiskPartition *e);
int MtdGetEntryTries(const MtdDiskPartition *e);
int MtdGetEntrySuccessful(const MtdDiskPartition *e);
int MtdGetEntryType(const MtdDiskPartition *e);
void MtdSetEntrySuccessful(MtdDiskPartition *e, int successful) ;
void MtdSetEntryPriority(MtdDiskPartition *e, int priority);
void MtdSetEntryTries(MtdDiskPartition *e, int tries);
void MtdSetEntryType(MtdDiskPartition *e, int type);

void MtdGetPartitionSize(const MtdDiskPartition *e,
                         uint64_t *start, uint64_t *end, uint64_t *size);

void MtdGetPartitionSizeInSectors(const MtdDiskPartition *e, uint64_t *start,
                                  uint64_t *end, uint64_t *size);

int MtdGptInit(MtdData *mtd);
uint32_t MtdHeaderCrc(MtdDiskLayout *h);

#endif

