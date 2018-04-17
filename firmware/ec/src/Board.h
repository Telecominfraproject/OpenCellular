/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "OC_CONNECT1.h"

#define Board_initEMAC              OC_CONNECT1_initEMAC
#define Board_initGeneral           OC_CONNECT1_initGeneral
#define Board_initGPIO              OC_CONNECT1_initGPIO
#define Board_initI2C               OC_CONNECT1_initI2C
#define Board_initUART              OC_CONNECT1_initUART
#define Board_initUSB               OC_CONNECT1_initUSB
#define Board_initWatchdog          OC_CONNECT1_initWatchdog

#define Board_IOEXP_ALERT           OC_EC_GBC_IOEXP71_ALERT
#define Board_ECINA_ALERT           OC_EC_GBC_INA_ALERT
#define Board_APINA_ALERT           OC_EC_GBC_AP_INA_ALERT
#define Board_SDRFPGA_TEMPINA_ALERT OC_EC_SDR_FPGA_TEMP_INA_ALERT
#define Board_SDR_INA_ALERT         OC_EC_SDR_INA_ALERT
#define Board_RFFE_TEMP_INA_ALERT   OC_EC_RFFE_TEMP_INA_ALERT
#define Board_SYNC_IOEXP_ALERT      OC_EC_SYNC_IOEXP_ALERT
#define Board_LeadAcidAlert         OC_EC_PWR_LACID_ALERT
#define Board_LithiumIonAlert       OC_EC_PWR_LION_ALERT
#define Board_PSEALERT              OC_EC_GBC_PSE_ALERT
#define Board_PD_PWRGDAlert         OC_EC_PD_PWRGD_ALERT
#define Board_SOC_UART3_TX          OC_EC_SOC_UART3_TX

#define Board_I2C0                  OC_CONNECT1_I2C0
#define Board_I2C1                  OC_CONNECT1_I2C1
#define Board_I2C2                  OC_CONNECT1_I2C2
#define Board_I2C3                  OC_CONNECT1_I2C3
#define Board_I2C4                  OC_CONNECT1_I2C4
#define Board_I2C6                  OC_CONNECT1_I2C6
#define Board_I2C7                  OC_CONNECT1_I2C7
#define Board_I2C8                  OC_CONNECT1_I2C8
#define Board_I2CCOUNT              OC_CONNECT1_I2CCOUNT

#define Board_USBHOST               OC_CONNECT1_USBHOST
#define Board_USBDEVICE             OC_CONNECT1_USBDEVICE

// TODO: maybe rename to "UART_GSM" and stuff to be more abstracted from HW
#define Board_UART0                 OC_CONNECT1_UART0
#define Board_UART3                 OC_CONNECT1_UART3
#define Board_UART4                 OC_CONNECT1_UART4
#define Board_UARTXR0               OC_CONNECT1_UARTXR0
#define Board_WATCHDOG0             OC_CONNECT1_WATCHDOG0

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
