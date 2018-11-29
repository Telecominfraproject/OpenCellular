/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * This file acts as wrapper for little filesystem, contains filesystem
 * initialization, block read, block write, block erase as a main functions
 * moreover provides API's like fileRead, fileWrite for external application to
 * read and write data to at45db flash memory by using SPI interface.
 */

#include "Board.h"
#include "common/inc/global/Framework.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/common/bigbrother.h"
#include "inc/common/global_header.h"
#include "inc/devices/at45db.h"
#include "inc/global/OC_CONNECT1.h"
#include "inc/utils/util.h"
#include "src/filesystem/fs_wrapper.h"
#include "src/filesystem/lfs.h"
#include <string.h>
#include <stdlib.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Task.h>

#define BLOCK_SIZE 256
#define BLOCK_COUNT 32768
#define FRAME_SIZE 64
#define LOOK_AHEAD 256
#define PAGE_SIZE 256
#define READ_SIZE 256
#define WRITE_SIZE 256

lfs_t lfs;
lfs_file_t file;

/*****************************************************************************
 **    FUNCTION NAME   : block_device_read
 **
 **    DESCRIPTION     : It is called by filesystem to read block device
 **
 **    ARGUMENTS       : context for device configuration, block or page number,
 **
 **                      block or page offset, data buffer, size of data to read
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
                      lfs_off_t off, void *buffer, lfs_size_t size)
{
    if (at45db_data_read(cfg->context, buffer, size, off, block) != RETURN_OK) {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : block_device_write
 **
 **    DESCRIPTION     : it is called by filesystem to write block device
 **
 **    ARGUMENTS       : context for device configuration, block or page number,
 **
 **                      block or page offset, data buffer, size of data to
 *write
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
int block_device_write(const struct lfs_config *cfg, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size)
{
    if (at45db_data_write(cfg->context, buffer, size, off, block) !=
        RETURN_OK) {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : block_device_erase
 **
 **    DESCRIPTION     : It is called by filesystem to erase block device
 **
 **    ARGUMENTS       : context for device configuration, block or page number,
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
int block_device_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    if (at45db_erasePage(cfg->context, block) != RETURN_OK) {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : block_device_sync
 **
 **    DESCRIPTION     : It is  called by filesystem to sync with block device
 **
 **    ARGUMENTS       : context for device configuration
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
int block_device_sync(const struct lfs_config *cfg)
{
    if (at45db_readStatusRegister(cfg->context) != RETURN_OK) {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : fileSize
 **
 **    DESCRIPTION     : Returns size of saved file
 **
 **    ARGUMENTS       : Path or file name
 **
 **    RETURN TYPE     : file size
 **
 *****************************************************************************/
int fileSize(const char *path)
{
    uint32_t fileSize = 0;

    if (lfs_file_open(&lfs, &file, path, LFS_O_RDONLY) == LFS_ERR_OK) {
        LOGGER_DEBUG("FS:: File open successfully \n");
    }
    fileSize = lfs_file_size(&lfs, &file);
    lfs_file_close(&lfs, &file);

    return fileSize;
}

/*****************************************************************************
 **    FUNCTION NAME   : fileWrite
 **
 **    DESCRIPTION     : It write data to specified file
 **
 **    ARGUMENTS       : Path or file name, pointer to data, data length or size
 **
 **    RETURN TYPE     : true or flase
 **
 *****************************************************************************/
bool fileWrite(const char *path, uint8_t *pMsg, uint32_t size)
{
    if (lfs_file_open(&lfs, &file, path,
                      LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND) == LFS_ERR_OK) {
        LOGGER_DEBUG("FS:: File open successfully \n");
    }
    if (lfs_file_write(&lfs, &file, pMsg, size) == size) {
        LOGGER_DEBUG("FS:: File written successfully \n");
    }
    if (lfs_file_close(&lfs, &file) == LFS_ERR_OK) {
        LOGGER_DEBUG("FS:: File closed successfully \n");
    }

    return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : fileRead
 **
 **    DESCRIPTION     : It reads data from specified file
 **
 **    ARGUMENTS       : Path or file name, pointer to data, data length or size
 **
 **    RETURN TYPE     : true or flase
 **
 *****************************************************************************/
bool fileRead(const char *path, UChar *buf, uint32_t size)
{
    if (lfs_file_open(&lfs, &file, path, LFS_O_RDONLY) == LFS_ERR_OK) {
        LOGGER_DEBUG("FS:: File open successfully \n");
    }
    if (lfs_file_read(&lfs, &file, buf, size) == size) {
        LOGGER_DEBUG("FS:: File read successfully \n");
    }
    if (lfs_file_close(&lfs, &file) == LFS_ERR_OK) {
        LOGGER_DEBUG("FS:: File closed successfully \n");
    }

    return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : fsMsgHandler
 **
 **    DESCRIPTION     : It is called when data to be written
 **
 **    ARGUMENTS       : data pointer
 **
 **    RETURN TYPE     : true or flase
 **
 *****************************************************************************/
static bool fsMsgHandler(OCMPMessageFrame *pMsg)
{
    char fileName[] = "logs";

    fileWrite(fileName, (uint8_t *)pMsg, FRAME_SIZE);

    return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : fs_init
 **
 **    DESCRIPTION     : It initializes filesystem by mounting device
 **
 **    ARGUMENTS       : arg0 for SPI device configuration, arg1 for return
 **
 **    RETURN TYPE     : true or flase
 **
 *****************************************************************************/
void fs_init(UArg arg0, UArg arg1)
{
    /*configuration of the filesystem is provided by this struct */
    const struct lfs_config cfg = {
        .context = (void *)arg0,
        .read = block_device_read,
        .prog = block_device_write,
        .erase = block_device_erase,
        .sync = block_device_sync,
        .read_size = READ_SIZE,
        .prog_size = WRITE_SIZE,
        .block_size = BLOCK_SIZE,
        .block_count = BLOCK_COUNT,
        .lookahead = LOOK_AHEAD,
    };
    int err = lfs_mount(&lfs, &cfg);

    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    if (!err) {
        LOGGER_DEBUG("FS:: Filesystem mounted successfully \n");
    }

    while (true) {
        if (Semaphore_pend(semFilesysMsg, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(fsTxMsgQueue)) {
                OCMPMessageFrame *pMsg =
                    (OCMPMessageFrame *)Util_dequeueMsg(fsTxMsgQueue);
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
