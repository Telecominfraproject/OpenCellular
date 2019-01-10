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

extern Semaphore_Handle semFilesysMsg;
extern Queue_Handle fsRxMsgQueue;
extern Queue_Handle fsTxMsgQueue;

typedef struct at45db_Dev {
    //const at45db_Cfg cfg;
    //at45db_Obj obj;
} AT45DB_Dev;
ePostCode at45db_probe(AT45DB_Dev *dev, POSTData *postData);

#endif /* SRC_FILESYSTEM_FS_H_ */
