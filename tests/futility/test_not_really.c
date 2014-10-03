/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include "gbb_header.h"
#include "test_common.h"

int main(int argc, char *argv[])
{
	TEST_EQ(sizeof(GoogleBinaryBlockHeader),
		GBB_HEADER_SIZE,
		"sizeof(GoogleBinaryBlockHeader)");

	TEST_EQ(0, 0, "Not Really A");

	return !gTestSuccess;
}
