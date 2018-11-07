/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifndef SRC_FILESYSTEM_FS_H_
#define SRC_FILESYSTEM_FS_H_

#include "common/inc/global/post_frame.h"

extern Queue_Handle fsRxMsgQueue;
extern Queue_Handle fsTxMsgQueue;
extern Semaphore_Handle semFilesysMsg;

int fileSize(const char *path);
void fs_init(UArg arg0, UArg arg1);
bool fileRead(const char *path, UChar *buf, uint32_t size);
bool fileWrite(const char *path, uint8_t *pMsg, uint32_t size);

#endif /* SRC_FILESYSTEM_FS_H_ */
