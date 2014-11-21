/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef VBOOT_REFERENCE_VB2_CONVERT_STRUCTS_H_
#define VBOOT_REFERENCE_VB2_CONVERT_STRUCTS_H_

#include "2struct.h"

/**
 * Create an unsigned hash signature of the data.
 *
 * @param data		Data to sign
 * @param size		Size of data in bytes
 * @return a newly-allocated signature, which the caller must free, or NULL if
 *	   error.
 */
struct vb2_signature2 *vb2_create_hash_sig(const uint8_t *data,
					   uint32_t size,
					   enum vb2_hash_algorithm hash_alg);

#endif  /* VBOOT_REFERENCE_VB2_CONVERT_STRUCTS_H_ */
