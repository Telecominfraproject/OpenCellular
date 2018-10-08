/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/devices/led.h"

#include "common/inc/global/post_frame.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/hci/hci.h"

#include <ti/sysbios/knl/Task.h>

/*****************************************************************************
 *                          REGISTER DEFINITIONS
 *****************************************************************************/

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS
 *****************************************************************************/

/* List of LEDS on the LED board.
 ----------------------------------------------------------
 |  Board Left LEDs         |          Board Right LEDs |
 ----------------------------------------------------------
 IO0  - D19 - Red                   IO0  - D24 - Red
 IO1  - D19 - Green                 IO1  - D24 - Green
 IO2  - D17 - Red                   IO2  - D31 - Red
 IO3  - D17 - Green                 IO3  - D31 - Green
 IO4  - D8  - Red                   IO4  - D23 - Red
 IO5  - D8  - Green                 IO5  - D23 - Green
 IO6  - D20 - Red                   IO6  - D25 - Red
 IO7  - D20 - Green                 IO7  - D25 - Green
 IO8  - D21 - Red                   IO8  - D26 - Red
 IO9  - D21 - Green                 IO9  - D26 - Green
 IO10 - D22 - Red                   IO10 - D27 - Red
 IO11 - D22 - Green                 IO11 - D27 - Green
 IO12 - D18 - Red                   IO12 - D28 - Red
 IO13 - D18 - Green                 IO13 - D28 - Green
 */

/* LED arrangements
 Left:   D8 => D17 => D18 => D19 => D20 => D21 => D22 ( From Left Bottom to Top Centre)
 Right: D23 => D31 => D24 => D25 => D26 => D27 => D28 ( From Top centre to Right Bottom)

 Green from D8 to D28:
 Left:  IO5 => IO3 => IO13 => IO1 => IO7 => IO9  => IO11
 Right: IO5 => IO3 => IO1  => IO7 => IO9 => IO11 => IO13

 Red from D8 to D28:
 Left:  IO4 => IO2 => IO12 => IO0 => IO6 => IO8  => IO10
 Right: IO4 => IO2 => IO0  => IO6 => IO8 => IO10 => IO12

 Bank A: IO0 => IO7  ( IO7, IO6, IO5, IO4, IO3, IO2, IO1, IO0)
 Bank B: IO8 => IO15 ( IO15, IO14, IO13, IO12, IO11, IO10, IO9, IO8)

 IO14 & IO15 = 1(Not connected IOs; Reg Data should be 11xxxxxx).
 */
static const hciLedData ledData[HCI_LED_TOTAL_NOS] =
        { [HCI_LED_1] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_5, // IO5
                          .ledRed = ~SX1509_IO_PIN_4, // IO4
                          .ledOff = SX1509_IO_PIN_5 | SX1509_IO_PIN_4,
                  },
          [HCI_LED_2] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_3, // IO3
                          .ledRed = ~SX1509_IO_PIN_2, // IO2
                          .ledOff = SX1509_IO_PIN_3 | SX1509_IO_PIN_2,
                  },
          [HCI_LED_3] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_B,
                          .ledGreen = ~SX1509_IO_PIN_13, // IO13
                          .ledRed = ~SX1509_IO_PIN_12, // IO12
                          .ledOff = SX1509_IO_PIN_13 | SX1509_IO_PIN_12,
                  },
          [HCI_LED_4] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_1, // IO1
                          .ledRed = ~SX1509_IO_PIN_0, // IO0
                          .ledOff = SX1509_IO_PIN_1 | SX1509_IO_PIN_0,
                  },
          [HCI_LED_5] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_7, // IO7
                          .ledRed = ~SX1509_IO_PIN_6, // IO6
                          .ledOff = SX1509_IO_PIN_7 | SX1509_IO_PIN_6,
                  },
          [HCI_LED_6] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_B,
                          .ledGreen = ~SX1509_IO_PIN_9, // IO9
                          .ledRed = ~SX1509_IO_PIN_8, // IO8
                          .ledOff = SX1509_IO_PIN_9 | SX1509_IO_PIN_8,
                  },
          [HCI_LED_7] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_LEFT,
                          .ledReg = SX1509_REG_B,
                          .ledGreen = ~SX1509_IO_PIN_11, // IO11
                          .ledRed = ~SX1509_IO_PIN_10, // I010
                          .ledOff = SX1509_IO_PIN_11 | SX1509_IO_PIN_10,
                  },
          [HCI_LED_8] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_RIGHT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_5, // IO5
                          .ledRed = ~SX1509_IO_PIN_4, // IO4
                          .ledOff = SX1509_IO_PIN_5 | SX1509_IO_PIN_4,
                  },
          [HCI_LED_9] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_RIGHT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_3, // IO3
                          .ledRed = ~SX1509_IO_PIN_2, // IO2
                          .ledOff = SX1509_IO_PIN_3 | SX1509_IO_PIN_2,
                  },
          [HCI_LED_10] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_RIGHT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_1, // IO1
                          .ledRed = ~SX1509_IO_PIN_0, // IO0
                          .ledOff = SX1509_IO_PIN_1 | SX1509_IO_PIN_0,
                  },
          [HCI_LED_11] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_RIGHT,
                          .ledReg = SX1509_REG_A,
                          .ledGreen = ~SX1509_IO_PIN_7, // IO7
                          .ledRed = ~SX1509_IO_PIN_6, // IO6
                          .ledOff = SX1509_IO_PIN_7 | SX1509_IO_PIN_6,
                  },
          [HCI_LED_12] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_RIGHT,
                          .ledReg = SX1509_REG_B,
                          .ledGreen = ~SX1509_IO_PIN_9, // IO9
                          .ledRed = ~SX1509_IO_PIN_8, // IO8
                          .ledOff = SX1509_IO_PIN_9 | SX1509_IO_PIN_8,
                  },
          [HCI_LED_13] =
                  {
                          .ioexpDev = HCI_LED_DRIVER_RIGHT,
                          .ledReg = SX1509_REG_B,
                          .ledGreen = ~SX1509_IO_PIN_11, // IO11
                          .ledRed = ~SX1509_IO_PIN_10, // IO10
                          .ledOff = SX1509_IO_PIN_11 | SX1509_IO_PIN_10,
                  },
          [HCI_LED_14] = {
                  .ioexpDev = HCI_LED_DRIVER_RIGHT,
                  .ledReg = SX1509_REG_B,
                  .ledGreen = ~SX1509_IO_PIN_13, // IO13
                  .ledRed = ~SX1509_IO_PIN_12, // IO12
                  .ledOff = SX1509_IO_PIN_13 | SX1509_IO_PIN_12,
          } };

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_turnon_green
 **
 **    DESCRIPTION     : Turn On Green LEDs on the LED board.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_turnon_green(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn On Left side Green LEDs */
    status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_LEFT],
                                SX1509_REG_AB, 0x55, 0x55);
    if (status == RETURN_OK) {
        /* Turn On Right side Green LEDs */
        status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_RIGHT],
                                    SX1509_REG_AB, 0x55, 0x55);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_turnon_red
 **
 **    DESCRIPTION     : Turn On Red LEDs on the LED board.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_turnon_red(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn On Left side Red LEDs */
    status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_LEFT],
                                SX1509_REG_AB, 0xAA, 0xAA);
    if (status == RETURN_OK) {
        /* Turn On Right side Red LEDs */
        status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_RIGHT],
                                    SX1509_REG_AB, 0xAA, 0xAA);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_turnoff_all
 **
 **    DESCRIPTION     : Turn Off All LEDs on the LED board.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_turnoff_all(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn Off Left side LEDs */
    status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_LEFT],
                                SX1509_REG_AB, LED_OFF, LED_OFF);
    if (status == RETURN_OK) {
        /* Turn Off Right side LEDs */
        status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_RIGHT],
                                    SX1509_REG_AB, LED_OFF, LED_OFF);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_configure_sx1509_onofftime
 **
 **    DESCRIPTION     : Configure On and Off time of LEDs on the LED board.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static ReturnStatus hci_led_configure_sx1509_onofftime(const I2C_Dev *ioexpDev)
{
    ReturnStatus status = RETURN_OK;
    uint8_t index;

    for (index = 0; index < 14; index++) {
        /* Configure RegTOn time of LEDs */
        status = ioexp_led_set_on_time(ioexpDev, index, REG_T_ON_VALUE);
        if (status != RETURN_OK) {
            break;
        } else {
            /* Configure RegOff time of LEDs */
            status = ioexp_led_set_off_time(ioexpDev, index, REG_OFF_VALUE);
            if (status != RETURN_OK) {
                break;
            }
        }
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_configure_onofftime
 **
 **    DESCRIPTION     : Configure On and Off time of LEDs on the LED board.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static ReturnStatus hci_led_configure_onofftime(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Configure LED driver parameters(RegTOn, RegOff) for Left side LEDs */
    status = hci_led_configure_sx1509_onofftime(
            &driver->sx1509_dev[HCI_LED_DRIVER_LEFT]);
    if (status == RETURN_OK) {
        /* Configure LED driver parameters(RegTOn, RegOff) for Right side LEDs */
        hci_led_configure_sx1509_onofftime(
                &driver->sx1509_dev[HCI_LED_DRIVER_RIGHT]);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_system_boot
 **
 **    DESCRIPTION     : Make all LEDs glow in green in circulating to indicate
 **                      system is booting Up.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_system_boot(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;
    uint8_t index = 0;
    uint8_t regValue = 0;

    /* Turn off all LEDs */
    status = hci_led_turnoff_all(driver);
    if (status != RETURN_OK) {
        return status;
    }
    /* Turn on the LEDs one by one from Left to Right of LED Board */
    for (index = 0; index < HCI_LED_TOTAL_NOS; index++) {
        status =
                ioexp_led_get_data(&driver->sx1509_dev[ledData[index].ioexpDev],
                                   ledData[index].ledReg, &regValue);

        regValue &= ledData[index].ledGreen;

        status =
                ioexp_led_set_data(&driver->sx1509_dev[ledData[index].ioexpDev],
                                   ledData[index].ledReg, regValue, 0);
        if (status != RETURN_OK) {
            break;
        }
        /* Put LED Task into sleep mode to allow other tasks to run */
        Task_sleep(100);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_system_running
 **
 **    DESCRIPTION     : Make all LEDs blink in green to indicate system is
 **                      running.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_system_running(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn off all LEDs */
    status = hci_led_turnoff_all(driver);
    if (status != RETURN_OK) {
        return status;
    }

    /* Configure on and off time of LED GPIO pins */
    status = hci_led_configure_onofftime(driver);
    if (status != RETURN_OK) {
        return status;
    }

    /* Turn on all the green LEDS */
    status = hci_led_turnon_green(driver);

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_system_failure
 **
 **    DESCRIPTION     : Make all LEDs blink in Red to indicate system is
 **                      Failure.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_system_failure(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn off all LEDs */
    status = hci_led_turnoff_all(driver);
    if (status != RETURN_OK) {
        return status;
    }

    /* Configure on and off time of LED GPIO pins */
    status = hci_led_configure_onofftime(driver);
    if (status != RETURN_OK) {
        return status;
    }

    /* Turn on all the Red LEDS */
    status = hci_led_turnon_red(driver);

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_radio_failure
 **
 **    DESCRIPTION     : Make Left side of LEDs blink in Red to indicate Radio
 **                      part is Failure.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_radio_failure(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn off all LEDs */
    status = hci_led_turnoff_all(driver);
    if (status == RETURN_OK) {
        /* Turn On Left side Red LEDs */
        status = ioexp_led_set_data(HCI_LED_DRIVER_LEFT, SX1509_REG_AB, 0xAA,
                                    0xAA);
    }

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_led_backhaul_failure
 **
 **    DESCRIPTION     : Make Right side of LEDs blink in Red to indicate
 **                      Backhaul is Failure.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus hci_led_backhaul_failure(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;

    /* Turn off all LEDs */
    status = hci_led_turnoff_all(driver);
    if (status == RETURN_OK) {
        /* Turn On Right side Red LEDs */
        status = ioexp_led_set_data(&driver->sx1509_dev[HCI_LED_DRIVER_RIGHT],
                                    SX1509_REG_AB, 0xAA, 0xAA);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : led_init
 **
 **    DESCRIPTION     : Initialize required LED registers to turn on the LED
 **                      driver.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus led_init(const HciLedCfg *driver)
{
    ReturnStatus status = RETURN_OK;
    uint8_t index;

    /* Steps required to use the LED driver
     - Disable input buffer (RegInputDisable)
     - Disable pull-up (RegPullUp)
     - Enable open drain (RegOpenDrain)
     - Set direction to output (RegDir) – by default RegData is set high => LED OFF
     - Enable oscillator (RegClock)
     - Configure LED driver clock and mode if relevant (RegMisc)
     - Enable LED driver operation (RegLEDDriverEnable)
     - Configure LED driver parameters (RegTOn, RegIOn, RegOff, RegTRise, RegTFall)
     - Set RegData bit low => LED driver started
     */

    /* Initilaize Left and Right LED driver SX1509 to turn on LED */
    for (index = 0; index < HCI_LED_DRIVER_COUNT; index++) {
        DEBUG("HCILED:INFO:: Initilaizing LED driver SX1509 0x%x.\n",
              driver->sx1509_dev[index].slave_addr);

        /* Do software reset for LED driver */
        status = ioexp_led_software_reset(&driver->sx1509_dev[index]);
        if (status != RETURN_OK) {
            return status;
        }

        /* Disable input buffer (RegInputDisable) */
        status = ioexp_led_config_inputbuffer(&driver->sx1509_dev[index],
                                              SX1509_REG_AB, 0xFF, 0xFF);
        if (status != RETURN_OK) {
            return status;
        }

        /* Disable pull-up (RegPullUp) */
        status = ioexp_led_config_pullup(&driver->sx1509_dev[index],
                                         SX1509_REG_AB, 0x00, 0x00);
        if (status != RETURN_OK) {
            return status;
        }

        /* Enable open drain (RegOpenDrain) */
        status = ioexp_led_config_opendrain(&driver->sx1509_dev[index],
                                            SX1509_REG_AB, 0xFF, 0xFF);
        if (status != RETURN_OK) {
            return status;
        }

        /* Set direction to output (RegDir) –
         * by default RegData is set high => LED OFF */
        status = ioexp_led_config_data_direction(&driver->sx1509_dev[index],
                                                 SX1509_REG_AB, 0x00, 0x00);
        if (status != RETURN_OK) {
            return status;
        }

        /* Configure internal clock oscillator frequency */
        status = ioexp_led_config_clock(&driver->sx1509_dev[index],
                                        SX1509_INTERNAL_CLOCK_2MHZ,
                                        SX1509_CLOCK_OSC_IN);
        if (status != RETURN_OK) {
            return status;
        }

        /* Configure LED driver clock and mode if relevant (RegMisc) */
        status = ioexp_led_config_misc(&driver->sx1509_dev[index],
                                       REG_MISC_VALUE);
        if (status != RETURN_OK) {
            return status;
        }

        /* Enable LED driver operation (RegLEDDriverEnable) */
        status = ioexp_led_enable_leddriver(&driver->sx1509_dev[index],
                                            SX1509_REG_AB, 0xFF, 0xFF);
        if (status != RETURN_OK) {
            return status;
        }

        DEBUG("HCILED:INFO:: LED driver SX1509 0x%x is Initialized.\n",
              driver->sx1509_dev[index].slave_addr);
    }
    return status;
}

void led_configure(HciLedCfg *driver)
{
    /* Initialize IO pins */
    OcGpio_configure(&driver->pin_ec_gpio,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
}
/*****************************************************************************
 **    FUNCTION NAME   : led_probe
 **
 **    DESCRIPTION     : Check LED module is present or not by reading Test
 **                      register of LED driver(SX1509) on LED board.
 **
 **    ARGUMENTS       : HCI Config driver and Postdata pointer.
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ePostCode led_probe(const HciLedCfg *driver, POSTData *postData)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    /* Read Test Register 1 of LED driver SX1509 of Left LED Module(RegTest1) */
    status = ioexp_led_read_testregister_1(
            &driver->sx1509_dev[HCI_LED_DRIVER_LEFT], &regValue);
    if (status != RETURN_OK) {
        return POST_DEV_MISSING;
    }

    /* Read Test Register 1 of LED driver SX1509 of Right LED Module(RegTest1) */
    status |= ioexp_led_read_testregister_1(
            &driver->sx1509_dev[HCI_LED_DRIVER_RIGHT], &regValue);
    if (status != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    post_update_POSTData(postData, &driver->sx1509_dev[HCI_LED_DRIVER_LEFT].bus,
                         &driver->sx1509_dev[HCI_LED_DRIVER_LEFT].slave_addr,
                         0xFF, 0xFF);
    return POST_DEV_FOUND;
}
