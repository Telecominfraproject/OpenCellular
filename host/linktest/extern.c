/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This tests for the presence of those functions actually used by userspace
 * apps that are not part of firmware or vboot_reference.
 */

#include "crossystem.h"
#include "tlcl.h"
#include "vboot_host.h"

/* TODO(crbug.com/318536) */
const char* progname = "";
const char* command = "";
void (*uuid_generator)(uint8_t* buffer) = NULL;

int main(void)
{
	/* crossystem.h */
	VbGetSystemPropertyInt(0);
	VbGetSystemPropertyString(0, 0, 0);
	VbSetSystemPropertyInt(0, 0);

	/* tlcl.h */
	TlclGetOwnership(0);
	TlclGetRandom(0, 0, 0);
	TlclLibClose();
	TlclLibInit();
	TlclRead(0, 0, 0);

	/* vboot_host.h */
	CgptAdd(0);
	CgptBoot(0);
	CgptCreate(0);
	CgptGetBootPartitionNumber(0);
	CgptGetNumNonEmptyPartitions(0);
	CgptGetPartitionDetails(0);
	CgptPrioritize(0);
	CgptSetAttributes(0);
	FindKernelConfig(0, 0);
	GuidEqual(0, 0);
	GuidIsZero(0);
	GuidToStr(0, 0, 0);
	StrToGuid(0, 0);

	return 0;
}
