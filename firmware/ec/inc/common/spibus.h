/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef INC_COMMON_SPIBUS_H_
#define INC_COMMON_SPIBUS_H_


/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"
#include <stdint.h>
#include <stdbool.h>
#include <ti/drivers/SPI.h>
#include <ti/sysbios/gates/GateMutex.h>

typedef struct SPI_Dev {
    unsigned int bus;
    OcGpio_Pin *chip_select;
} SPI_Dev;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
SPI_Handle spi_get_handle(uint32_t index);

ReturnStatus spi_reg_read(SPI_Handle spiHandle,
                          OcGpio_Pin *chip_select,
                          void *regAddress,
                          uint8_t *data,
                          uint32_t data_size,
                          uint32_t byte,
                          uint8_t numofBytes);

ReturnStatus spi_reg_write(SPI_Handle spiHandle,
                           OcGpio_Pin *chip_select,
                           void *regAddress,
                           uint8_t *data,
                           uint32_t data_size,
                           uint32_t byte,
                           uint8_t numofBytes);


#endif /* INC_COMMON_SPIBUS_H_ */
