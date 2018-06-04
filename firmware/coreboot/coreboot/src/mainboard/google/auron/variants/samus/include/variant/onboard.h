/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef ONBOARD_H
#define ONBOARD_H

#define BOARD_TRACKPAD_I2C_ADDR		0x4a
#define BOARD_TOUCHSCREEN_I2C_ADDR	0x4b
#define BOARD_CODEC_I2C_ADDR		0x2c

#define BOARD_TRACKPAD_WAKE_GPIO	9	/* GPIO9 */
#define BOARD_WLAN_WAKE_GPIO		10	/* GPIO10 */
#define BOARD_TOUCHSCREEN_WAKE_GPIO	14	/* GPIO14 */
#define BOARD_PP3300_AUTOBAHN_GPIO	23	/* GPIO23 */
#define BOARD_WLAN_DISABLE_GPIO		42	/* GPIO42 */
#define BOARD_CODEC_WAKE_GPIO		45	/* GPIO45 */
#define BOARD_SSD_RESET_GPIO		47	/* GPIO47 */
#define BOARD_LTE_DISABLE_GPIO		59	/* GPIO59 */

#endif
