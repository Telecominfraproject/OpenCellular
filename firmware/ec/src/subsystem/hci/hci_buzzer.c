/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/subsystem/hci/hci_buzzer.h"

#include "inc/subsystem/hci/hci.h"

extern HciBuzzer_Cfg gbc_hci_buzzer;
#define HCI gbc_hci_buzzer

ReturnStatus HciBuzzer_init(void)
{
    if (OcGpio_configure(&HCI.pin_en, OCGPIO_CFG_OUTPUT) < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }
    return RETURN_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : hci_buzzer_beep
 **
 **    DESCRIPTION     : Turn off or Turn on Buzzer.
 **
 **    ARGUMENTS       : Buzzer Count.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus hci_buzzer_beep(uint8_t buzzCount)
{
    for (uint8_t count = 0; count < buzzCount; count++) {
        if (OcGpio_write(&HCI.pin_en, true) < OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }
        if (OcGpio_write(&HCI.pin_en, false) < OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }
    }
    return RETURN_OK;
}
