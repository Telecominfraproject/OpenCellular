/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*
*/

#include "Board.h"
#include "common/inc/global/Framework.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/utils/util.h"
#include <inc/global/OC_CONNECT1.h>
#include "inc/common/global_header.h"
#include "inc/devices/at45db.h"
#include "inc/common/bigbrother.h"
#include "src/filesystem/fs_wrapper.h"
#include <string.h>
#include <stdlib.h>
#include "src/filesystem/lfs.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

#define FRAME_SIZE                  64
#define READ_SIZE                   256
#define WRITE_SIZE                  256
#define PAGE_SIZE                   256
#define BLOCK_SIZE                  256
#define BLOCK_COUNT                 32768
#define LOOK_AHEAD                  256

static Queue_Struct fsRxMsg;
static Queue_Struct fsTxMsg;

lfs_t lfs;
lfs_file_t file;

int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
                      lfs_off_t off, void *buffer, lfs_size_t size)
{
    if(at45db_data_read(cfg->context, buffer, size, off, block) != RETURN_OK){
       return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int block_device_write(const struct lfs_config *cfg, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size)
{
    if(at45db_data_write(cfg->context, buffer, size, off, block) != RETURN_OK){
       return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int block_device_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    if(at45db_erasePage(cfg->context, block) != RETURN_OK){
       return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int block_device_sync(const struct lfs_config *cfg)
{
    if(at45db_readStatusRegister(cfg->context) != RETURN_OK){
       return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

int fileSize(const char *path)
{
    uint32_t fileSize = 0;

    if(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY) == LFS_ERR_OK){
       LOGGER_DEBUG("FS:: File open successfully \n");
    }
    fileSize = lfs_file_size(&lfs, &file);
    lfs_file_close(&lfs, &file);

    return fileSize;
}
bool fileWrite(const char *path, uint8_t *pMsg, uint32_t size )
{
    if(lfs_file_open(&lfs, &file, path, LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND) == LFS_ERR_OK) {
       LOGGER_DEBUG("FS:: File open successfully \n");
    }
    if(lfs_file_write(&lfs, &file, pMsg, size) == size) {
       LOGGER_DEBUG("FS:: File written successfully \n");
    }
    if(lfs_file_close(&lfs, &file) == LFS_ERR_OK){
    LOGGER_DEBUG("FS:: File closed successfully \n");
    }

    return true;
}
bool fileRead(const char *path, UChar *buf, uint32_t size)
{
    if(lfs_file_open(&lfs, &file, path, LFS_O_RDONLY) == LFS_ERR_OK) {
       LOGGER_DEBUG("FS:: File open successfully \n");
    }
    if(lfs_file_read(&lfs, &file, buf, size) == size) {
       LOGGER_DEBUG("FS:: File read successfully \n");
    }
    if(lfs_file_close(&lfs, &file) == LFS_ERR_OK) {
       LOGGER_DEBUG("FS:: File closed successfully \n");
    }

    return true;
}
static bool fsMsgHandler(OCMPMessageFrame *pMsg)
{
    char fileName[] = "logs";

    fileWrite(fileName, pMsg, FRAME_SIZE);

    return true;
}
void fs_init(UArg arg0, UArg arg1)
{
    /*configuration of the filesystem is provided by this struct */
    const struct lfs_config cfg = {
        .context = (void*)arg0,
        .read  = block_device_read,
        .prog  = block_device_write,
        .erase = block_device_erase,
        .sync  = block_device_sync,
        .read_size = READ_SIZE,
        .prog_size = WRITE_SIZE,
        .block_size = BLOCK_SIZE,
        .block_count = BLOCK_COUNT,
        .lookahead = LOOK_AHEAD,
    };
    int err = lfs_mount(&lfs, &cfg);

    if (err){
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    if(!err){
       LOGGER_DEBUG("FS:: Filesystem mounted successfully \n");
    }

    while (true) {
        if (Semaphore_pend(semFilesysMsg, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(fsTxMsgQueue)) {
                OCMPMessageFrame *pMsg = (OCMPMessageFrame *)Util_dequeueMsg(fsTxMsgQueue);
                if (pMsg != NULL) {
                    if (!fsMsgHandler(pMsg)) {
                        LOGGER_ERROR("ERROR:: Unable to route message \n");
                        free(pMsg);
                    }
                }
            }
        }
    }
}
