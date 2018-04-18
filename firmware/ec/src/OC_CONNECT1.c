/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdint.h>
#include <stdbool.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>

#include <driverlib/flash.h>
#include <driverlib/gpio.h>
#include <driverlib/i2c.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
#include <driverlib/udma.h>

#include "OC_CONNECT1.h"

#ifndef TI_DRIVERS_UART_DMA
#define TI_DRIVERS_UART_DMA 0
#endif

#ifndef TI_EXAMPLES_PPP
#define TI_EXAMPLES_PPP 0
#else
/* prototype for NIMU init function */
extern int USBSerialPPP_NIMUInit();
#endif

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
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    // TODO: why did we comment this out?
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOR);
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOS);
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOT);
}

/*
 *  =============================== EMAC ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
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
        .intNum = INT_EMAC0,
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
    [OC_EC_SOC_UART3_TX] =
        GPIOTiva_PA_5 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_SDR_INA_ALERT] =
        GPIOTiva_PD_2 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_PWR_PSE_RESET] =
        GPIOTiva_PD_3 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    [OC_EC_PWR_PRSNT_SOLAR_AUX] =
        GPIOTiva_PD_6 | GPIO_CFG_IN_PU ,
    [OC_EC_SYNC_IOEXP_ALERT] =
        GPIOTiva_PD_7 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_GBC_IOEXP71_ALERT] =
        GPIOTiva_PE_0 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_FE_CONTROL] =
        GPIOTiva_PE_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_GPP_AP_BM_1] =
        GPIOTiva_PE_3 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_GPP_PMIC_CORE_PWR] =
        GPIOTiva_PH_0 | GPIO_CFG_IN_PU ,
    [OC_EC_GPP_SOC_PLTRST] =
        GPIOTiva_PH_1 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_GPP_PMIC_CTRL] =
        GPIOTiva_PH_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_GBC_INA_ALERT] =
        GPIOTiva_PH_3 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_PWR_PD_NT2P] =
        GPIOTiva_PJ_0 | GPIO_CFG_IN_PU ,
    [OC_EC_GBC_AP_INA_ALERT] =
        GPIOTiva_PJ_1 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_GBC_PSE_ALERT] =
        GPIOTiva_PL_2 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_GPP_AP_BM_2] =
        GPIOTiva_PL_3 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_PWR_PRSNT_POE] =
        GPIOTiva_PL_5 | GPIO_CFG_IN_PU ,
    [OC_EC_PWR_LION_ALERT] =
        GPIOTiva_PM_0 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_HCI_LED_RESET] =
        GPIOTiva_PM_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    [OC_EC_PWR_LACID_ALERT] =
        GPIOTiva_PM_3 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_RFFE_TEMP_INA_ALERT] =
        GPIOTiva_PM_4 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_ETH_SW_RESET] =
        GPIOTiva_PM_5 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    [OC_EC_PWR_BATT_SELECT] =
        GPIOTiva_PM_7 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    [OC_EC_PD_PWRGD_ALERT] =
        GPIOTiva_PN_0 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES,
    [OC_EC_SDR_FPGA_TEMP_INA_ALERT] =
        GPIOTiva_PN_1 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    [OC_EC_SDR_PWR_GD] =
        GPIOTiva_PN_3 | GPIO_CFG_IN_NOPULL,
    [OC_EC_SDR_DEVICE_CONTROL] =
        GPIOTiva_PN_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_FE_PWR_GD] =
        GPIOTiva_PN_4 | GPIO_CFG_IN_NOPULL,
    [OC_EC_SDR_FE_IO_RESET_CTRL] =
        GPIOTiva_PP_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_SDR_PWR_CNTRL] =
        GPIOTiva_PP_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_GPP_PWRGD_PROTECTION] =
        GPIOTiva_PP_3 | GPIO_CFG_IN_PU ,
    [OC_EC_RFFE_RESET] =
        GPIOTiva_PP_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    [OC_EC_FE_TRXFE_CONN_RESET] = /* Watchdog nAO pin */
        GPIOTiva_PQ_0 | GPIO_CFG_IN_NOPULL,
    [OC_EC_GPP_MSATA_DAS] =
        GPIOTiva_PQ_1 | GPIO_CFG_IN_PU ,
    [OC_EC_GPP_RST_TO_PROC] =
        GPIOTiva_PQ_3 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    [OC_EC_SYNC_RESET] =
        GPIOTiva_PQ_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
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

#include "drivers/GpioNative.h"
#include "drivers/GpioSX1509.h"
#include "drivers/GpioPCA9557.h"
#include "inc/common/ocmp_frame.h"
#include "inc/devices/eeprom.h"
#include "inc/devices/eth_sw.h"
#include "inc/devices/ext_battery.h" /* Just for battery resistor configs */
#include "inc/devices/int_battery.h"
#include "inc/subsystem/bms/bms.h"
#include "inc/subsystem/gpp/gpp.h"
#include "inc/subsystem/hci/hci.h"
#include "inc/subsystem/obc/obc.h"
#include "inc/subsystem/power/power.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "inc/subsystem/rffe/rffe_powermonitor.h"
#include "inc/subsystem/rffe/rffe_sensor.h"
#include "inc/subsystem/sdr/sdr.h"
#include "inc/subsystem/sync/sync.h"
#include "inc/subsystem/testModule/testModule.h"

OcGpio_Port ec_io;
OcGpio_Port gbc_io_0;
OcGpio_Port gbc_io_1;
OcGpio_Port sdr_fx3_io;
//OcGpio_Port sdr_eeprom_wp_io;
OcGpio_Port fe_ch1_gain_io;
OcGpio_Port fe_ch2_gain_io ;
OcGpio_Port fe_ch1_lna_io;
OcGpio_Port fe_ch2_lna_io;
OcGpio_Port fe_watchdog_io;
OcGpio_Port sync_io;

OcGpio_Port ec_io = {
    .fn_table = &GpioNative_fnTable,
};

#define BIGBROTHER_IOEXP0_ADDRESS           0x71
OcGpio_Port gbc_io_0 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C6, BIGBROTHER_IOEXP0_ADDRESS },
        .pin_irq = &(OcGpio_Pin){ &ec_io, OC_EC_GBC_IOEXP71_ALERT },
    },
    .object_data = &(SX1509_Obj){},
};

#define BIGBROTHER_IOEXP1_ADDRESS           0x70
OcGpio_Port gbc_io_1 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C6, BIGBROTHER_IOEXP1_ADDRESS },
        .pin_irq = NULL, /* This IO expander doesn't provide interrupts */
    },
    .object_data = &(SX1509_Obj){},
};

OcGpio_Port sdr_fx3_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C3, SDR_FX3_IOEXP_ADDRESS },
    },
    .object_data = &(PCA9557_Obj){},
};

/* This IO expander is disabled on rev c */
/* OcGpio_Port sdr_eeprom_wp_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C3, SDR_EEPROM_IOEXP_ADDRESS },
    },
    .object_data = &(PCA9557_Obj){},
}; */

OcGpio_Port fe_ch1_gain_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL1_IO_TX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port fe_ch2_gain_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL2_IO_TX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port fe_ch1_lna_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL1_IO_RX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port fe_ch2_lna_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_CHANNEL2_IO_RX_ATTEN_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port fe_watchdog_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C2, RFFE_IO_REVPOWER_ALERT_ADDR },
    },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port sync_io  = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg = &(SX1509_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C7, SYNC_IO_DEVICE_ADDR },
        .pin_irq = &(OcGpio_Pin){ &ec_io, OC_EC_SYNC_IOEXP_ALERT },
    },
    .object_data = &(SX1509_Obj){},
};

/* These are terrible pin names, but they match the net names... */
OcGpio_Pin pin_inven_eeprom_wp      = { &gbc_io_0, 1, OCGPIO_CFG_OUT_OD_NOPULL };
OcGpio_Pin pin_s_id_eeprom_wp       = { &gbc_io_0, 2, OCGPIO_CFG_OUT_OD_NOPULL };
OcGpio_Pin pin_tempsen_evt1         = { &gbc_io_0, 4 };
OcGpio_Pin pin_tempsen_evt2         = { &gbc_io_0, 5 };
OcGpio_Pin pin_tempsen_evt3         = { &gbc_io_0, 6 };
OcGpio_Pin pin_tempsen_evt4         = { &gbc_io_0, 7 };
OcGpio_Pin pin_tempsen_evt5         = { &gbc_io_0, 8 };
OcGpio_Pin eth_sw_tiva_intn         = { &gbc_io_0, 11 };

/*****************************************************************************
 *                               EEPROM CONFIG
 *****************************************************************************/
#define CAT24C256 { .page_size = 64, .mem_size = (256 / 8) }

Eeprom_Cfg eeprom_gbc_sid = {
    .i2c_dev = { OC_CONNECT1_I2C7, 0x51 },
    .pin_wp = &pin_s_id_eeprom_wp,
    .type = CAT24C256,
};

Eeprom_Cfg eeprom_gbc_inv = {
    .i2c_dev = { OC_CONNECT1_I2C7, 0x50 },
    .pin_wp = &pin_inven_eeprom_wp,
    .type = CAT24C256,
};

Eeprom_Cfg eeprom_sdr_inv = {
    .i2c_dev = { OC_CONNECT1_I2C3, 0x50 },
    /* .pin_wp = &(OcGpio_Pin){ &sdr_eeprom_wp_io, 0 }, */
    .pin_wp = NULL, /* IO Expander disabled on rev c */
    .type = CAT24C256,
};

Eeprom_Cfg eeprom_fe_inv = {
    .i2c_dev = { OC_CONNECT1_I2C4, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &fe_ch1_lna_io, 0 },
    .type = CAT24C256,
};

/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
Power_Cfg g_power_cfg = {
    /* EC_PD_PWRGD_OK */
    .pin_ec_pd_pwrgd_ok     = { &ec_io, OC_EC_PD_PWRGD_ALERT },
    /* SOLAR_AUX_PRSNT_N */
    .pin_solar_aux_prsnt_n  = { &ec_io, OC_EC_PWR_PRSNT_SOLAR_AUX },
    /* POE_PRSNT_N */
    .pin_poe_prsnt_n        = { &ec_io, OC_EC_PWR_PRSNT_POE },
    /* LT4275_EC_nT2P */
    .pin_lt4275_ec_nt2p     = { &ec_io, OC_EC_PWR_PD_NT2P },
    /* nECPSE_RST */
    .pin_necpse_rst         = { &ec_io, OC_EC_PWR_PSE_RESET },
    /* EC_LT4015_I2C_SEL */
    .pin_lt4015_i2c_sel     = { &gbc_io_1, 4, OCGPIO_CFG_OUT_STD },
    /* INT_BAT_PRSNT */
    .pin_int_bat_prsnt      = { &gbc_io_0, 11 },
    /* EXT_BAT_PRSNT */
    .pin_ext_bat_prsnt      = { &gbc_io_0, 12 },
    /* TODO: consider inverting int/ext_bat_prsnt since the opto-isolator
     * basically inverts the line */
    .lead_acid_temp_sens = {
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C1,
                .slave_addr = PWR_LEAD_ACID_BATT_DEV_TEMP_SENS_ADDR
            },
            .pin_evt = &pin_tempsen_evt1,
	    },
        .obj = {NULL},
    },
    .ext_bat_charger = {
        .cfg = {
            .i2c_dev = {
                .bus = OC_CONNECT1_I2C0,
                .slave_addr = 0x68, /* LTC4015 I2C address in 7-bit format */
            },
            .chem = LTC4015_CHEM_LEAD_ACID,
            .r_snsb = PWR_EXT_BATT_RSNSB,
            .r_snsi = PWR_EXT_BATT_RSNSI,
            .cellcount = 6,
            .pin_lt4015_i2c_sel = { &gbc_io_1, 4, OCGPIO_CFG_OUT_OD_NOPULL },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_PWR_LACID_ALERT },
        },
        .obj = {},
    },
    .int_bat_charger = {
        .cfg = {
            .i2c_dev = {
                .bus = OC_CONNECT1_I2C0,
                .slave_addr = 0x68, /* LTC4015 I2C address in 7-bit format */
            },
            .chem = LTC4015_CHEM_LI_ION,
            .r_snsb = PWR_INT_BATT_RSNSB,
            .r_snsi = PWR_INT_BATT_RSNSI,
            .cellcount = 3,
            .pin_lt4015_i2c_sel = { &gbc_io_1, 4, OCGPIO_CFG_OUT_OD_NOPULL },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_PWR_LION_ALERT },
        },
        .obj = {},
    },
    .pse = {
        .cfg = {
            .i2c_dev = {
                .bus = OC_CONNECT1_I2C8,
                .slave_addr = 0x2F, /* LTC4274 I2C address in 7-bit format */
            },
            .pin_evt = &(OcGpio_Pin){ &ec_io,
                                      OC_EC_GBC_PSE_ALERT },
        },
        .obj = {},
    },
    .pd = {
        .cfg = {
                .pin_evt = &(OcGpio_Pin){ &ec_io,
                                           OC_EC_PD_PWRGD_ALERT },
                .pin_detect = &(OcGpio_Pin){ &ec_io,
                                              OC_EC_PWR_PD_NT2P },

            },
            .obj = {},
        },
        .powerSource = { /*Added as a place holder for now.*/
            .cfg = {

            },
            .obj = {},
        },
};

Bms_Cfg g_bms_cfg = {
    .ec_current_sensor_12v = {
        /* 12V Power Sensor */
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C6,
                .slave_addr = BMS_EC_CURRENT_SENSOR_12V_ADDR,
            },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_GBC_INA_ALERT },
        },
    },
    .ec_current_sensor_3p3v = {
        /* 3.3V Power Sensor */
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C7,
                .slave_addr = BMS_EC_CURRENT_SENSOR_3P3V_ADDR,
            },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_GBC_INA_ALERT },
        },
    },
    .ec_temp_sensor = {
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C1,
                .slave_addr = BMS_EC_TEMP_SENSOR_ADDR
            },
            .pin_evt = &pin_tempsen_evt2,
        },
        .obj = {},
    },
};

Hci_Cfg g_hci_cfg = {
    .buzzer = {
        .pin_en = { &gbc_io_0, 10, OCGPIO_CFG_OUT_OD_NOPULL },
    },
    .led = {
        .temp_sensor = {
            .cfg = {
                .dev = {
                    .bus = OC_CONNECT1_I2C8,
                    .slave_addr = HCI_LED_TEMP_SENSOR_ADDR
                },
                .pin_evt = NULL,
            },
            .obj = {},
        },
        .sx1509_dev[HCI_LED_DRIVER_LEFT] = {
            .bus = OC_CONNECT1_I2C8,
            .slave_addr = LED_SX1509_LEFT_ADDRESS,
        },
        .sx1509_dev[HCI_LED_DRIVER_RIGHT] = {
            .bus = OC_CONNECT1_I2C8,
            .slave_addr = LED_SX1509_RIGHT_ADDRESS,
        },
        /* EC_GPIO */
        .pin_ec_gpio = { &ec_io, OC_EC_HCI_LED_RESET },
    },
};

Gpp_Cfg g_gpp_cfg = {
    .ap = {
        .current_sensor = {
            .cfg = {
                .dev = {
                    .bus = OC_CONNECT1_I2C6,
                    .slave_addr = GPP_AP_CURRENT_SENSOR_ADDR,
                },
                .pin_alert = &(OcGpio_Pin){ &ec_io,
                             OC_EC_GBC_AP_INA_ALERT },
            },
        },
        .temp_sensor = {
           {
                .cfg = {
                    .dev = {
                        .bus = OC_CONNECT1_I2C1,
                        .slave_addr = GPP_AP_TEMPSENS1_ADDR
                    },
                    .pin_evt = &pin_tempsen_evt3,
                },
                .obj = {},
            },
           {
                .cfg = {
                    .dev = {
                        .bus = OC_CONNECT1_I2C1,
                        .slave_addr = GPP_AP_TEMPSENS2_ADDR
                    },
                    .pin_evt = &pin_tempsen_evt5,
                },
                .obj = {},
            },
          {
                .cfg = {
                    .dev = {
                        .bus = OC_CONNECT1_I2C1,
                        .slave_addr = GPP_AP_TEMPSENS3_ADDR
                    },
                    .pin_evt = &pin_tempsen_evt4,
                },
                .obj = {},
            },
        },
    },
    .msata = {
        .current_sensor = {
            .cfg = {
                .dev = {
                    .bus = OC_CONNECT1_I2C6,
                    .slave_addr = GPP_MSATA_CURRENT_SENSOR_ADDR,
                },
                .pin_alert = &(OcGpio_Pin){ &ec_io,
                                            OC_EC_GBC_AP_INA_ALERT },
            },
        },
    },

    /* SOC_PLTRST_N */
    .pin_soc_pltrst_n     = { &ec_io, OC_EC_GPP_SOC_PLTRST },
    /* TIVA_SOC_GPIO2 */
    .pin_ap_boot_alert1   = { &ec_io, OC_EC_GPP_AP_BM_1 },
    /* TIVA_SOC_GPIO3 */
    .pin_ap_boot_alert2   = { &ec_io, OC_EC_GPP_AP_BM_2 },
    /* SOC_COREPWROK */
    .pin_soc_corepwr_ok   = { &ec_io, OC_EC_GPP_PMIC_CORE_PWR },
    /* MSATA_EC_DAS */
    .pin_msata_ec_das     = { &ec_io, OC_EC_GPP_MSATA_DAS },
    /* LT4256_EC_PWRGD */
    .pin_lt4256_ec_pwrgd  = { &ec_io, OC_EC_GPP_PWRGD_PROTECTION },
    /* AP_12V_ONOFF */
    .pin_ap_12v_onoff     = { &ec_io, OC_EC_GPP_PMIC_CTRL },
    /* EC_RESET_TO_PROC */
    .pin_ec_reset_to_proc = { &ec_io, OC_EC_GPP_RST_TO_PROC },
};

Sdr_Cfg g_sdr_cfg = {
    .fpga = {
        .current_sensor = {
            .cfg = {
                .dev = {
                    .bus = OC_CONNECT1_I2C3,
                    .slave_addr = SDR_FPGA_CURRENT_SENSOR_ADDR,
                },
                .pin_alert = &(OcGpio_Pin){ &ec_io,
                                            OC_EC_SDR_FPGA_TEMP_INA_ALERT },
            },
        },
        .temp_sensor = {
            .bus = OC_CONNECT1_I2C3,
            .slave_addr = SDR_FPGA_TEMP_SENSOR_ADDR
        },
    },
    .eeprom_inventory = &eeprom_sdr_inv,
    .current_sensor = {
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C6,
                .slave_addr = SDR_CURRENT_SENSOR_ADDR,
            },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_SDR_INA_ALERT },
        },
    },
    /* FX3_RESET */
    .pin_fx3_reset       = { &sdr_fx3_io, 0 },
    /* EC_TRXFECONN_GPIO2/SDR_REG_LDO_PGOOD */
    .pin_sdr_reg_ldo_pgood  = { &ec_io, OC_EC_SDR_PWR_GD },
    /* TRXFE_12V_ONOFF */
    .pin_trxfe_12v_onoff    = { &ec_io, OC_EC_SDR_PWR_CNTRL },
    /* EC_FE_RESET_OUT/RF_FE_IO_RESET */
    .pin_rf_fe_io_reset     = { &ec_io, OC_EC_SDR_FE_IO_RESET_CTRL },
    /* EC_TRXFECONN_GPIO1/SDR_RESET_IN */
    .pin_sdr_reset_in       = { &ec_io, OC_EC_SDR_DEVICE_CONTROL },
    /* EC_TRXFE_RESET */
    .pin_ec_trxfe_reset     = { &ec_io, OC_EC_RFFE_RESET },
    /* FX3_RESET */
    .pin_fx3_reset          = { &sdr_fx3_io, 0 },
};

Fe_Cfg g_fe_cfg = {
    /* EC_TRXFECONN_GPIO3/RF_PGOOD_LDO */
    .pin_rf_pgood_ldo         = { &ec_io, OC_EC_FE_PWR_GD },
    /* FE_12V_CTRL */
    .pin_fe_12v_ctrl          = { &ec_io, OC_EC_FE_CONTROL },
    .fe_ch1_gain_cfg = {
        .fe_gain_cfg = {
            /* CH1_TX_ATTN_16DB */
            .pin_tx_attn_16db = { &fe_ch1_gain_io, 1 },
            /* CH1_TX_ATTN_P5DB */
            .pin_tx_attn_p5db = { &fe_ch1_gain_io, 2 },
            /* CH1_TX_ATTN_1DB */
            .pin_tx_attn_1db  = { &fe_ch1_gain_io, 3 },
            /* CH1_TX_ATTN_2DB */
            .pin_tx_attn_2db  = { &fe_ch1_gain_io, 4 },
            /* CH1_TX_ATTN_4DB */
            .pin_tx_attn_4db  = { &fe_ch1_gain_io, 5 },
            /* CH1_TX_ATTN_8DB */
            .pin_tx_attn_8db  = { &fe_ch1_gain_io, 6 },
            /* CH1_TX_ATTN_ENB */
            .pin_tx_attn_enb  = { &fe_ch1_gain_io, 7 },
        },
    },
    .fe_ch2_gain_cfg = {
        /* CH1_2G_LB_BAND_SEL_L */
        .pin_ch1_2g_lb_band_sel_l = { &fe_ch2_gain_io, 0 },
        .fe_gain_cfg = {
            /* CH2_TX_ATTN_16DB */
            .pin_tx_attn_16db = { &fe_ch2_gain_io, 1 },
            /* CH2_TX_ATTN_P5DB */
            .pin_tx_attn_p5db = { &fe_ch2_gain_io, 2 },
            /* CH2_TX_ATTN_1DB */
            .pin_tx_attn_1db  = { &fe_ch2_gain_io, 3 },
            /* CH2_TX_ATTN_2DB */
            .pin_tx_attn_2db  = { &fe_ch2_gain_io, 4 },
            /* CH2_TX_ATTN_4DB */
            .pin_tx_attn_4db  = { &fe_ch2_gain_io, 5 },
            /* CH2_TX_ATTN_8DB */
            .pin_tx_attn_8db  = { &fe_ch2_gain_io, 6 },
            /* CH2_TX_ATTN_ENB */
            .pin_tx_attn_enb  = { &fe_ch2_gain_io, 7 },
        },
    },
    .fe_ch1_lna_cfg = {
        .fe_lna_cfg = {
            /* CH1_RX_ATTN_P5DB */
            .pin_rx_attn_p5db = { &fe_ch1_lna_io, 2 },
            /* CH1_RX_ATTN_1DB */
            .pin_rx_attn_1db  = { &fe_ch1_lna_io, 3 },
            /* CH1_RX_ATTN_2DB */
            .pin_rx_attn_2db  = { &fe_ch1_lna_io, 4 },
            /* CH1_RX_ATTN_4DB */
            .pin_rx_attn_4db  = { &fe_ch1_lna_io, 5 },
            /* CH1_RX_ATTN_8DB */
            .pin_rx_attn_8db  = { &fe_ch1_lna_io, 6 },
            /* CH1_RX_ATTN_ENB */
            .pin_rx_attn_enb  = { &fe_ch1_lna_io, 7 },
        },
    },
    .fe_ch2_lna_cfg = {
        /* CH1_RF_PWR_OFF */
        .pin_ch1_rf_pwr_off = { &fe_ch2_lna_io, 1 },
        .fe_lna_cfg = {
            /* CH2_RX_ATTN_P5DB */
            .pin_rx_attn_p5db = { &fe_ch2_lna_io, 2 },
            /* CH2_RX_ATTN_1DB */
            .pin_rx_attn_1db  = { &fe_ch2_lna_io, 3 },
            /* CH2_RX_ATTN_2DB */
            .pin_rx_attn_2db  = { &fe_ch2_lna_io, 4 },
            /* CH2_RX_ATTN_4DB */
            .pin_rx_attn_4db  = { &fe_ch2_lna_io, 5 },
            /* CH2_RX_ATTN_8DB */
            .pin_rx_attn_8db  = { &fe_ch2_lna_io, 6 },
            /* CH2_RX_ATTN_ENB */
            .pin_rx_attn_enb  = { &fe_ch2_lna_io, 7 },
        },
    },
    .pin_trxfe_conn_reset = { &ec_io, OC_EC_FE_TRXFE_CONN_RESET },
    .fe_watchdog_cfg = {
        /* AOSEL_FPGA */
        .pin_aosel_fpga     = { &fe_watchdog_io, 0 },
        /* CH2_RF_PWR_OFF */
        .pin_ch2_rf_pwr_off = { &fe_watchdog_io, 1 },
        /* CO6_WD */
        .pin_co6_wd         = { &fe_watchdog_io, 2 },
        /* CO5_WD */
        .pin_co5_wd         = { &fe_watchdog_io, 3 },
        /* CO4_WD */
        .pin_co4_wd         = { &fe_watchdog_io, 4 },
        /* CO3_WD */
        .pin_co3_wd         = { &fe_watchdog_io, 5 },
        /* CO2_WD */
        .pin_co2_wd         = { &fe_watchdog_io, 6 },
        /* COPOL_FPGA */
        .pin_copol_fpga     = { &fe_watchdog_io, 7 },
    },
    .ina226_ch1_5_7v = {
        /* CH1 5.7V Sensor */
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C4,
                .slave_addr = RFFE_INA226_CH1_5_7V_ADDR,
            },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_RFFE_TEMP_INA_ALERT },
        },
    },
    .ina226_ch2_5_7v = {
        /* CH2 5.7V Sensor */
        .cfg = {
            .dev = {
                .bus = OC_CONNECT1_I2C4,
                .slave_addr = RFFE_INA226_CH2_5_7V_ADDR,
            },
            .pin_alert = &(OcGpio_Pin){ &ec_io,
                                        OC_EC_RFFE_TEMP_INA_ALERT },
        },
    },
    .adt7481_ch1 = {
        .bus = OC_CONNECT1_I2C4,
        .slave_addr = RFFE_CH1_TEMP_SENSOR_ADDR,
    },
    .adt7481_ch2 = {
        .bus = OC_CONNECT1_I2C4,
        .slave_addr = RFFE_CH2_TEMP_SENSOR_ADDR,
    },
    .eeprom_inventory = &eeprom_fe_inv,
    .ads7830_ch1 = {
        .bus = OC_CONNECT1_I2C4,
        .slave_addr = RFFE_CHANNEL1_ADC_ADDR,
    },
    .ads7830_ch2 = {
        .bus = OC_CONNECT1_I2C4,
        .slave_addr = RFFE_CHANNEL2_ADC_ADDR,
    },
};

Sync_Cfg g_sync_cfg = {
    .temp_sens = {
        .bus = OC_CONNECT1_I2C7,
        .slave_addr = SYNC_TEMP_SENSOR_ADDR,
    },
    .io_exp = {
        .bus = OC_CONNECT1_I2C7,
        .slave_addr = SYNC_IO_DEVICE_ADDR,
    },
    /* SPDT_CNTRL_LVL */
    .pin_spdt_cntrl_lvl             = { &sync_io, 0, OCGPIO_CFG_OUT_OD_NOPULL },
    /* WARMUP_SURVEY_INIT_SEL */
    .pin_warmup_survey_init_sel     = { &sync_io, 1, OCGPIO_CFG_OUT_OD_NOPULL },
    /* R_PHASE_LOCK_IOEXP */
    .pin_r_phase_lock_ioexp         = { &sync_io, 4, OCGPIO_CFG_IN_PU },
    /* R_LOCK_OK_IOEXP */
    .pin_r_lock_ok_ioexp            = { &sync_io, 5, OCGPIO_CFG_IN_PU },
    /* R_ALARM_IOEXP */
    .pin_r_alarm_ioexp              = { &sync_io, 6, OCGPIO_CFG_IN_PU },
    /* 12V_REG_ENB */
    .pin_12v_reg_enb                = { &sync_io, 7, OCGPIO_CFG_OUT_STD },
    /* TEMP_ALERT */
    .pin_temp_alert                 = { &sync_io, 8, OCGPIO_CFG_IN_PU },
    /* SPDT_CNTRL_LTE_CPU_GPS_LVL */
    .pin_spdt_cntrl_lte_cpu_gps_lvl = { &sync_io, 9, OCGPIO_CFG_OUT_OD_NOPULL },
    /* INIT_SURVEY_SEL */
    .pin_init_survey_sel            = { &sync_io, 10, OCGPIO_CFG_OUT_OD_NOPULL },
    /* EC_SYNC_RESET */
    .pin_ec_sync_reset              = { &ec_io, OC_EC_SYNC_RESET },
};

Obc_Cfg g_obc_cfg = {
    .iridium_cfg = {
        .uart = OC_CONNECT1_UARTXR0,
        /* IRIDIUM_RSTIOEXP */
        .pin_enable         = { &sync_io, 2, OCGPIO_CFG_OUT_STD },
        /* R_NW_AVAIL */
        .pin_nw_avail       = { &sync_io, 3, OCGPIO_CFG_IN_PU },
    },
    /* 12V_REG_ENB */
    .pin_pwr_en = &(OcGpio_Pin){ &sync_io, 7, OCGPIO_CFG_OUT_STD },
};

Eth_Sw_Cfg g_eth_cfg = {
    .pin_evt = NULL,
    .pin_ec_ethsw_reset = { &ec_io, OC_EC_ETH_SW_RESET },
    .eth_switch = {},
};

void *sys_config[] = {
    [OC_SS_SYS] = NULL,
    [OC_SS_PWR] = &g_power_cfg,
    [OC_SS_BMS] = &g_bms_cfg,
    [OC_SS_HCI] = &g_hci_cfg,
    [OC_SS_ETH_SWT] = &g_eth_cfg,
    [OC_SS_OBC] = &g_obc_cfg,
    [OC_SS_GPP] = &g_gpp_cfg,
    [OC_SS_SDR] = &g_sdr_cfg,
    [OC_SS_RF] = &g_fe_cfg,
    [OC_SS_SYNC] = &g_sync_cfg,
    [OC_SS_TEST_MODULE] = &(TestMod_Cfg){
        .g510_cfg = {
            .uart = OC_CONNECT1_UART4,
            /* 2G_SIM_PRESENCE */
            .pin_sim_present = { &gbc_io_1, 0, OCGPIO_CFG_IN_PU },

            /* NOTE: enable & power go through MOSFETs, inverting them */
            /* 2GMODULE_POWEROFF  */
            .pin_enable = { &gbc_io_1, 2, OCGPIO_CFG_INVERT },
            /* EC_2GMODULE_PWR_ON  */
            .pin_pwr_en = { &gbc_io_1, 1, OCGPIO_CFG_INVERT },
        },
        .pin_ant_sw = {},
    },
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
    [OC_CONNECT1_I2C6] = {
        .baseAddr = I2C6_BASE,
        .intNum = INT_I2C6,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C7] = {
        .baseAddr = I2C7_BASE,
        .intNum = INT_I2C7,
        .intPriority = (~0)
    },
    [OC_CONNECT1_I2C8] = {
        .baseAddr = I2C8_BASE,
        .intNum = INT_I2C8,
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
    [OC_CONNECT1_I2C6] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C6],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C6]
    },
    [OC_CONNECT1_I2C7] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C7],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C7]
    },
    [OC_CONNECT1_I2C8] = {
        .fxnTablePtr = &I2CTiva_fxnTable,
        .object = &i2cTivaObjects[OC_CONNECT1_I2C8],
        .hwAttrs = &i2cTivaHWAttrs[OC_CONNECT1_I2C8]
    },
    { NULL, NULL, NULL }
};

/*
 *  ======== OC_CONNECT1_initI2C ========
 */
void OC_CONNECT1_initI2C(void)
{
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
    GPIOPinConfigure(GPIO_PG0_I2C1SCL);
    GPIOPinConfigure(GPIO_PG1_I2C1SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTG_BASE, GPIO_PIN_0);
    GPIOPinTypeI2C(GPIO_PORTG_BASE, GPIO_PIN_1);

    /* I2C2 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PL0_I2C2SDA);
    GPIOPinConfigure(GPIO_PL1_I2C2SCL);
    GPIOPinTypeI2CSCL(GPIO_PORTL_BASE, GPIO_PIN_1);
    GPIOPinTypeI2C(GPIO_PORTL_BASE, GPIO_PIN_0);

    /* I2C3 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PK4_I2C3SCL);
    GPIOPinConfigure(GPIO_PK5_I2C3SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTK_BASE, GPIO_PIN_4);
    GPIOPinTypeI2C(GPIO_PORTK_BASE, GPIO_PIN_5);

    /* I2C4 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C4);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PK6_I2C4SCL);
    GPIOPinConfigure(GPIO_PK7_I2C4SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTK_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTK_BASE, GPIO_PIN_7);

    /* I2C6 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C6);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PA6_I2C6SCL);
    GPIOPinConfigure(GPIO_PA7_I2C6SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    /* I2C7 Init */
    /*
     * NOTE: TI-RTOS examples configure pins PA4 & PA5 for SSI0 or I2C7.  Thus,
     * a conflict occurs when the I2C & SPI drivers are used simultaneously in
     * an application.  Modify the pin mux settings in this file and resolve the
     * conflict before running your the application.
     */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PD0_I2C7SCL);
    GPIOPinConfigure(GPIO_PD1_I2C7SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

    /* I2C8 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C8);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PA2_I2C8SCL);
    GPIOPinConfigure(GPIO_PA3_I2C8SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_3);

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
#if TI_DRIVERS_UART_DMA
#include <ti/drivers/uart/UARTTivaDMA.h>

UARTTivaDMA_Object uartTivaObjects[OC_CONNECT1_UARTCOUNT];

const UARTTivaDMA_HWAttrs uartTivaHWAttrs[OC_CONNECT1_UARTCOUNT] = {
    [OC_CONNECT1_UART3] = {
        .baseAddr = UART0_BASE,
        .intNum = INT_UART0,
        .intPriority = (~0),
        .rxChannelIndex = UDMA_CH8_UART0RX,
        .txChannelIndex = UDMA_CH9_UART0TX,
    }
};

const UART_Config UART_config[] = {
    [OC_CONNECT1_UART3] = {
        .fxnTablePtr = &UARTTivaDMA_fxnTable,
        .object = &uartTivaObjects[0],
        .hwAttrs = &uartTivaHWAttrs[0]
    },
    { NULL, NULL, NULL }
};
#else
#include <ti/drivers/uart/UARTTiva.h>
#include "devices/i2c/XR20M1170.h" // TODO: is devices the right directory? also, is it confusing to have this in i2c?
#include "devices/uart/UartMon.h"

UARTTiva_Object uartTivaObjects[3];
unsigned char uartTivaRingBuffer[3][64];

XR20M1170_Object XR20M1170Objects;
unsigned char XR20M1170RingBuffer[1][64];

// TODO: probably more efficient to use DMA drivers
const UARTTiva_HWAttrs uartTivaHWAttrs[] = {
    {
        .baseAddr = UART0_BASE,
        .intNum = INT_UART0,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[0],
        .ringBufSize = sizeof(uartTivaRingBuffer[0])
    },
    {
        .baseAddr = UART3_BASE,
        .intNum = INT_UART3,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE,
        .ringBufPtr  = uartTivaRingBuffer[1],
        .ringBufSize = sizeof(uartTivaRingBuffer[1])
    },
    {
        .baseAddr = UART4_BASE,
        .intNum = INT_UART4,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_RX | UART_FLOWCONTROL_TX,
        .ringBufPtr  = uartTivaRingBuffer[2],
        .ringBufSize = sizeof(uartTivaRingBuffer[2])
    },
};

// TODO: flow control settings
const XR20M1170_HWAttrs XR20M1170HWAttrs = {
    /*.baseAddr = UART3_BASE,
        .intNum = INT_UART3,
        .intPriority = (~0),
        .flowControl = UART_FLOWCONTROL_NONE, */
    // i2c port
    // i2c bit rate
    // uart interrupt
    .i2cIndex = OC_CONNECT1_I2C7,
    .i2cSlaveAddress = 0x60 >> 1, // TODO: i2c driver uses 7-bit address...sort of annoying
    .xtal1_freq = 14745600, // 14.7456 MHz
    .pin_irq = &(OcGpio_Pin){ &gbc_io_0, 0, OCGPIO_CFG_IN_PU },
    .flowControl = XR20M1170_FLOWCONTROL_TX | XR20M1170_FLOWCONTROL_RX,
    .ringBufPtr  = XR20M1170RingBuffer[0],
    .ringBufSize = sizeof(XR20M1170RingBuffer[0]),
};

UartMon_Object uart_mon_obj;
const UartMon_Cfg uart_mon_cfg = {
    .uart_in_idx = OC_CONNECT1_UARTXR0,
    .uart_debug_idx = OC_CONNECT1_UART0,
};

const UART_Config UART_config[OC_CONNECT1_UARTCOUNT + 1] = {
    [OC_CONNECT1_UART0] = {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[0],
        .hwAttrs = &uartTivaHWAttrs[0]
    },
    [OC_CONNECT1_UART3] = {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[1],
        .hwAttrs = &uartTivaHWAttrs[1]
    },
    [OC_CONNECT1_UART4] = {
        .fxnTablePtr = &UARTTiva_fxnTable,
        .object = &uartTivaObjects[2],
        .hwAttrs = &uartTivaHWAttrs[2]
    },
    [OC_CONNECT1_UARTXR0] = {
        .fxnTablePtr = &XR20M1170_fxnTable,
        .object = &XR20M1170Objects,
        .hwAttrs = &XR20M1170HWAttrs
    },
    [OC_CONNECT1_UARTMON] = {
        .fxnTablePtr = &UartMon_fxnTable,
        .object = &uart_mon_obj,
        .hwAttrs = &uart_mon_cfg,
    },
    { NULL, NULL, NULL }
};

/*
 *  ======== OC_CONNECT1_initUART ========
 */
void OC_CONNECT1_initUART(void)
{
    // Debug UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // AP UART
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    //GPIOPinConfigure(GPIO_PA4_U3RX);
    //GPIOPinConfigure(GPIO_PA5_U3TX);
    //GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // XR20M1170 IRQ pin
    //GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5);

    // GSM Module UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    GPIOPinConfigure(GPIO_PK0_U4RX);
    GPIOPinConfigure(GPIO_PK1_U4TX);
    GPIOPinConfigure(GPIO_PK2_U4RTS);
    GPIOPinConfigure(GPIO_PK3_U4CTS);
    GPIOPinTypeUART(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                    GPIO_PIN_3);

    /* Initialize the UART driver */
#if TI_DRIVERS_UART_DMA
    OC_CONNECT1_initDMA();
#endif
    UART_init();
}
#endif /* TI_DRIVERS_UART_DMA */

/*
 *  =============================== USB ===============================
 */

/*
 *  ======== OC_CONNECT1_usbBusFaultHwi ========
 */
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
