/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** ============================================================================
 *  @file       AT45DB.h
 *
 *  @brief      AT45DB driver interface
 *
 *  The AT45DB header file should be included in an application as follows:
 *  @code
 *  #include <AT45DB.h>
 *  @endcode
 *
 *  # Operation #
 *  This example module allows an application to read from/write to a AT45DB
 *  device. The full functionality of the device has not be implemented (please
 *  refer to Enhancements below). 
 *
 *  The APIs are thread-safe. Two tasks can write/read to the same device safely.
 *  This is accomplished with a GateMutex in the implementation.
 
 *  ## Creating an instance #
 *  @code
 *  SPI_Params spiParams;
 *  SPI_Handle spiHandle;
 *  AT45DB_Params at45dbParams;
 *  AT45DB_Object obj;
 *  AT45DB_Handle at45dbHandle;
 *  volatile unsigned char ready;
 *
 *  SPI_Params_init(&spiParams);
 *  spiHandle = SPI_open(Board_SPI0, &spiParams);
 *
 *  AT45DB_Params_init(&at45dbParams);
 *  at45dbHandle = AT45DB_create(spiHandle, Board_CS, &at45dbParams);
 *  if (!handle) {
 *      System_printf("AT45DB_create failed");
 *  }
 *  @endcode
 *
 *  # Enhancements #
 *    - Implement Security features
 *    - Utilize SRAM1 also
 *    - Continuous read features
 *    - Power features
 *    - Handle different frequencies for the reads
 *    - Get device id/manufacture information
 *    - validate with 512 page size
 */

#ifndef __AT45DB_H
#define __AT45DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <ti/drivers/SPI.h>
#include <ti/sysbios/gates/GateMutex.h>

/*!
 *  @brief AT45DB Ready Value
 */
#define AT45DB_READY 0x80

/*!
 *  @brief AT45DB Parameters
 *
 *  This is a place-holder structure now since there are no parameters
 *  for the create/construct calls.
 *
 *  @sa         AT45DB_Params_init()
 */
typedef struct AT45DB_Params {
    uint8_t dummy;
} AT45DB_Params;

/*!
 *  @brief AT45DB Transaction Structure
 *
 *  This structure is used to describe the data to read/write.
 */
typedef struct AT45DB_Transaction {
    unsigned char *data;       /*!< data pointer to read or write       */
    uint32_t       data_size;  /*!< size of data to read or write       */
    uint32_t       byte;       /*!< byte offset in memory to read/write */
} AT45DB_Transaction;

/*!
 *  @brief AT45DB Number of Pages constant
 */
#define AT45DB161_NUMBER_OF_PAGES  32768

/*!
 *  @brief AT45DB Page size constant
 */
#define AT45DB161_PAGE_SIZE         256

/*!
 *  @brief AT45DB Object
 *
 *  The application should never directly access the fields in the structure.
 */
typedef struct AT45DB_Object {
    SPI_Handle       spiHandle;
    unsigned int     gpioCS;
    GateMutex_Struct gate;
} AT45DB_Object;

/*!
 *  @brief AT45DB Handle
 *
 *  Used to identify a AD45DB device in the APIs
 */
typedef AT45DB_Object *AT45DB_Handle;

/*!
 *  @brief  Function to initialize a given AT45DB object
 *
 *  Function to initialize a given AT45DB object specified by the
 *  particular SPI handle and GPIO CS index values.
 *
 *  @param  obj           Pointer to a AT45DB_Object structure. It does not
 *                        need to be initialized.
 *
 *  @param  spiHandle     SPI handle that the AT45DB is attached to
 *
 *  @param  gpioCSIndex   GPIO index that is used for the Chip Select
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values. All the fields in this structure are
 *                        RO (read-only).
 *
 *  @return A AT45DB_Handle on success or a NULL on an error.
 *
   @sa     AT45DB_destruct()
 */
AT45DB_Handle AT45DB_construct(AT45DB_Object *obj, SPI_Handle spiHandle,
                               unsigned int gpioCSIndex, AT45DB_Params *params);

/*!
 *  @brief  Function to initialize a given AT45DB device
 *
 *  Function to create a AT45DB object specified by the
 *  particular SPI handle and GPIO CS index values.
 *
 *  @param  spiHandle     SPI handle that the AT45DB is attached to
 *
 *  @param  gpioCSIndex   GPIO index that is used for the Chip Select
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values. All the fields in this structure are
 *                        RO (read-only).
 *
 *  @return A AT45DB_Handle on success or a NULL on an error.
 *
   @sa     AT45DB_delete()
 */
AT45DB_Handle AT45DB_create(SPI_Handle spiHandle, unsigned int gpioCSIndex,
                            AT45DB_Params *params);

/*!
 *  @brief  Function to delete a AT45DB instance
 *
 *  @pre    AT45DB_create() had to be called first.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_create
 *
 *  @sa     AT45DB_create()
 */
void AT45DB_delete(AT45DB_Handle handle);

/*!
 *  @brief  Function to erase a page
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct or
 *                      AT45DB_create
 *
 *  @param  page        Page to erase
 *
 *  @return true if successful, false otherwise
 */
bool AT45DB_erasePage(AT45DB_Handle handle, uint32_t page);

/*!
 *  @brief  Function to delete a AT45DB instance
 *
 *  @pre    AT45DB_construct() had to be called first.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct
 *
 *  @sa     AT45DB_construct()
 */
void AT45DB_destruct(AT45DB_Handle handle);

/*!
 *  @brief  Function to initialize the AT45DB_Params struct to its defaults
 *
 *  Currently there are no real parameters. It is still recommended to use
 *  AT45DB_Paramaters_init to support future enhancements.

 *  @param  params      An pointer to AT45DB_Params structure for
 *                      initialization
 */
void AT45DB_Params_init(AT45DB_Params *params);

/*!
 *  @brief  Function read the flash memory
 *
 *  This function reads the flash memory from the page specified and the
 *  byte offset  in the transaction structure. The amount of memory
 *  to read is also specified in the transaction structure. The read memory
 *  is placed into the data pointer in the transaction structure. Make sure
 *  the buffer pointed to by the data pointer is large enough to hold the
 *  read data.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct/create
 *
 *  @param  transaction Transaction pointer that contains byte offset, size
 *                      of data to read and the place to put the read data.
 *
 *  @param  page        Which flash page to read
 */
bool AT45DB_read(AT45DB_Handle handle, AT45DB_Transaction *transaction,
                 uint32_t page);

/*!
 *  @brief  Function read the SRAM2 buffer memory
 *
 *  This function reads the SRAM2 memory from the byte offset in the
 *  transaction structure. The amount of memory to read is also specified
 *  in the transaction structure. The read memory is placed into the
 *  data pointer in the transaction structure. Make sure the buffer
 *  pointed to by the data pointer is large enough to hold the read data.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct/create
 *
 *  @param  transaction Transaction pointer that contains byte offset, size
 *                      of data to read and the place to put the read data.
 */
bool AT45DB_readBuffer(AT45DB_Handle handle, AT45DB_Transaction *transaction);

/*!
 *  @brief  Function read the status register
 *
 *  This function reads the status register. If AT45DB_READY is returned, the 
 *  the device 
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct/create
 *
 *  @return AT45DB_READY if the device is ready. 
 *
 */
unsigned char AT45DB_readStatusRegister(AT45DB_Handle handle);

/*!
 *  @brief  Function write the flash memory
 *
 *  This function writes to the flash memory on the page specified and at the
 *  byte offset in the transaction structure. The amount of memory
 *  to write is also specified in the transaction structure. The memory to
 *  write is in the data pointer in the transaction structure.
 *
 *  SRAM2 is used to move the data to the device and then it is pushed to
 *  the flash memory.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct/create
 *
 *  @param  transaction Transaction pointer that contains byte offset, size
 *                      of data to write and the data to write.
 *
 *  @param  page        Which flash page to write to
 */
bool AT45DB_write(AT45DB_Handle handle, AT45DB_Transaction *transaction,
                  uint32_t page);

/*!
 *  @brief  Function write the SRAM2 buffer memory
 *
 *  This function writes to the SRAM2 buffer at the byte offset in the
 *  transaction structure. The amount of memory to write is also specified
 *  in the transaction structure. The memory to write is in the data pointer
 *  in the transaction structure.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct/create
 *
 *  @param  transaction Transaction pointer that contains byte offset, size
 *                      of data to write and the data to write.
 */
bool AT45DB_writeBuffer(AT45DB_Handle handle, AT45DB_Transaction *transaction);

/*!
 *  @brief  Function push the SRAM2 buffer memory to flash memory
 *
 *  This function pushes the SRAM2 buffer memory to the specified flash page.
 *
 *  @param  handle      A AT45DB_Handle returned from AT45DB_construct/create
 *
 *  @param  page        Which flash page to write to
 */
bool AT45DB_writeBufferToPage(AT45DB_Handle handle, uint32_t page);

bool AT45DB_pageSelection(AT45DB_Handle handle, AT45DB_Transaction *transaction);

bool AT45DB_readDeviceRegister(AT45DB_Handle handle, unsigned char *deviceInfo);

//bool flash_write(uint32_t page, uint32_t offset,
//		unsigned char *data, uint32_t data_size);

//bool flash_read(uint32_t page, uint32_t offset,
//		unsigned char *data, uint32_t data_size);
bool flash_info(AT45DB_Handle handle, unsigned char *deviceInfo) ;
//bool flash_init();
#ifdef __cplusplus
}
#endif

#endif /* __AT45DB_H */
