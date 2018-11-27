/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_sx1509.h"

uint8_t SX1509_regs[] = {
    [SX1509_REG_INPUT_DISABLE_B] =
        0x00, /* Input buffer disableSX1509_REGister B */
    [SX1509_REG_INPUT_DISABLE_A] =
        0x00, /* Input buffer disableSX1509_REGister A */
    [SX1509_REG_LONG_SLEW_B] =
        0x00, /* Output buffer long slewSX1509_REGister B */
    [SX1509_REG_LONG_SLEW_A] =
        0x00, /* Output buffer long slewSX1509_REGister A */
    [SX1509_REG_LOW_DRIVE_B] =
        0x00, /* Output buffer low driveSX1509_REGister B */
    [SX1509_REG_LOW_DRIVE_A] =
        0x00, /* Output buffer low driveSX1509_REGister A */
    [SX1509_REG_PULL_UP_B] = 0x00,        /* Pull UpSX1509_REGister B */
    [SX1509_REG_PULL_UP_A] = 0x00,        /* Pull UpSX1509_REGister A */
    [SX1509_REG_PULL_DOWN_B] = 0x00,      /* Pull DownSX1509_REGister B */
    [SX1509_REG_PULL_DOWN_A] = 0x00,      /* Pull DownSX1509_REGister A */
    [SX1509_REG_OPEN_DRAIN_B] = 0x00,     /* Open drainSX1509_REGister B */
    [SX1509_REG_OPEN_DRAIN_A] = 0x00,     /* Open drainSX1509_REGister A */
    [SX1509_REG_POLARITY_B] = 0x00,       /* PolaritySX1509_REGister B */
    [SX1509_REG_POLARITY_A] = 0x00,       /* PolaritySX1509_REGister A */
    [SX1509_REG_DIR_B] = 0x00,            /* DirectionSX1509_REGister B */
    [SX1509_REG_DIR_A] = 0x00,            /* DirectionSX1509_REGister A */
    [SX1509_REG_DATA_B] = 0x00,           /* DataSX1509_REGister B */
    [SX1509_REG_DATA_A] = 0x00,           /* DataSX1509_REGister A */
    [SX1509_REG_INTERRUPT_MASK_B] = 0x00, /* Interrupt maskSX1509_REGister B */
    [SX1509_REG_INTERRUPT_MASK_A] = 0x00, /* Interrupt maskSX1509_REGister A */
    [SX1509_REG_SENSE_HIGH_B] = 0x00,     /* Sense HighSX1509_REGister B */
    [SX1509_REG_SENSE_LOW_B] = 0x00,      /* Sense LowSX1509_REGister B */
    [SX1509_REG_SENSE_HIGH_A] = 0x00,     /* Sense HighSX1509_REGister A */
    [SX1509_REG_SENSE_LOW_A] = 0x00,      /* Sense LowSX1509_REGister A */
    [SX1509_REG_INTERRUPT_SOURCE_B] =
        0x00, /* Interrupt sourceSX1509_REGister B */
    [SX1509_REG_INTERRUPT_SOURCE_A] =
        0x00,                            /* Interrupt sourceSX1509_REGister A */
    [SX1509_REG_EVENT_STATUS_B] = 0x00,  /* Event statusSX1509_REGister B */
    [SX1509_REG_EVENT_STATUS_A] = 0x00,  /* Event statusSX1509_REGister A */
    [SX1509_REG_LEVEL_SHIFTER_1] = 0x00, /* Level shifterSX1509_REGister 1 */
    [SX1509_REG_LEVEL_SHIFTER_2] = 0x00, /* Level shifterSX1509_REGister 2 */
    [SX1509_REG_CLOCK] = 0x00,           /* Clock managementSX1509_REGister */
    [SX1509_REG_MISC] = 0x00, /* Miscellaneous device settingsSX1509_REGister */
    [SX1509_REG_LED_DRIVER_ENABLE_B] =
        0x00, /* LED driver enableSX1509_REGister B */
    [SX1509_REG_LED_DRIVER_ENABLE_A] =
        0x00, /* LED driver enableSX1509_REGister A */
    [SX1509_REG_DEBOUNCW_CONFIG] =
        0x00, /* Debounce configurationSX1509_REGister */
    [SX1509_REG_DEBOUNCW_ENABLE_B] =
        0x00, /* Debounce enableSX1509_REGister B */
    [SX1509_REG_DEBOUNCW_ENABLE_A] =
        0x00, /* Debounce enableSX1509_REGister A */
    [SX1509_REG_KEY_CONFIG_1] =
        0x00, /* Key scan configurationSX1509_REGister 1 */
    [SX1509_REG_KEY_CONFIG_2] =
        0x00, /* Key scan configurationSX1509_REGister 2 */
    [SX1509_REG_KEY_DATA_1] = 0x00, /* Key value (column) 1 */
    [SX1509_REG_KEY_DATA_2] = 0x00, /* Key value (row) 2 */
    [SX1509_REG_T_ON_0] = 0x00,     /* ON timeSX1509_REGister I/O[0] */
    [SX1509_REG_I_ON_0] = 0x00,     /* ON intensitySX1509_REGister I/O[0] */
    [SX1509_REG_OFF_0] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[0] */
    [SX1509_REG_T_ON_1] = 0x00,   /* ON timeSX1509_REGister I/O[1] */
    [SX1509_REG_I_ON_1] = 0x00,   /* ON intensitySX1509_REGister I/O[1] */
    [SX1509_REG_OFF_1] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[1] */
    [SX1509_REG_T_ON_2] = 0x00,   /* ON timeSX1509_REGister I/O[2] */
    [SX1509_REG_I_ON_2] = 0x00,   /* ON intensitySX1509_REGister I/O[2] */
    [SX1509_REG_OFF_2] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[2] */
    [SX1509_REG_T_ON_3] = 0x00,   /* ON timeSX1509_REGister I/O[3] */
    [SX1509_REG_I_ON_3] = 0x00,   /* ON intensitySX1509_REGister I/O[3] */
    [SX1509_REG_OFF_3] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[3] */
    [SX1509_REG_T_ON_4] = 0x00,   /* ON timeSX1509_REGister I/O[4] */
    [SX1509_REG_I_ON_4] = 0x00,   /* ON intensitySX1509_REGister I/O[4] */
    [SX1509_REG_OFF_4] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[4] */
    [SX1509_REG_T_RISE_4] = 0x00, /* Fade inSX1509_REGister I/O[4] */
    [SX1509_REG_T_Fall_4] = 0x00, /* Fade outSX1509_REGister I/O[4] */
    [SX1509_REG_T_ON_5] = 0x00,   /* ON timeSX1509_REGister I/O[5] */
    [SX1509_REG_I_ON_5] = 0x00,   /* ON intensitySX1509_REGister I/O[5] */
    [SX1509_REG_OFF_5] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[5] */
    [SX1509_REG_T_RISE_5] = 0x00, /* Fade inSX1509_REGister I/O[5] */
    [SX1509_REG_T_Fall_5] = 0x00, /* Fade outSX1509_REGister I/O[5] */
    [SX1509_REG_T_ON_6] = 0x00,   /* ON timeSX1509_REGister I/O[6] */
    [SX1509_REG_I_ON_6] = 0x00,   /* ON intensitySX1509_REGister I/O[6] */
    [SX1509_REG_OFF_6] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[6] */
    [SX1509_REG_T_RISE_6] = 0x00, /* Fade inSX1509_REGister I/O[6] */
    [SX1509_REG_T_Fall_6] = 0x00, /* Fade outSX1509_REGister I/O[6] */
    [SX1509_REG_T_ON_7] = 0x00,   /* ON timeSX1509_REGister I/O[6] */
    [SX1509_REG_I_ON_7] = 0x00,   /* ON intensitySX1509_REGister I/O[7] */
    [SX1509_REG_OFF_7] = 0x00,    /* OFF time/intensitySX1509_REGister I/O[7] */
    [SX1509_REG_T_RISE_7] = 0x00, /* Fade inSX1509_REGister I/O[7] */
    [SX1509_REG_T_Fall_7] = 0x00, /* Fade outSX1509_REGister I/O[7] */
    [SX1509_REG_T_ON_8] = 0x00,   /* ON timeSX1509_REGister I/O[8] */
    [SX1509_REG_I_ON_8] = 0x00,   /* ON intensitySX1509_REGister I/O[8] */
    [SX1509_REG_OFF_8] = 0x00,   /* OFF time/intensitySX1509_REGister I/O[8]  */
    [SX1509_REG_T_ON_9] = 0x00,  /* ON timeSX1509_REGister I/O[9] */
    [SX1509_REG_I_ON_9] = 0x00,  /* ON intensitySX1509_REGister I/O[9] */
    [SX1509_REG_OFF_9] = 0x00,   /* OFF time/intensitySX1509_REGister I/O[9] */
    [SX1509_REG_T_ON_10] = 0x00, /* ON timeSX1509_REGister I/O[10] */
    [SX1509_REG_I_ON_10] = 0x00, /* ON intensitySX1509_REGister I/O[10] */
    [SX1509_REG_OFF_10] = 0x00,  /* OFF time/intensitySX1509_REGister I/O[10] */
    [SX1509_REG_T_ON_11] = 0x00, /* ON timeSX1509_REGister I/O[11] */
    [SX1509_REG_I_ON_11] = 0x00, /* ON intensitySX1509_REGister I/O[11] */
    [SX1509_REG_OFF_11] = 0x00,  /* OFF time/intensitySX1509_REGister I/O[11] */
    [SX1509_REG_T_ON_12] = 0x00, /* ON timeSX1509_REGister I/O[12] */
    [SX1509_REG_I_ON_12] = 0x00, /* ON intensitySX1509_REGister I/O[12] */
    [SX1509_REG_OFF_12] = 0x00,  /* OFF time/intensitySX1509_REGister I/O[12] */
    [SX1509_REG_T_RISE_12] = 0x00, /* Fade inSX1509_REGister I/O[12] */
    [SX1509_REG_T_Fall_12] = 0x00, /* Fade outSX1509_REGister I/O[12] */
    [SX1509_REG_T_ON_13] = 0x00,   /* ON timeSX1509_REGister I/O[13] */
    [SX1509_REG_I_ON_13] = 0x00,   /* ON intensitySX1509_REGister I/O[13] */
    [SX1509_REG_OFF_13] = 0x00, /* OFF time/intensitySX1509_REGister I/O[13] */
    [SX1509_REG_T_RISE_13] = 0x00, /* Fade inSX1509_REGister I/O[13] */
    [SX1509_REG_T_Fall_13] = 0x00, /* Fade outSX1509_REGister I/O[13] */
    [SX1509_REG_T_ON_14] = 0x00,   /* ON timeSX1509_REGister I/O[14] */
    [SX1509_REG_I_ON_14] = 0x00,   /* ON intensitySX1509_REGister I/O[14] */
    [SX1509_REG_OFF_14] = 0x00, /* OFF time/intensitySX1509_REGister I/O[14] */
    [SX1509_REG_T_RISE_14] = 0x00, /* Fade inSX1509_REGister I/O[14] */
    [SX1509_REG_T_Fall_14] = 0x00, /* Fade outSX1509_REGister I/O[14] */
    [SX1509_REG_T_ON_15] = 0x00,   /* ON timeSX1509_REGister I/O[15] */
    [SX1509_REG_I_ON_15] = 0x00,   /* ON intensitySX1509_REGister I/O[15] */
    [SX1509_REG_OFF_15] = 0x00, /* OFF time/intensitySX1509_REGister I/O[15] */
    [SX1509_REG_T_RISE_15] = 0x00,    /* Fade inSX1509_REGister I/O[115] */
    [SX1509_REG_T_Fall_15] = 0x00,    /* Fade outSX1509_REGister I/O[15] */
    [SX1509_REG_HIGH_INPUT_B] = 0x00, /*  */
    [SX1509_REG_HIGH_INPUT_A] = 0x00, /*  */
    [SX1509_REG_RESET] = 0x00,        /*  */
    [SX1509_REG_TEST_1] = 0x00,       /*  */
    [SX1509_REG_TEST_2] = 0x00,       /*  */
};
