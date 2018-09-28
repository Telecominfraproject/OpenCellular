/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _HCI_BUZZER_H
#define _HCI_BUZZER_H

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"

#include <stdint.h>

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct HciBuzzer_Cfg {
    OcGpio_Pin pin_en;
} HciBuzzer_Cfg;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus HciBuzzer_init(void);
ReturnStatus hci_buzzer_beep(uint8_t buzzCount);

#endif /* _HCI_BUZZER_H */
