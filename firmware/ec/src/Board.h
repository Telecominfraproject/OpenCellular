/*******************************************************************************
  Filename:       Board.h
  Revised:        $Date: 2015-06-02 11:18:40 -0700 (Tue, 02 Jun 2015) $
  Revision:       $Revision: 43957 $

  Description:    This file contains utility functions.

  Copyright 2014 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/
#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/inc/global/OC_CONNECT1.h"

#define Board_initEMAC OC_CONNECT1_initEMAC
#define Board_initGeneral OC_CONNECT1_initGeneral
#define Board_initGPIO OC_CONNECT1_initGPIO
#define Board_initI2C OC_CONNECT1_initI2C
#define Board_initUART OC_CONNECT1_initUART
#define Board_initUSB OC_CONNECT1_initUSB
#define Board_initWatchdog OC_CONNECT1_initWatchdog

#define Board_IOEXP_ALERT OC_EC_GBC_IOEXP71_ALERT
#define Board_ECINA_ALERT OC_EC_GBC_INA_ALERT
#define Board_APINA_ALERT OC_EC_GBC_AP_INA_ALERT
#define Board_SDRFPGA_TEMPINA_ALERT OC_EC_SDR_FPGA_TEMP_INA_ALERT
#define Board_SDR_INA_ALERT OC_EC_SDR_INA_ALERT
#define Board_RFFE_TEMP_INA_ALERT OC_EC_RFFE_TEMP_INA_ALERT
#define Board_SYNC_IOEXP_ALERT OC_EC_SYNC_IOEXP_ALERT
#define Board_LeadAcidAlert OC_EC_PWR_LACID_ALERT
#define Board_LithiumIonAlert OC_EC_PWR_LION_ALERT
#define Board_PSEALERT OC_EC_GBC_PSE_ALERT
#define Board_PD_PWRGDAlert OC_EC_PD_PWRGD_ALERT
#define Board_SOC_UART3_TX OC_EC_SOC_UART3_TX

#define Board_I2C0 OC_CONNECT1_I2C0
#define Board_I2C1 OC_CONNECT1_I2C1
#define Board_I2C2 OC_CONNECT1_I2C2
#define Board_I2C3 OC_CONNECT1_I2C3
#define Board_I2C4 OC_CONNECT1_I2C4
#define Board_I2C6 OC_CONNECT1_I2C6
#define Board_I2C7 OC_CONNECT1_I2C7
#define Board_I2C8 OC_CONNECT1_I2C8
#define Board_I2CCOUNT OC_CONNECT1_I2CCOUNT

#define Board_USBHOST OC_CONNECT1_USBHOST
#define Board_USBDEVICE OC_CONNECT1_USBDEVICE

// TODO: maybe rename to "UART_GSM" and stuff to be more abstracted from HW
#define Board_UART0 OC_CONNECT1_UART0
#define Board_UART3 OC_CONNECT1_UART3
#define Board_UART4 OC_CONNECT1_UART4
#define Board_UARTXR0 OC_CONNECT1_UARTXR0
#define Board_WATCHDOG0 OC_CONNECT1_WATCHDOG0

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
