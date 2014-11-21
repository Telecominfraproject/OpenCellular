/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Convert structs from vboot1 data format to new vboot2 structs
 */

#include "2sysincludes.h"
#include "2common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "vb2_convert_structs.h"

#include "test_common.h"

struct vb2_signature2 *vb2_create_hash_sig(const uint8_t *data,
					   uint32_t size,
					   enum vb2_hash_algorithm hash_alg)
{
	const struct vb2_private_key *key;
	struct vb2_signature2 *sig;

	if (vb2_private_key_hash(&key, hash_alg))
		return NULL;

	if (vb2_sign_data(&sig, data, size, key, NULL))
		return NULL;

	return sig;
}
