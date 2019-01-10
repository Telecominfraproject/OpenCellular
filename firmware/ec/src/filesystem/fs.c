/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*
*/

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>

#include <string.h>
#include <stdlib.h>

#include "Board.h"
#include <inc/global/OC_CONNECT1.h>
#include "inc/common/global_header.h"
#include "common/inc/global/Framework.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/utils/util.h"

#include "src/filesystem/fs.h"
#include "src/filesystem/lfs.h"
#include "inc/devices/AT45DB.h"
#include "inc/common/bigbrother.h"

#define AT45DB_MANUFACT_ID 	0X1F
#define	AT45DB_DEV_ID		0x0028

static SPI_Params spiParams;
static SPI_Handle spiHandle;
static AT45DB_Params at45dbParams;
static AT45DB_Object obj;
static AT45DB_Handle at45dbHandle;
static AT45DB_Transaction at45dbTransaction;

#define OCFS_TASK_PRIORITY            5
#define OCFS_TASK_STACK_SIZE          4096

Task_Struct ocFSTask;
Char ocFSTaskStack[OCFS_TASK_STACK_SIZE];

Semaphore_Handle semFilesysMsg;

Semaphore_Struct semFSstruct;

static Queue_Struct fsRxMsg;
static Queue_Struct fsTxMsg;

Queue_Handle fsRxMsgQueue;
Queue_Handle fsTxMsgQueue;

lfs_t lfs;
lfs_file_t file;

bool at45db_init(void* driver, void *returnVal) {

    SPI_Params_init(&spiParams);
    spiHandle = SPI_open(OC_CONNECT1_SPI1, &spiParams);
    if (spiHandle == NULL) {
    	LOGGER_DEBUG("SPI_open failed\n");
    	return false;
    }
    /* Construct the AT45DB handle */
    AT45DB_Params_init(&at45dbParams);
    at45dbHandle = AT45DB_construct(&obj, spiHandle, Board_EC_FLASH, &at45dbParams);
    if (at45dbHandle == NULL) {
    	LOGGER_DEBUG("AT45DB_construct failed\n");
    	return false;
    }
    return true;
}

int flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    at45dbTransaction.data = buffer;
    at45dbTransaction.data_size = size;
    at45dbTransaction.byte      = off;

    AT45DB_read(at45dbHandle, &at45dbTransaction, block);

    return LFS_ERR_OK;
}

int flash_write(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
	at45dbTransaction.data      = (unsigned char*)buffer;
	at45dbTransaction.data_size = size;
	at45dbTransaction.byte      = off;

	AT45DB_write(at45dbHandle, &at45dbTransaction, block);

	return LFS_ERR_OK;
}

int flash_erase(const struct lfs_config *c, lfs_block_t block)
{
	AT45DB_erasePage(at45dbHandle, block);

	return LFS_ERR_OK;
}

int flash_sync(const struct lfs_config *c)
{
	AT45DB_readStatusRegister(at45dbHandle);

	return LFS_ERR_OK;
}

/*configuration of the filesystem is provided by this struct */
const struct lfs_config cfg = {
    .read  = flash_read,
    .prog  = flash_write,
    .erase = flash_erase,
	.sync  = flash_sync,
    .read_size = 256,
    .prog_size = 256,
    .block_size = 256,
    .block_count = 32768,
    .lookahead = 256,
};

ePostCode at45db_probe(AT45DB_Dev *dev, POSTData *postData)
{
    uint32_t value = 0x00000000;
    uint16_t devId = 0x0000;
    uint8_t manufactId = 0x00;
    if (flash_info(at45dbHandle, &value) != true) {
        return POST_DEV_MISSING;
    }

    devId = (value >> 8) & 0xFFFF;

    if(devId != AT45DB_DEV_ID)
    {
    	return POST_DEV_ID_MISMATCH;
    }

    manufactId = value & 0xFF;

    if(manufactId != AT45DB_MANUFACT_ID)
    {
    	return POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, NULL, NULL,NULL, devId);
    return POST_DEV_FOUND;
}

uint32_t fileSize(const char *path)
{
	uint32_t fileSize = 0;
	lfs_file_open(&lfs, &file, path, LFS_O_RDONLY );
	fileSize = lfs_file_size(&lfs, &file);
	lfs_file_close(&lfs, &file);
	return fileSize;
}
static bool fileWrite(const char *path, uint8_t *pMsg, uint32_t size )
{
	lfs_file_open(&lfs, &file, path, LFS_O_RDWR | LFS_O_CREAT | LFS_O_APPEND);
    lfs_file_write(&lfs, &file, pMsg, size);
    lfs_file_close(&lfs, &file);
    return true;
}
void fileRead(const char *path, UChar *buf, uint32_t size)
{
    lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    lfs_file_read(&lfs, &file, buf, size);
    lfs_file_close(&lfs, &file);
}
static bool fsMsgHandler(OCMPMessageFrame *pMsg)
{
	char fileName[] = "logs";
	fileWrite(fileName, pMsg, 64);
	return true;
}
static void fs_init(UArg arg0, UArg arg1)
{
	/* mount the filesystem */
    int err = lfs_mount(&lfs, &cfg);

    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    while (true) {
        if (Semaphore_pend(semFilesysMsg, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(fsTxMsgQueue)) {
            	OCMPMessageFrame *pMsg = (OCMPMessageFrame *)Util_dequeueMsg(fsTxMsgQueue);
                if (pMsg != NULL) {
                    if (!fsMsgHandler(pMsg)) {
                        LOGGER_ERROR("ERROR:: Unable to route Alert message\n");
                        free(pMsg);
                    }
                }
            }
        }
    }
}

bool fs_createtask(void* driver, void *returnValue)
{
    Semaphore_construct(&semFSstruct, 0, NULL);
    semFilesysMsg = Semaphore_handle(&semFSstruct);
    if (!semFilesysMsg) {
        LOGGER_DEBUG("FS:ERROR:: Failed in Creating Semaphore");
        return false;
    }
    /* Create Message Queue for RX Messages */
    fsTxMsgQueue = Util_constructQueue(&fsTxMsg);
    if (!fsTxMsgQueue) {
        LOGGER_ERROR("FS:ERROR:: Failed in Constructing Message Queue for");
        return false;
    }
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = OCFS_TASK_STACK_SIZE;
    taskParams.stack = &ocFSTaskStack;
    taskParams.instance->name = "FS_TASK";
    taskParams.priority = OCFS_TASK_PRIORITY;
    Task_construct(&ocFSTask,fs_init, &taskParams,NULL);
    LOGGER_DEBUG("FS:INFO:: Creating filesystem task function.\n");
    return true;
}

