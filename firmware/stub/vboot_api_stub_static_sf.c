/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Workaround for TODO(crbug.com/437107). Remove this file when it's fixed.
 */

#define _STUB_IMPLEMENTATION_

#include <stdio.h>
#include <stdlib.h>

#include "vboot_api.h"

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


/*
 * This file should be used only when building the static version of futility,
 * so let's intentionally break any tests that link with it by accident.
 */
int vboot_api_stub_check_memory(void)
{
	return -1;
}
