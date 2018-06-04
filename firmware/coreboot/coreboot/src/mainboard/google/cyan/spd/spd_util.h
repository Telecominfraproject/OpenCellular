/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Matt DeVillier
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

#ifndef SPD_UTIL_H
#define SPD_UTIL_H

uint8_t get_ramid(void);
int get_variant_spd_index(int ram_id, int *dual);

#endif /* SPD_UTIL_H */
