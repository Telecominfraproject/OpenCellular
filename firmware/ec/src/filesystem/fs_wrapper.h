/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifndef SRC_FILESYSTEM_FS_WRAPPER_H_
#define SRC_FILESYSTEM_FS_WRAPPER_H_

#include "common/inc/global/post_frame.h"
#include "common/inc/global/ocmp_frame.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

#define FRAME_SIZE 64
#define FS_OCMP_MSGTYPE_POS 13
#define FS_OCMP_SUBSYSTEM_POS 11
#define FS_STR_SIZE 50
#define LAST_MSG 1
#define LAST_MSG_FLAG 0
#define MAX_ALERT_FILE_SIZE 512
#define NEXT_MSG_FLAG_POS 17
#define NO_OF_ALERT_FILES 8
#define READ_FLAG 0
#define WRITE_FLAG 1
#define CONSOLE_LOG 2

Semaphore_Handle semFilesysMsg;
Semaphore_Struct semFSstruct;

Semaphore_Handle semFSreadMsg;
Semaphore_Struct semFSreadStruct;

Semaphore_Handle semFSwriteMsg;
Semaphore_Struct semFSwriteStruct;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static Queue_Struct fsRxMsg;
static Queue_Struct fsTxMsg;
#pragma GCC diagnostic pop

Queue_Handle fsRxMsgQueue;
Queue_Handle fsTxMsgQueue;

typedef struct FILESystemStruct {
    char *fileName;
    uint32_t frameSize;
    uint8_t noOfFiles;
    void *pMsg;
    uint32_t maxFileSize;
    uint8_t operation;
    uint32_t fileSize;
} FILESystemStruct;

int fs_wrapper_get_fileSize(const char *path);
bool fs_wrapper_data_read(FILESystemStruct *fileSysStruct);
void fs_wrapper_flashMemory_read(OCMPSubsystem subsystem, const char *path,
                                 uint32_t file_size, uint8_t fileIndex);
void fs_wrapper_fileSystem_init(UArg arg0, UArg arg1);
bool fs_wrapper_file_read(const char *fileName, uint8_t *buf, uint32_t size);

#endif /* SRC_FILESYSTEM_FS_WRAPPER_H_ */
