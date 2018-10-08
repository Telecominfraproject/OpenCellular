/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef EEPROM_H_
#define EEPROM_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/ocmp_frame.h" /* Temporary, just for OCMPSubsystem def */
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define OC_TEST_ADDRESS 0xFFFF
#define OC_CONNECT1_SERIAL_INFO 0x01C6
#define OC_CONNECT1_SERIAL_SIZE 0x12
#define OC_GBC_BOARD_INFO 0x01AC
#define OC_GBC_BOARD_INFO_SIZE 0x12
#define OC_GBC_DEVICE_INFO 0x0100 /*TODO: Update offsets*/
#define OC_SDR_BOARD_INFO 0x01AC
#define OC_SDR_BOARD_INFO_SIZE 0x12
#define OC_SDR_DEVICE_INFO 0x0100 /*TODO: Update offsets*/
#define OC_RFFE_BOARD_INFO 0x01AC
#define OC_RFFE_BOARD_INFO_SIZE 0x11
#define OC_RFFE_DEVICE_INFO 0x0100 /*TODO: Update offsets*/
#define OC_DEVICE_INFO_SIZE 0x0A

/*****************************************************************************
 *                             STRUCT DEFINITIONS
 *****************************************************************************/
typedef struct EepromDev_Cfg {
    /*!< EEPROM size in bytes */
    size_t mem_size;
    /*!< Page size (max bytes we can write in a single shot) */
    size_t page_size;
} EepromDev_Cfg;

typedef struct Eeprom_Cfg {
    I2C_Dev i2c_dev;
    OcGpio_Pin *pin_wp;
    EepromDev_Cfg type; /*!< Device specific config (page size, etc) */
    OCMPSubsystem ss; /* TODO: The HW config need not know about the subsytem
    to be fixed later */
} Eeprom_Cfg, *Eeprom_Handle;

typedef enum {
    OC_STAT_SYS_SERIAL_ID = 0,
    OC_STAT_SYS_GBC_BOARD_ID,
    OC_STAT_SYS_STATE
} eOCStatusParamId;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
bool eeprom_init(Eeprom_Cfg *cfg);

ReturnStatus eeprom_read(const Eeprom_Cfg *cfg, uint16_t address, void *buffer,
                         size_t size);

ReturnStatus eeprom_write(const Eeprom_Cfg *cfg, uint16_t address,
                          const void *buffer, size_t size);

ReturnStatus eeprom_disable_write(Eeprom_Cfg *cfg);

ReturnStatus eeprom_enable_write(Eeprom_Cfg *cfg);

ReturnStatus eeprom_read_oc_info(uint8_t *oc_serial);

ReturnStatus eeprom_read_board_info(const Eeprom_Cfg *cfg, uint8_t *rom_info);

ReturnStatus eeprom_read_device_info_record(const Eeprom_Cfg *cfg,
                                            uint8_t recordNo,
                                            char *device_info);

ReturnStatus eeprom_write_device_info_record(Eeprom_Cfg *cfg, uint8_t recordNo,
                                             char *device_info);

#endif /* EEPROM_H_ */
