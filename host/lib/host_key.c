/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keys.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <openssl/pem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "host_common.h"
#include "host_misc.h"
#include "vb2_common.h"
#include "vboot_common.h"

int packed_key_looks_ok(const struct vb2_packed_key *key, uint32_t size)
{
	struct vb2_public_key pubkey;
	if (VB2_SUCCESS != vb2_unpack_key_buffer(&pubkey,
						 (const uint8_t *)key,
						 size))
		return 0;

	if (key->key_version > VB2_MAX_KEY_VERSION) {
		/* Currently, TPM only supports 16-bit version */
		VB2_DEBUG("%s() - packed key invalid version\n", __func__);
		return 0;
	}

	/* Success */
	return 1;
}
