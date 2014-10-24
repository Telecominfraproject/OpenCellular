/* Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_UTILITY_CGPT_DRIVE_H_
#define VBOOT_REFERENCE_UTILITY_CGPT_DRIVE_H_

struct drive;
typedef off_t (*DriveSeekFunc)(struct drive*, off_t offset, int whence);
typedef ssize_t (*DriveReadFunc)(struct drive*, void* buf, size_t count);
typedef ssize_t (*DriveWriteFunc)(struct drive*, const void* buf, size_t count);
typedef int (*DriveCloseFunc)(struct drive*);
typedef int (*DriveSyncFunc)(struct drive*);

off_t FileSeek(struct drive* drive, off_t offset, int whence);
ssize_t FileRead(struct drive* drive, void* buf, size_t count);
ssize_t FileWrite(struct drive* drive, const void* buf, size_t count);
int FileSync(struct drive* drive);
int FileClose(struct drive* drive);

int FlashInit(struct drive* drive);
off_t FlashSeek(struct drive* drive, off_t offset, int whence);
ssize_t FlashRead(struct drive* drive, void* buf, size_t count);
ssize_t FlashWrite(struct drive* drive, const void* buf, size_t count);
int FlashSync(struct drive* drive);
int FlashClose(struct drive* drive);

#endif  // VBOOT_REFERENCE_UTILITY_CGPT_DRIVE_H_
