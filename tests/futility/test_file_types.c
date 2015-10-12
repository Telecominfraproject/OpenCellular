/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_type.h"
#include "futility.h"
#include "test_common.h"

/*
 * Files that exemplify each type.
 * Paths are relative to the source directory.
 * A missing path means we don't (yet?) know how to identify it reliably.
 */
static struct {
	enum futil_file_type type;
	const char * const file;
} test_case[] = {
	{FILE_TYPE_UNKNOWN,         "tests/futility/data/random_noise.bin"},
	{FILE_TYPE_PUBKEY,          "tests/devkeys/root_key.vbpubk"},
	{FILE_TYPE_KEYBLOCK,        "tests/devkeys/kernel.keyblock"},
	{FILE_TYPE_FW_PREAMBLE,     "tests/futility/data/fw_vblock.bin"},
	{FILE_TYPE_GBB,	            "tests/futility/data/fw_gbb.bin"},
	{FILE_TYPE_BIOS_IMAGE,      "tests/futility/data/bios_zgb_mp.bin"},
	{FILE_TYPE_OLD_BIOS_IMAGE,  "tests/futility/data/bios_mario_mp.bin"},
	{FILE_TYPE_KERN_PREAMBLE,   "tests/futility/data/kern_preamble.bin"},
	{FILE_TYPE_RAW_FIRMWARE,    },		/* need a test for this */
	{FILE_TYPE_RAW_KERNEL,	    },		/* need a test for this */
	{FILE_TYPE_CHROMIUMOS_DISK, },		/* need a test for this */
	{FILE_TYPE_PRIVKEY,	    "tests/devkeys/root_key.vbprivk"},
	{FILE_TYPE_VB2_PUBKEY,      "tests/futility/data/sample.vbpubk2"},
	{FILE_TYPE_VB2_PRIVKEY,     "tests/futility/data/sample.vbprik2"},
	{FILE_TYPE_PEM,             "tests/testkeys/key_rsa2048.pem"},
	{FILE_TYPE_USBPD1,          "tests/futility/data/zinger_mp_image.bin"},
	{FILE_TYPE_RWSIG,           },		/* need a test for this */
};
BUILD_ASSERT(ARRAY_SIZE(test_case) == NUM_FILE_TYPES);

int main(int argc, char *argv[])
{
	char filename[PATH_MAX];
	char status[80];
	char *srcdir;
	enum futil_file_type type;
	int i;

	/* Where's the source directory? */
	srcdir = getenv("SRCDIR");
	if (argc > 1)
		srcdir = argv[1];
	if (!srcdir)
		srcdir = ".";

	/* Complain about some files we can't handle */
	TEST_EQ(futil_file_type("/Sir/Not/Appearing/In/This/Film", &type),
		FILE_ERR_OPEN, "Identify missing file");
	TEST_EQ(futil_file_type("/", &type),
		FILE_ERR_DIR, "Identify directory");
	TEST_EQ(futil_file_type("/dev/zero", &type),
		FILE_ERR_CHR, "Identify char device");

	/* Now test things we can handle */
	for (i = 0; i < NUM_FILE_TYPES; i++) {

		if (!test_case[i].file) {
			printf("%sWarning: No test for file type %d (%s)%s\n",
			       COL_YELLOW, test_case[i].type,
			       futil_file_type_name(test_case[i].type),
			       COL_STOP);
			continue;
		}

		snprintf(filename, sizeof(filename), "%s/%s",
			 srcdir, test_case[i].file);

		type = NUM_FILE_TYPES;
		snprintf(status, sizeof(status),
			 "File type %d (%s): examined",
			 test_case[i].type,
			 futil_file_type_name(test_case[i].type));
		TEST_EQ(FILE_ERR_NONE, futil_file_type(filename, &type),
			status);

		snprintf(status, sizeof(status),
			 "File type %d (%s) identified",
			 test_case[i].type,
			 futil_file_type_name(test_case[i].type));
		TEST_EQ(type, test_case[i].type, status);
	}

	return !gTestSuccess;
}
