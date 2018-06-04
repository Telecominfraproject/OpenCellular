/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <console/console.h>

void __div0(void); // called from asm so no need for a prototype in a header

/* Replacement (=dummy) for GNU/Linux division-by zero handler */
/* recursion is ok here because we have no formats ... */
void __div0 (void)
{
	printk(BIOS_EMERG, "DIVIDE BY ZERO! continuing ...\n");
}
