/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel.c
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"
#include "vboot_api.h"

/* Mock data */
static uint32_t virtual_dev_mode_fail;

/**
 * Reset mock data (for use before each test)
 */
static void ResetMocks(void)
{
	virtual_dev_mode_fail = 0;
}

/* Mocks */
uint32_t SetVirtualDevMode(int val)
{
	if (virtual_dev_mode_fail)
		return VBERROR_SIMULATED;
	return VBERROR_SUCCESS;
}

static void VbUnlockDeviceTest(void)
{
	ResetMocks();
	TEST_EQ(VbUnlockDevice(), 0, "unlock success");

	ResetMocks();
	virtual_dev_mode_fail = 1;
	TEST_EQ(VbUnlockDevice(), VBERROR_TPM_SET_BOOT_MODE_STATE,
		"set dev fail");
}

int main(void)
{
	VbUnlockDeviceTest();

	return gTestSuccess ? 0 : 255;
}
