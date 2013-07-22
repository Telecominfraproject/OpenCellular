/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for do-nothing rollback_index functions with disabled TPM
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "rollback_index.h"
#include "test_common.h"

int main(int argc, char* argv[])
{
	int is_virt_dev;
	uint32_t version;

	TEST_EQ(RollbackS3Resume(), 0, "RollbackS3Resume()");

	is_virt_dev = 1;
	version = 1;
	TEST_EQ(RollbackFirmwareSetup(0, 0, 0, &is_virt_dev, &version),
		0, "RollbackFirmwareSetup()");
	TEST_EQ(is_virt_dev, 0, "rfs is_virt_dev");
	TEST_EQ(version, 0, "rfs version");

	TEST_EQ(RollbackFirmwareWrite(0), 0, "RollbackFirmwareWrite()");
	TEST_EQ(RollbackFirmwareLock(), 0, "RollbackFirmwareLock()");

	version = 1;
	TEST_EQ(RollbackKernelRead(&version), 0, "RollbackKernelRead()");
	TEST_EQ(version, 0, "rkr version");

	TEST_EQ(RollbackKernelWrite(0), 0, "RollbackKernelWrite()");
	TEST_EQ(RollbackKernelLock(0), 0, "RollbackKernelLock()");

	return gTestSuccess ? 0 : 255;
}
