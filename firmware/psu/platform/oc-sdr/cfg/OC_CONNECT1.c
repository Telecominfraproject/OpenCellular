/*******************************************************************************
  Filename:       OC_CONNECT1.c
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
/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */


#include <inc/global/OC_CONNECT1.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>

#include <stdint.h>
#include <stdbool.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <driverlib/flash.h>
#include <driverlib/gpio.h>
#include <driverlib/i2c.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
#include <driverlib/udma.h>

#ifndef TI_DRIVERS_UART_DMA
#define TI_DRIVERS_UART_DMA 0
#endif

#ifndef TI_EXAMPLES_PPP
#define TI_EXAMPLES_PPP 0
#else
/* prototype for NIMU init function */
extern int USBSerialPPP_NIMUInit();
#endif

#include <ti/drivers/uart/UARTTiva.h>


typedef enum DK_TM4C129X_UARTName {
    DK_TM4C129X_UART0 = 0,
    DK_TM4C129X_UART1,
    DK_TM4C129X_UART2,
    DK_TM4C129X_UART3,
    DK_TM4C129X_UART4,
    DK_TM4C129X_UARTCOUNT
} DK_TM4C129X_UARTName;

UARTTiva_Object uartTivaObjects[DK_TM4C129X_UARTCOUNT];
unsigned char uartTivaRingBuffer[DK_TM4C129X_UARTCOUNT][32];

const UARTTiva_HWAttrs uartTivaHWAttrs[DK_TM4C129X_UARTCOUNT] = {
    {
        .baseAddr = UART0_BASE,
        .intNum = INT_UART0,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[0],
        .ringBufSize = sizeof(uartTivaRingBuffer[0])
    },
    {
        .baseAddr = UART1_BASE,
        .intNum = INT_UART1,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[1],
        .ringBufSize = sizeof(uartTivaRingBuffer[1])
    },
    {
        .baseAddr = UART2_BASE,
        .intNum = INT_UART2,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[2],
        .ringBufSize = sizeof(uartTivaRingBuffer[2])
    },
    {
        .baseAddr = UART4_BASE,
        .intNum = INT_UART3,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[3],
        .ringBufSize = sizeof(uartTivaRingBuffer[3])
    },
    {
        .baseAddr = UART4_BASE,
        .intNum = INT_UART4,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[4],
        .ringBufSize = sizeof(uartTivaRingBuffer[4])
    },
    {
        .baseAddr = UART5_BASE,
        .intNum = INT_UART5,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[5],
        .ringBufSize = sizeof(uartTivaRingBuffer[5])
    }
};

const UART_Config UART_config[] = {
    {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[0],
        .hwAttrs = &uartTivaHWAttrs[0]
    },
    {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[1],
        .hwAttrs = &uartTivaHWAttrs[1]
    },
    {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[2],
        .hwAttrs = &uartTivaHWAttrs[2]
    },
    {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[3],
        .hwAttrs = &uartTivaHWAttrs[3]
    },
    {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[4],
        .hwAttrs = &uartTivaHWAttrs[4]
    },
    {NULL, NULL, NULL}
};

#include "common/inc/global/ocmp_frame.h"
#include "inc/common/global_header.h"
char input[64];
UART_Handle uart;

Void echoFxn(UArg arg0, UArg arg1)
{
 //   char input;
    UART_Params uartParams;
    const char echoPrompt[] = "\fEchoing characters:\r\n";

    OC_CONNECT1_initUART();

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;
    uart = UART_open(DK_TM4C129X_UART4, &uartParams);

    if (uart == NULL) {
        System_abort("Error opening the UART");
    }

 //   UART_write(uart, echoPrompt, sizeof(echoPrompt));

    /* Loop forever echoing */
   // input = '1';
    while (1) {
        UART_read(uart, &input, 64);
        uint8_t * pWrite = NULL;
        pWrite = (uint8_t *) malloc(
                sizeof(OCMPMessageFrame) + OCMP_FRAME_MSG_LENGTH);
        if (pWrite != NULL) {
            memset(pWrite, '\0', 64);
            memcpy(pWrite, input, 64);
#if 1
            uint8_t i = 0;
            LOGGER_DEBUG("UARTDMACTR:INFO:: UART RX BUFFER:\n");
            for( i = 0; i < 64; i++)
            {
                LOGGER_DEBUG("0x%x  ",pWrite[i]);
            }
            LOGGER_DEBUG("\n");
        }
#endif
  //      UART_write(uart, &input, 1);
    }
}

/*
 *  =============================== DMA ===============================
 */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(dmaControlTable, 1024)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=1024
#elif defined(__GNUC__)
__attribute__ ((aligned (1024)))
#endif
static tDMAControlTable dmaControlTable[32];
static bool dmaInitialized = false;

/* Hwi_Struct used in the initDMA Hwi_construct call */
static Hwi_Struct dmaHwiStruct;

/* Hwi_Struct used in the usbBusFault Hwi_construct call */
static Hwi_Struct usbBusFaultHwiStruct;

/*
 *  ======== dmaErrorHwi ========
 */
static void dmaErrorHwi(UArg arg)
{
    System_printf("DMA error code: %d\n", uDMAErrorStatusGet());
    uDMAErrorStatusClear();
    System_abort("DMA error!!");
}

/*
 *  ======== OC_CONNECT1_initDMA ========
 */
void OC_CONNECT1_initDMA(void)
{
    Error_Block eb;
    Hwi_Params hwiParams;

    if (!dmaInitialized) {
        Error_init(&eb);
        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(dmaHwiStruct), INT_UDMAERR, dmaErrorHwi, &hwiParams,
                      &eb);
        if (Error_check(&eb)) {
            System_abort("Couldn't construct DMA error hwi");
        }

        SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
        uDMAEnable();
        uDMAControlBaseSet(dmaControlTable);

        dmaInitialized = true;
    }
}

/*
 *  =============================== General ===============================
 */
/*
 *  ======== OC_CONNECT1_initGeneral ========
 */
void OC_CONNECT1_initGeneral(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
}
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(EMAC_config, ".const:EMAC_config")
#pragma DATA_SECTION(emacHWAttrs, ".const:emacHWAttrs")
#pragma DATA_SECTION(NIMUDeviceTable, ".data:NIMUDeviceTable")
#endif

#include <ti/drivers/EMAC.h>
#include <ti/drivers/emac/EMACSnow.h>

/*
 *  Required by the Networking Stack (NDK). This array must be NULL terminated.
 *  This can be removed if NDK is not used.
 *  Double curly braces are needed to avoid GCC bug #944572
 *  https://bugs.launchpad.net/gcc-linaro/+bug/944572
 */
NIMU_DEVICE_TABLE_ENTRY NIMUDeviceTable[2] = {
    {
#if TI_EXAMPLES_PPP
        /* Use PPP driver for PPP example only */
        .init = USBSerialPPP_NIMUInit
#else
        /* Default: use Ethernet driver */
        .init = EMACSnow_NIMUInit
#endif
    },
    { NULL }
};

EMACSnow_Object emacObjects[OC_CONNECT1_EMACCOUNT];

/*
 *  EMAC configuration structure
 *  Set user/company specific MAC octates. The following sets the address
 *  to ff-ff-ff-ff-ff-ff. Users need to change this to make the label on
 *  their boards.
 */
unsigned char macAddress[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

const EMACSnow_HWAttrs emacHWAttrs[OC_CONNECT1_EMACCOUNT] = {
    [OC_CONNECT1_EMAC0] = {
        .baseAddr = EMAC0_BASE,
  //      .intNum = INT_EMAC0,
        .intPriority = (~0),
        .macAddress = macAddress
    }
};

const EMAC_Config EMAC_config[] = {
    [OC_CONNECT1_EMAC0] = {
        .fxnTablePtr = &EMACSnow_fxnTable,
        .object = &emacObjects[OC_CONNECT1_EMAC0],
        .hwAttrs = &emacHWAttrs[OC_CONNECT1_EMAC0]
    },
    { NULL, NULL, NULL }
};

/*
 *  ======== OC_CONNECT1_initEMAC ========
 */
void OC_CONNECT1_initEMAC(void)
{
    uint32_t ulUser0, ulUser1;

    /* Get the MAC address */
    FlashUserGet(&ulUser0, &ulUser1);
    if ((ulUser0 != 0xffffffff) && (ulUser1 != 0xffffffff)) {
        System_printf("Using MAC address in flash\n");
        /*
         *  Convert the 24/24 split MAC address from NV ram into a 32/16 split
         *  MAC address needed to program the hardware registers, then program
         *  the MAC address into the Ethernet Controller registers.
         */
        macAddress[0] = ((ulUser0 >> 0) & 0xff);
        macAddress[1] = ((ulUser0 >> 8) & 0xff);
        macAddress[2] = ((ulUser0 >> 16) & 0xff);
        macAddress[3] = ((ulUser1 >> 0) & 0xff);
        macAddress[4] = ((ulUser1 >> 8) & 0xff);
        macAddress[5] = ((ulUser1 >> 16) & 0xff);
    } else if (macAddress[0] == 0xff && macAddress[1] == 0xff
               && macAddress[2] == 0xff && macAddress[3] == 0xff
               && macAddress[4] == 0xff && macAddress[5] == 0xff) {
        System_printf("Change the macAddress variable to valid Mac address");
    }

//    GPIOPinConfigure(GPIO_PF0_EN0LED0); /* OC_CONNECT1_USR_D3 */
//    GPIOPinConfigure(GPIO_PF4_EN0LED1); /* OC_CONNECT1_USR_D4 */
//    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

    /* Once EMAC_init is called, EMAC_config cannot be changed */
    EMAC_init();
}

/*
 *  =============================== GPIO ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(GPIOTiva_config, ".const:GPIOTiva_config")
#endif

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOTiva.h>
extern GPIO_PinConfig gpioPinConfigs[];
/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in OC_CONNECT1.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array.  Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[OC_EC_GPIOCOUNT] = {
    [OC_EC_PSE_INT] =
        GPIOTiva_PA_0 |  GPIO_CFG_INPUT | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_nPSE_RESET] =
        GPIOTiva_PA_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_WD_INPUT] =
        GPIOTiva_PA_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    [OC_EC_ENABLE_OC_INPUT] =
        GPIOTiva_PA_3 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_POWER_OFF] =
        GPIOTiva_PA_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_DISABLE_DC_INPUT] =
        GPIOTiva_PB_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_IOEXP_INT] =
        GPIOTiva_PB_1 | GPIO_CFG_INPUT |  GPIO_CFG_IN_INT_FALLING,
    [OC_EC_ENABLE_PASSIVE_POE] =
        GPIOTiva_PB_4 | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,
    [OC_EC_CS_ALERT_24V_20V] =
        GPIOTiva_PB_5 | GPIO_CFG_IN_PU ,
    [OC_EC_EN_INT_BATT_PWR] =
        GPIOTiva_PC_7 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_DC_INPUT_FAULT] =
        GPIOTiva_PC_6 | GPIO_CFG_INPUT  | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_UART_TX] =
        GPIOTiva_PC_5 | GPIO_CFG_OUTPUT,
    [OC_EC_CS_ALERT] =
        GPIOTiva_PD_0 | GPIO_CFG_INPUT ,
    [OC_EC_DC_IN_PRESENT] =
        GPIOTiva_PD_1 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_CS_ALERT_12V_GBC] =
        GPIOTiva_PD_2 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_CS_ALERT_12V_BB] =
        GPIOTiva_PD_3 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_CS_ALERT_12V_FE] =
        GPIOTiva_PD_4 | GPIO_CFG_IN_PU ,
    [OC_EC_PGOOD_5V0] =
        GPIOTiva_PD_5 | GPIO_CFG_IN_NOPULL,
    [OC_EC_PGOOD_12V0] =
        GPIOTiva_PD_6 | GPIO_CFG_INPUT,
    [OC_EC_IVINMON] =
        GPIOTiva_PE_0 | GPIO_CFG_IN_NOPULL,
    [OC_EC_ISMON] =
        GPIOTiva_PE_1 | GPIO_CFG_IN_NOPULL,
    [OC_EC_CHARGER_ALERT] =
        GPIOTiva_PE_2 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_PGOOD_BOOST_CONV_BATT] =
        GPIOTiva_PE_3 | GPIO_CFG_IN_NOPULL ,
    [OC_EC_TIVA_GPIO1] =
        GPIOTiva_PF_0 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_TIVA_GPIO2] =
        GPIOTiva_PF_1 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_BUZZER_ON] =
        GPIOTiva_PF_2 | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,
    [OC_EC_PD_T2P] =
        GPIOTiva_PF_3 | GPIO_CFG_INPUT ,
    [OC_EC_TEMP_EVENT] =
        GPIOTiva_PF_4 | GPIO_CFG_IN_NOPULL,
    [OC_EC_OC_IN_PRESENT] =
        GPIOTiva_PG_4 | GPIO_CFG_INPUT,
    [OC_EC_POE_IN_PRESENT] =
        GPIOTiva_PG_5 | GPIO_CFG_INPUT,
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in OC_CONNECT1.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 *       We have lots of RAM right now, so just set it to full size of
 *       GPIO array
 */
GPIO_CallbackFxn gpioCallbackFunctions[OC_EC_GPIOCOUNT] = { };

/* The device-specific GPIO_config structure */
const GPIOTiva_Config GPIOTiva_config = {
    .pinConfigs = (GPIO_PinConfig *) gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *) gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions) /
                         sizeof(GPIO_CallbackFxn),
    .intPriority = (~0)
};

/* Below is a sample system config demonstrating how we can configure our
 * subsystems. It will effectively replace board.h. I've kept the header
 * includes here since I'm thinking we'll want to move this out at some point
 */
#include "common/inc/global/ocmp_frame.h"
#include "drivers/GpioSX1509.h"
#include "drivers/GpioNative.h"

OcGpio_Port ec_io = {
    .fn_table = &GpioNative_fnTable,
};

OcGpio_Port pwr_io = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C5, 0x3E },
     //   .pin_irq = &(OcGpio_Pin){ &ec_io, OC_EC_IOEXP_INT},
    },
    .object_data = &(SX1509_Obj){},
};

/*
 *  ======== OC_CONNECT1_initGPIO ========
 */
void OC_CONNECT1_initGPIO(void)
{
    /* Initialize peripheral and pins */
    GPIO_init();

    GpioNative_init();
}

/*
 *  =============================== I2C ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(I2C_config, ".const:I2C_config")
#pragma DATA_SECTION(i2cTivaHWAttrs, ".const:i2cTivaHWAttrs")
#endif

#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CTiva.h>

I2CTiva_Object i2cTivaObjects[OC_CONNECT1_I2CCOUNT];

const I2CTiva_HWAttrs i2cTivaHWAttrs[OC_CONNECT1_I2CCOUNT] = {
    [OC_CONNECT1_I2C0] = {
        .baseAddr = I2C0_BASE,
        .intNum = INT_I2C0,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C1] = {
        .baseAddr = I2C1_BASE,
        .intNum = INT_I2C1,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C2] = {
        .baseAddr = I2C2_BASE,
        .intNum = INT_I2C2,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C3] = {
        .baseAddr = I2C3_BASE,
        .intNum = INT_I2C3,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C4] = {
        .baseAddr = I2C4_BASE,
        .intNum = INT_I2C4,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C5] = {
        .baseAddr = I2C5_BASE,
        .intNum = INT_I2C5,
        .intPriority = (~0)
    },
};

const I2C_Config I2C_config[] = {
    [OC_CONNECT1_I2C0] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C0],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C0]
    },
    [OC_CONNECT1_I2C1] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C1],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C1]
    },
    [OC_CONNECT1_I2C2] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C2],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C2]
    },
    [OC_CONNECT1_I2C3] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C3],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C3]
    },
    [OC_CONNECT1_I2C4] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C4],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C4]
    },
    [OC_CONNECT1_I2C5] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C5],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C5]
    },
    { NULL, NULL, NULL }
};

/*
 *  ======== OC_CONNECT1_initI2C ========
 */
void OC_CONNECT1_initI2C(void)
{

    //Remove I2C0 PWR2
    /* I2C0 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    /* I2C1 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    /* I2C2 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PE4_I2C2SCL);
    GPIOPinConfigure(GPIO_PE5_I2C2SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTE_BASE, GPIO_PIN_4);
    GPIOPinTypeI2C(GPIO_PORTE_BASE, GPIO_PIN_5);

    /* I2C3 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PG0_I2C3SCL);
    GPIOPinConfigure(GPIO_PG1_I2C3SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTG_BASE, GPIO_PIN_0);
    GPIOPinTypeI2C(GPIO_PORTG_BASE, GPIO_PIN_1);

    /* I2C4 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C4);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PG2_I2C4SCL);
    GPIOPinConfigure(GPIO_PG3_I2C4SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTG_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTG_BASE, GPIO_PIN_3);

    /* I2C5 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C5);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PB6_I2C5SCL);
    GPIOPinConfigure(GPIO_PB7_I2C5SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_7);

    I2C_init();
}

/*
 *  =============================== UART ===============================
 */

/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(UART_config, ".const:UART_config")
#pragma DATA_SECTION(uartTivaHWAttrs, ".const:uartTivaHWAttrs")
#endif

#include <ti/drivers/UART.h>
/*  ======== OC_CONNECT1_initUART ========
 */
void OC_CONNECT1_initUART(void)
{

    // UART For external communication
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    GPIOPinConfigure(GPIO_PC4_U4RX);
    GPIOPinConfigure(GPIO_PC5_U4TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

  //  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
  //  GPIOPinConfigure(GPIO_PC4_U1RX);
  //  GPIOPinConfigure(GPIO_PC5_U1TX);
  //  GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    /* Initialize the UART driver */
#if TI_DRIVERS_UART_DMA
    OC_CONNECT1_initDMA();
#endif
    UART_init();
}

/*
 *  =============================== USB ===============================
 */
/*
 *  ======== OC_CONNECT1_usbBusFaultHwi ========
 */
#if 0
static void OC_CONNECT1_usbBusFaultHwi(UArg arg)
{
    /*
     *  This function should be modified to appropriately manage handle
     *  a USB bus fault.
     */
    System_printf("USB bus fault detected.");
    Hwi_clearInterrupt(INT_GPIOQ4);
    System_abort("USB error!!");
}

/*
 *  ======== OC_CONNECT1_initUSB ========
 *  This function just turns on the USB
 */

void OC_CONNECT1_initUSB(OC_CONNECT1_USBMode usbMode)
{
//PWR2
    Error_Block eb;
    Hwi_Params hwiParams;

    /* Enable the USB peripheral and PLL */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    SysCtlUSBPLLEnable();

    /* Setup pins for USB operation */
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    /* Additional configurations for Host mode */
    if (usbMode == OC_CONNECT1_USBHOST) {
        /* Configure the pins needed */
        HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
        HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;
        GPIOPinConfigure(GPIO_PD6_USB0EPEN);
        GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);

        /*
         *  USB bus fault is routed to pin PQ4.  We create a Hwi to allow us
         *  to detect power faults and recover gracefully or terminate the
         *  program.  PQ4 is active low; set the pin as input with a weak
         *  pull-up.
         */
        GPIOPadConfigSet(GPIO_PORTQ_BASE, GPIO_PIN_4,
        GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
        GPIOIntTypeSet(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
        GPIOIntClear(GPIO_PORTQ_BASE, GPIO_PIN_4);

        /* Create a Hwi for PQ4 pin. */
        Error_init(&eb);
        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(usbBusFaultHwiStruct), INT_GPIOQ4,
                      OC_CONNECT1_usbBusFaultHwi, &hwiParams, &eb);
        if (Error_check(&eb)) {
            System_abort("Couldn't construct USB bus fault hwi");
        }
    }
}
#endif


/*
 *  =============================== Watchdog ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Watchdog_config, ".const:Watchdog_config")
#pragma DATA_SECTION(watchdogTivaHWAttrs, ".const:watchdogTivaHWAttrs")
#endif

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogTiva.h>

WatchdogTiva_Object watchdogTivaObjects[OC_CONNECT1_WATCHDOGCOUNT];

const WatchdogTiva_HWAttrs watchdogTivaHWAttrs[OC_CONNECT1_WATCHDOGCOUNT] = {
    [OC_CONNECT1_WATCHDOG0] = {
        .baseAddr = WATCHDOG0_BASE,
        .intNum = INT_WATCHDOG,
        .intPriority = (~0),
        .reloadValue = 80000000 // 1 second period at default CPU clock freq
        },
};

const Watchdog_Config Watchdog_config[] = {
    [OC_CONNECT1_WATCHDOG0] = {
        .fxnTablePtr = &WatchdogTiva_fxnTable,
        .object = &watchdogTivaObjects[OC_CONNECT1_WATCHDOG0],
        .hwAttrs = &watchdogTivaHWAttrs[OC_CONNECT1_WATCHDOG0]
    },
    { NULL, NULL, NULL },
};

/*
 *  ======== OC_CONNECT1_initWatchdog ========
 *
 * NOTE: To use the other watchdog timer with base address WATCHDOG1_BASE,
 *       an additional function call may need be made to enable PIOSC. Enabling
 *       WDOG1 does not do this. Enabling another peripheral that uses PIOSC
 *       such as ADC0 or SSI0, however, will do so. Example:
 *
 *       SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
 *       SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG1);
 *
 *       See the following forum post for more information:
 *       http://e2e.ti.com/support/microcontrollers/stellaris_arm_cortex-m3_microcontroller/f/471/p/176487/654390.aspx#654390
 */
void OC_CONNECT1_initWatchdog(void)
{
    /* Enable peripherals used by Watchdog */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    Watchdog_init();
}
