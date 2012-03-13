/**
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

/*
 * crypto.h - Definitions for the crypto support.
 */

#ifndef INCLUDED_CRYPTO_H_N
#define INCLUDED_CRYPTO_H_N

#include "cbootimage.h"

/* lengths, in bytes */
#define KEY_LENGTH (128/8)

#define ICEIL(a, b) (((a) + (b) - 1)/(b))

#define AES_CMAC_CONST_RB 0x87  // from RFC 4493, Figure 2.2

/* Function prototypes */

int
sign_bct(build_image_context *context,
                  u_int8_t *bct);

int
sign_data_block(u_int8_t *source,
		u_int32_t length,
		u_int8_t *signature);

#endif /* #ifndef INCLUDED_CRYPTO_H */
