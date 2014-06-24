/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_MTDLIB_UNUSED_H_
#define VBOOT_REFERENCE_MTDLIB_UNUSED_H_

int MtdIsKernelEntry(const MtdDiskPartition *e);
void MtdModified(MtdData *mtd);
int MtdIsPartitionValid(const MtdDiskPartition *part);
int MtdCheckParameters(MtdData *disk);
int MtdCheckEntries(MtdDiskPartition *entries, MtdDiskLayout *h);
int MtdSanityCheck(MtdData *disk);
int MtdInit(MtdData *mtd);
int MtdNextKernelEntry(MtdData *mtd, uint64_t *start_sector, uint64_t *size);
int MtdUpdateKernelEntry(MtdData *mtd, uint32_t update_type);

#endif	/* VBOOT_REFERENCE_MTDLIB_UNUSED_H_ */
