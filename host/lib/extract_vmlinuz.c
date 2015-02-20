/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports a vmlinuz from a kernel partition in memory.
 */

#include <stdlib.h>
#include <string.h>

#include "vboot_struct.h"


int ExtractVmlinuz(void *kpart_data, size_t kpart_size,
		   void **vmlinuz_out, size_t *vmlinuz_size) {
	uint64_t now = 0;
	VbKeyBlockHeader *keyblock = NULL;
	VbKernelPreambleHeader *preamble = NULL;
	uint8_t *kblob_data = NULL;
	uint64_t kblob_size = 0;
	uint64_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;
	uint64_t vmlinuz_header_offset = 0;
	void *vmlinuz = NULL;

	keyblock = (VbKeyBlockHeader *)kpart_data;
	now += keyblock->key_block_size;
	if (now > kpart_size)
		return 1;

	preamble = (VbKernelPreambleHeader *)(kpart_data + now);
	now += preamble->preamble_size;
	if (now > kpart_size)
		return 1;

	kblob_data = kpart_data + now;
	kblob_size = preamble->body_signature.data_size;

	if (!kblob_data || (now + kblob_size) > kpart_size)
		return 1;

	if (preamble->header_version_minor > 0) {
		vmlinuz_header_address = preamble->vmlinuz_header_address;
		vmlinuz_header_size = preamble->vmlinuz_header_size;
	}

	if (!vmlinuz_header_size ||
	     kpart_data + vmlinuz_header_offset + vmlinuz_header_size > kpart_data) {
		return 1;
	}

	// calculate the vmlinuz_header offset from
	// the beginning of the kpart_data.  The kblob doesn't
	// include the body_load_offset, but does include
	// the keyblock and preamble sections.
	vmlinuz_header_offset = vmlinuz_header_address -
		preamble->body_load_address +
		keyblock->key_block_size +
		preamble->preamble_size;

	vmlinuz = malloc(vmlinuz_header_size + kblob_size);
	if (vmlinuz == NULL)
		return 1;

	memcpy(vmlinuz, kpart_data + vmlinuz_header_offset,
	       vmlinuz_header_size);

	memcpy(vmlinuz + vmlinuz_header_size, kblob_data, kblob_size);

	*vmlinuz_out = vmlinuz;
	*vmlinuz_size = vmlinuz_header_size + kblob_size;

	return 0;
}
