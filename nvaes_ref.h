/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "cbootimage.h"
#include "string.h"

#ifndef INCLUDED_NVAES_REF_H
#define INCLUDED_NVAES_REF_H

#define NVAES_STATECOLS	4
#define NVAES_KEYCOLS	4
#define NVAES_ROUNDS	10

void nv_aes_expand_key(u_int8_t *key, u_int8_t *expkey);
void nv_aes_encrypt(u_int8_t *in,
		u_int8_t *expkey,
		u_int8_t *out);

#endif // INCLUDED_NVAES_REF_H
