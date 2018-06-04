/*
 * Copyright (C) 2015 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "secimage.h"
#include <openssl/hmac.h>

/*----------------------------------------------------------------------
 * Name    : HmacSha256Hash
 * Purpose :
 * Input   : none
 * Output  : none
 *---------------------------------------------------------------------*/
int HmacSha256Hash(uint8_t *data, uint32_t len, uint8_t *hash, uint8_t *key)
{
	unsigned int hash_len = 0;

	if (!HMAC(EVP_sha256(), key, 32, data, len, hash, &hash_len)) {
		printf("HMAC failed\n");
		return -1;
	} else if (hash_len != 32) {
		printf("HMAC reported unexpected md_len of %u\n", hash_len);
		return -2;
	}

	return 0;
}

/*----------------------------------------------------------------------
 * Name    : AppendHMACSignature
 * Purpose : Appends HMAC signature at the end of the data
 *---------------------------------------------------------------------*/
int AppendHMACSignature(uint8_t *data, uint32_t length, char *filename,
			uint32_t offset)
{
	uint8_t  hmackey[32];
	uint32_t len;
	uint32_t status;
	uint8_t *digest = data + length;

	len = ReadBinaryFile(filename, hmackey, 32);
	if (len != 32) {
		printf("Error reading hmac key file\n");
		return 0;
	}

	status = HmacSha256Hash(&data[offset], length - offset, digest,
				hmackey);

	if (status) {
		printf("HMAC-SHA256 hash error\n");
		return 0;
	}

	return 32;
}
