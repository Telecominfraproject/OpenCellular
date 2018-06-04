/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 Intel Corporation. All Rights Reserved.
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

#include <soc/gpio.h>

const struct pad_config *gpio_table(size_t *num);
const struct pad_config *early_gpio_table(size_t *num);
const struct pad_config *sleep_gpio_table(size_t *num);
