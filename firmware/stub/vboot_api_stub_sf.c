/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of firmware-provided API functions.
 */

#define _STUB_IMPLEMENTATION_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "vboot_api.h"

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

void *VbExMalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		/* Fatal Error. We must abort. */
		abort();
	}
	return p;
}

void VbExFree(void *ptr)
{
	free(ptr);
}

VbError_t VbExHashFirmwareBody(VbCommonParams *cparams,
                               uint32_t firmware_index)
{
	return VBERROR_SUCCESS;
}
