/*
 * This file is part of the coreboot project.
 *
 * Copyright 2014 Google Inc.
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

#ifndef __CHROMEOS_SYMBOLS_H
#define __CHROMEOS_SYMBOLS_H

extern u8 _watchdog_tombstone[];
extern u8 _ewatchdog_tombstone[];
#define _watchdog_tombstone_size (_ewatchdog_tombstone - _watchdog_tombstone)

#endif /* __CHROMEOS_SYMBOLS_H */
