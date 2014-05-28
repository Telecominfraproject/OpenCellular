/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for rollback_index functions
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "crc8.h"
#include "rollback_index.h"
#include "test_common.h"
#include "tlcl.h"
#include "utility.h"
#include "vboot_common.h"

/*
 * Buffer to hold accumulated list of calls to mocked Tlcl functions.
 * Each function appends itself to the buffer and updates mock_cnext.
 *
 * Size of mock_calls[] should be big enough to handle all expected
 * call sequences; 16KB should be plenty since none of the sequences
 * below is more than a few hundred bytes.  We could be more clever
 * and use snprintf() with length checking below, at the expense of
 * making all the mock implementations bigger.  If this were code used
 * outside of unit tests we'd want to do that, but here if we did
 * overrun the buffer the worst that's likely to happen is we'll crash
 * the test, and crash = failure anyway.
 */
static char mock_calls[16384];
static char *mock_cnext = mock_calls;

/*
 * Variables to support mocked error values from Tlcl functions.  Each
 * call, mock_count is incremented.  If mock_count==fail_at_count, return
 * fail_with_error instead of the normal return value.
 */
static int mock_count = 0;
static int fail_at_count = 0;
static uint32_t fail_with_error = TPM_SUCCESS;

/* Similar, to determine when to inject noise during reads & writes */
#define MAX_NOISE_COUNT 64              /* no noise after this many */
static int noise_count = 0;             /* read/write attempt (zero-based) */
static int noise_on[MAX_NOISE_COUNT];   /* calls to inject noise on */

/* Params / backing store for mocked Tlcl functions. */
static TPM_PERMANENT_FLAGS mock_pflags;
static RollbackSpaceFirmware mock_rsf;
static RollbackSpaceKernel mock_rsk;
static uint32_t mock_permissions;

/* Reset the variables for the Tlcl mock functions. */
static void ResetMocks(int fail_on_call, uint32_t fail_with_err)
{
	*mock_calls = 0;
	mock_cnext = mock_calls;
	mock_count = 0;
	fail_at_count = fail_on_call;
	fail_with_error = fail_with_err;
	noise_count = 0;
	Memset(&noise_on, 0, sizeof(noise_on));

	Memset(&mock_pflags, 0, sizeof(mock_pflags));
	Memset(&mock_rsf, 0, sizeof(mock_rsf));
	Memset(&mock_rsk, 0, sizeof(mock_rsk));
	mock_permissions = 0;
}

/****************************************************************************/
/* Function to garble data on its way to or from the TPM */
static void MaybeInjectNoise(void *data, uint32_t length)
{
	if (noise_count < MAX_NOISE_COUNT && noise_on[noise_count]) {
		uint8_t *val = data;
		val[length - 1]++;
	}
	noise_count++;
}

/****************************************************************************/
/* Mocks for tlcl functions which log the calls made to mock_calls[]. */

uint32_t TlclLibInit(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclLibInit()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclStartup(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclStartup()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclResume(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclResume()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclForceClear(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclForceClear()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclSetEnable(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclSetEnable()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclSetDeactivated(uint8_t flag)
{
	mock_cnext += sprintf(mock_cnext, "TlclSetDeactivated(%d)\n", flag);
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclRead(uint32_t index, void* data, uint32_t length)
{
	mock_cnext += sprintf(mock_cnext, "TlclRead(0x%x, %d)\n",
			      index, length);

	if (FIRMWARE_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsf), "TlclRead rsf size");
		Memcpy(data, &mock_rsf, length);
		MaybeInjectNoise(data, length);
	} else if (KERNEL_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsk), "TlclRead rsk size");
		Memcpy(data, &mock_rsk, length);
		MaybeInjectNoise(data, length);
	} else {
		Memset(data, 0, length);
	}

	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void *data, uint32_t length)
{
	mock_cnext += sprintf(mock_cnext, "TlclWrite(0x%x, %d)\n",
			      index, length);

	if (FIRMWARE_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsf), "TlclWrite rsf size");
		Memcpy(&mock_rsf, data, length);
		MaybeInjectNoise(&mock_rsf, length);
	} else if (KERNEL_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsk), "TlclWrite rsk size");
		Memcpy(&mock_rsk, data, length);
		MaybeInjectNoise(&mock_rsk, length);
	}

	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size)
{
	mock_cnext += sprintf(mock_cnext, "TlclDefineSpace(0x%x, 0x%x, %d)\n",
			      index, perm, size);
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclSelfTestFull(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclSelfTestFull()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclContinueSelfTest(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclContinueSelfTest()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS *pflags)
{
	mock_cnext += sprintf(mock_cnext, "TlclGetPermanentFlags()\n");
	Memcpy(pflags, &mock_pflags, sizeof(mock_pflags));
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

/* TlclGetFlags() doesn't need mocking; it calls TlclGetPermanentFlags() */

uint32_t TlclAssertPhysicalPresence(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclAssertPhysicalPresence()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclFinalizePhysicalPresence(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclFinalizePhysicalPresence()\n");
	mock_pflags.physicalPresenceLifetimeLock = 1;
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclPhysicalPresenceCMDEnable(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclPhysicalPresenceCMDEnable()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclSetNvLocked(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclSetNvLocked()\n");
	mock_pflags.nvLocked = 1;
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclSetGlobalLock(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclSetGlobalLock()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclLockPhysicalPresence(void)
{
	mock_cnext += sprintf(mock_cnext, "TlclLockPhysicalPresence()\n");
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions)
{
	mock_cnext += sprintf(mock_cnext, "TlclGetPermissions(0x%x)\n", index);
	*permissions = mock_permissions;
	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

/****************************************************************************/
/* Tests for CRC errors  */

extern uint32_t ReadSpaceFirmware(RollbackSpaceFirmware *rsf);
extern uint32_t WriteSpaceFirmware(RollbackSpaceFirmware *rsf);

static void CrcTestFirmware(void)
{
	RollbackSpaceFirmware rsf;

	/* Noise on reading, shouldn't matter here because version == 0 */
	ResetMocks(0, 0);
	noise_on[0] = 1;
	TEST_EQ(ReadSpaceFirmware(&rsf), 0, "ReadSpaceFirmware(), v0");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/*
	 * But if the version >= 2, it will try three times and fail because
	 * the CRC is no good.
	 */
	ResetMocks(0, 0);
	mock_rsf.struct_version = 2;
	TEST_EQ(ReadSpaceFirmware(&rsf), TPM_E_CORRUPTED_STATE,
		"ReadSpaceFirmware(), v2, bad CRC");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* If the CRC is good and some noise happens, it should recover. */
	ResetMocks(0, 0);
	mock_rsf.struct_version = 2;
	mock_rsf.crc8 = Crc8(&mock_rsf, offsetof(RollbackSpaceFirmware, crc8));
	noise_on[0] = 1;
	TEST_EQ(ReadSpaceFirmware(&rsf), 0,
		"ReadSpaceFirmware(), v2, good CRC");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* A write with version < 2 should convert to v2 and create the CRC */
	ResetMocks(0, 0);
	Memset(&rsf, 0, sizeof(rsf));
	TEST_EQ(WriteSpaceFirmware(&rsf), 0, "WriteSpaceFirmware(), v0");
	TEST_EQ(mock_rsf.struct_version, 2, "WriteSpaceFirmware(), check v2");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* Same as above, but with some noise during the readback */
	ResetMocks(0, 0);
	Memset(&rsf, 0, sizeof(rsf));
	noise_on[1] = 1;
	noise_on[2] = 1;
	TEST_EQ(WriteSpaceFirmware(&rsf), 0,
		"WriteSpaceFirmware(), read noise");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* With noise during the write, we'll try the write again */
	ResetMocks(0, 0);
	Memset(&rsf, 0, sizeof(rsf));
	noise_on[0] = 1;
	TEST_EQ(WriteSpaceFirmware(&rsf), 0,
		"WriteSpaceFirmware(), write noise");
	TEST_EQ(mock_rsf.struct_version, 2, "WriteSpaceFirmware(), check v2");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* Only if it just keeps on failing forever do we eventually give up */
	ResetMocks(0, 0);
	Memset(&rsf, 0, sizeof(rsf));
	Memset(noise_on, 1, sizeof(noise_on));
	TEST_EQ(WriteSpaceFirmware(&rsf), TPM_E_CORRUPTED_STATE,
		"WriteSpaceFirmware(), always noise");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
}

extern uint32_t ReadSpaceKernel(RollbackSpaceKernel *rsk);
extern uint32_t WriteSpaceKernel(RollbackSpaceKernel *rsk);

static void CrcTestKernel(void)
{
	RollbackSpaceKernel rsk;

	/* Noise on reading shouldn't matter here because version == 0 */
	ResetMocks(0, 0);
	noise_on[0] = 1;
	TEST_EQ(ReadSpaceKernel(&rsk), 0, "ReadSpaceKernel(), v0");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/*
	 * But if the version >= 2, it will try three times and fail because
	 * the CRC is no good.
	 */
	ResetMocks(0, 0);
	mock_rsk.struct_version = 2;
	TEST_EQ(ReadSpaceKernel(&rsk), TPM_E_CORRUPTED_STATE,
		"ReadSpaceKernel(), v2, bad CRC");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* If the CRC is good and some noise happens, it should recover. */
	ResetMocks(0, 0);
	mock_rsk.struct_version = 2;
	mock_rsk.crc8 = Crc8(&mock_rsk, offsetof(RollbackSpaceKernel, crc8));
	noise_on[0] = 1;
	TEST_EQ(ReadSpaceKernel(&rsk), 0, "ReadSpaceKernel(), v2, good CRC");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* A write with version < 2 should convert to v2 and create the CRC */
	ResetMocks(0, 0);
	Memset(&rsk, 0, sizeof(rsk));
	TEST_EQ(WriteSpaceKernel(&rsk), 0, "WriteSpaceKernel(), v0");
	TEST_EQ(mock_rsk.struct_version, 2, "WriteSpaceKernel(), check v2");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* Same as above, but with some noise during the readback */
	ResetMocks(0, 0);
	Memset(&rsk, 0, sizeof(rsk));
	noise_on[1] = 1;
	noise_on[2] = 1;
	TEST_EQ(WriteSpaceKernel(&rsk), 0, "WriteSpaceKernel(), read noise");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* With noise during the write, we'll try the write again */
	ResetMocks(0, 0);
	Memset(&rsk, 0, sizeof(rsk));
	noise_on[0] = 1;
	TEST_EQ(WriteSpaceKernel(&rsk), 0, "WriteSpaceKernel(), write noise");
	TEST_EQ(mock_rsk.struct_version, 2, "WriteSpaceKernel(), check v2");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* Only if it just keeps on failing forever do we eventually give up */
	ResetMocks(0, 0);
	Memset(&rsk, 0, sizeof(rsk));
	Memset(noise_on, 1, sizeof(noise_on));
	TEST_EQ(WriteSpaceKernel(&rsk), TPM_E_CORRUPTED_STATE,
		"WriteSpaceKernel(), always noise");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");
}

/****************************************************************************/
/* Tests for misc helper functions */

static void MiscTest(void)
{
	uint8_t buf[8];

	ResetMocks(0, 0);
	TEST_EQ(TPMClearAndReenable(), 0, "TPMClearAndReenable()");
	TEST_STR_EQ(mock_calls,
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n",
		    "tlcl calls");

	ResetMocks(0, 0);
	TEST_EQ(SafeWrite(0x123, buf, 8), 0, "SafeWrite()");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x123, 8)\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_BADINDEX);
	TEST_EQ(SafeWrite(0x123, buf, 8), TPM_E_BADINDEX, "SafeWrite() bad");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x123, 8)\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_MAXNVWRITES);
	TEST_EQ(SafeWrite(0x123, buf, 8), 0, "SafeWrite() retry max writes");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x123, 8)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    "TlclWrite(0x123, 8)\n",
		    "tlcl calls");

	ResetMocks(0, 0);
	TEST_EQ(SafeDefineSpace(0x123, 6, 8), 0, "SafeDefineSpace()");
	TEST_STR_EQ(mock_calls,
		    "TlclDefineSpace(0x123, 0x6, 8)\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_BADINDEX);
	TEST_EQ(SafeDefineSpace(0x123, 6, 8), TPM_E_BADINDEX,
		"SafeDefineSpace() bad");
	TEST_STR_EQ(mock_calls,
		    "TlclDefineSpace(0x123, 0x6, 8)\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_MAXNVWRITES);
	TEST_EQ(SafeDefineSpace(0x123, 6, 8), 0,
		"SafeDefineSpace() retry max writes");
	TEST_STR_EQ(mock_calls,
		    "TlclDefineSpace(0x123, 0x6, 8)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    "TlclDefineSpace(0x123, 0x6, 8)\n",
		    "tlcl calls");
}

/****************************************************************************/

/* Tests for one-time initialization */
static void OneTimeInitTest(void)
{
	RollbackSpaceFirmware rsf;
	RollbackSpaceKernel rsk;

	/* Complete initialization */
	ResetMocks(0, 0);
	TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), 0, "OneTimeInitializeTPM()");
	TEST_STR_EQ(mock_calls,
		    "TlclSelfTestFull()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclFinalizePhysicalPresence()\n"
		    "TlclSetNvLocked()\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    /* backup space */
		    "TlclDefineSpace(0x1009, 0x1, 16)\n"
		    /* kernel space */
		    "TlclDefineSpace(0x1008, 0x1, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    /* firmware space */
		    "TlclDefineSpace(0x1007, 0x8001, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
	TEST_EQ(mock_rsf.struct_version, ROLLBACK_SPACE_FIRMWARE_VERSION,
		"rsf ver");
	TEST_EQ(mock_rsf.flags, 0, "rsf flags");
	TEST_EQ(mock_rsf.fw_versions, 0, "rsf fw_versions");
	TEST_EQ(mock_rsk.struct_version, ROLLBACK_SPACE_KERNEL_VERSION,
		"rsk ver");
	TEST_EQ(mock_rsk.uid, ROLLBACK_SPACE_KERNEL_UID, "rsk uid");
	TEST_EQ(mock_rsk.kernel_versions, 0, "rsk kernel_versions");

	/* Physical presence already initialized */
	ResetMocks(0, 0);
	mock_pflags.physicalPresenceLifetimeLock = 1;
	TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), 0, "OneTimeInitializeTPM()");
	TEST_STR_EQ(mock_calls,
		    "TlclSelfTestFull()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclSetNvLocked()\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    /* backup space */
		    "TlclDefineSpace(0x1009, 0x1, 16)\n"
		    /* kernel space */
		    "TlclDefineSpace(0x1008, 0x1, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    /* firmware space */
		    "TlclDefineSpace(0x1007, 0x8001, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* NV locking already initialized */
	ResetMocks(0, 0);
	mock_pflags.nvLocked = 1;
	TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), 0, "OneTimeInitializeTPM()");
	TEST_STR_EQ(mock_calls,
		    "TlclSelfTestFull()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclFinalizePhysicalPresence()\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    /* backup space */
		    "TlclDefineSpace(0x1009, 0x1, 16)\n"
		    /* kernel space */
		    "TlclDefineSpace(0x1008, 0x1, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    /* firmware space */
		    "TlclDefineSpace(0x1007, 0x8001, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* Self test error */
	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(OneTimeInitializeTPM(&rsf, &rsk), TPM_E_IOERROR,
		"OneTimeInitializeTPM() selftest");
	TEST_STR_EQ(mock_calls,
		    "TlclSelfTestFull()\n",
		    "tlcl calls");
}

/****************************************************************************/
/* Tests for TPM setup */

static void SetupTpmTest(void)
{
	RollbackSpaceFirmware rsf;

	/* Complete setup */
	ResetMocks(0, 0);
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), 0, "SetupTPM()");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* If TPM is disabled or deactivated, must enable it */
	ResetMocks(0, 0);
	mock_pflags.disable = 1;
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), TPM_E_MUST_REBOOT,
		"SetupTPM() disabled");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n",
		    "tlcl calls");

	ResetMocks(0, 0);
	mock_pflags.deactivated = 1;
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), TPM_E_MUST_REBOOT,
		"SetupTPM() deactivated");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n",
		    "tlcl calls");

	/* If physical presence command isn't enabled, try to enable it */
	ResetMocks(3, TPM_E_IOERROR);
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), 0, "SetupTPM() pp cmd");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclPhysicalPresenceCMDEnable()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* If firmware space is missing, do one-time init */
	ResetMocks(5, TPM_E_BADINDEX);
	mock_pflags.physicalPresenceLifetimeLock = 1;
	mock_pflags.nvLocked = 1;
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), 0, "SetupTPM() no firmware space");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n"
		    /* Calls from one-time init */
		    "TlclSelfTestFull()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    /* backup space */
		    "TlclDefineSpace(0x1009, 0x1, 16)\n"
		    "TlclDefineSpace(0x1008, 0x1, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n"
		    "TlclDefineSpace(0x1007, 0x8001, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* Other firmware space error is passed through */
	ResetMocks(5, TPM_E_IOERROR);
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), TPM_E_CORRUPTED_STATE,
		"SetupTPM() bad firmware space");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* If developer flag has toggled, clear ownership and write new flag */
	ResetMocks(0, 0);
	TEST_EQ(SetupTPM(1, 0, 0, &rsf), 0, "SetupTPM() to dev");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
	TEST_EQ(mock_rsf.flags, FLAG_LAST_BOOT_DEVELOPER,
		"fw space flags to dev 1");

	ResetMocks(0, 0);
	mock_rsf.flags = FLAG_LAST_BOOT_DEVELOPER;
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), 0, "SetupTPM() from dev");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
	TEST_EQ(mock_rsf.flags, 0, "fw space flags from dev 1");

	/* If TPM clear request, clear ownership also */
	ResetMocks(0, 0);
	TEST_EQ(SetupTPM(0, 0, 1, &rsf), 0, "SetupTPM() clear owner");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n",
		    "tlcl calls");

	/* Handle request to clear virtual dev switch */
	ResetMocks(0, 0);
	mock_rsf.flags = FLAG_VIRTUAL_DEV_MODE_ON | FLAG_LAST_BOOT_DEVELOPER;
	TEST_EQ(SetupTPM(0, 1, 0, &rsf), 0, "SetupTPM() clear virtual dev");
	TEST_EQ(mock_rsf.flags, 0, "Clear virtual dev");

	/* If virtual dev switch is on, that should set last boot developer */
	ResetMocks(0, 0);
	mock_rsf.flags = FLAG_VIRTUAL_DEV_MODE_ON;
	SetupTPM(0, 0, 0, &rsf);
	TEST_EQ(mock_rsf.flags,
		FLAG_VIRTUAL_DEV_MODE_ON | FLAG_LAST_BOOT_DEVELOPER,
		"virtual dev sets last boot");

	/*
	 * Note: SetupTPM() recovery_mode parameter sets a global flag in
	 * rollback_index.c; this is tested along with RollbackKernelLock()
	 * below.
	 */
}

/****************************************************************************/
/* Tests for RollbackFirmware() calls */

static void RollbackFirmwareTest(void)
{
	uint32_t version;
	int dev_mode;

	/* Normal setup */
	ResetMocks(0, 0);
	dev_mode = 0;
	version = 123;
	mock_rsf.fw_versions = 0x12345678;
	TEST_EQ(RollbackFirmwareSetup(0, dev_mode, 0, &dev_mode, &version),
		0, "RollbackFirmwareSetup()");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
	TEST_EQ(version, 0x12345678, "RollbackFirmwareSetup() version");

	/* Error during setup should clear version */
	ResetMocks(1, TPM_E_IOERROR);
	dev_mode = 0;
	version = 123;
	mock_rsf.fw_versions = 0x12345678;
	TEST_EQ(RollbackFirmwareSetup(0, dev_mode, 0, &dev_mode, &version),
		TPM_E_IOERROR,
		"RollbackFirmwareSetup() error");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n",
		    "tlcl calls");
	TEST_EQ(version, 0, "RollbackFirmwareSetup() version on error");

	/* Developer mode flag gets passed properly */
	ResetMocks(0, 0);
	dev_mode = 1;
	TEST_EQ(RollbackFirmwareSetup(dev_mode, 0, 0, &dev_mode, &version),
		0, "RollbackFirmwareSetup() to dev");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
	TEST_EQ(mock_rsf.flags, FLAG_LAST_BOOT_DEVELOPER,
		"fw space flags to dev 2");

	/* So does clear-TPM request */
	ResetMocks(0, 0);
	dev_mode = 0;
	TEST_EQ(RollbackFirmwareSetup(dev_mode, 0, 1, &dev_mode, &version),
		0, "RollbackFirmwareSetup() clear owner");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclStartup()\n"
		    "TlclAssertPhysicalPresence()\n"
		    "TlclGetPermanentFlags()\n"
		    "TlclRead(0x1007, 10)\n"
		    "TlclForceClear()\n"
		    "TlclSetEnable()\n"
		    "TlclSetDeactivated(0)\n",
		    "tlcl calls");

	/* Test write */
	ResetMocks(0, 0);
	TEST_EQ(RollbackFirmwareWrite(0xBEAD1234), 0,
		"RollbackFirmwareWrite()");
	TEST_EQ(mock_rsf.fw_versions, 0xBEAD1234,
		"RollbackFirmwareWrite() version");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackFirmwareWrite(123), TPM_E_IOERROR,
		"RollbackFirmwareWrite() error");

	/* Test setting virtual dev mode */
	ResetMocks(0, 0);
	TEST_EQ(SetVirtualDevMode(1), 0, "SetVirtualDevMode(1)");
	TEST_EQ(mock_rsf.flags, FLAG_VIRTUAL_DEV_MODE_ON, "Virtual dev on");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");
	ResetMocks(0, 0);
	TEST_EQ(SetVirtualDevMode(0), 0, "SetVirtualDevMode(0)");
	TEST_EQ(mock_rsf.flags, 0, "Virtual dev off");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n"
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* Test lock */
	ResetMocks(0, 0);
	TEST_EQ(RollbackFirmwareLock(), 0, "RollbackFirmwareLock()");
	TEST_STR_EQ(mock_calls,
		    "TlclSetGlobalLock()\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackFirmwareLock(), TPM_E_IOERROR,
		"RollbackFirmwareLock() error");
}

/****************************************************************************/
/* Tests for RollbackKernel() calls */

static void RollbackKernelTest(void)
{
	RollbackSpaceFirmware rsf;
	uint32_t version = 0;

	/*
	 * RollbackKernel*() functions use a global flag inside
	 * rollback_index.c based on recovery mode, which is set by SetupTPM().
	 * Clear the flag for the first set of tests.
	 */
	TEST_EQ(SetupTPM(0, 0, 0, &rsf), 0, "SetupTPM()");

	/* Normal read */
	ResetMocks(0, 0);
	mock_rsk.uid = ROLLBACK_SPACE_KERNEL_UID;
	mock_permissions = TPM_NV_PER_PPWRITE;
	mock_rsk.kernel_versions = 0x87654321;
	TEST_EQ(RollbackKernelRead(&version), 0, "RollbackKernelRead()");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n"
		    "TlclGetPermissions(0x1008)\n",
		    "tlcl calls");
	TEST_EQ(version, 0x87654321, "RollbackKernelRead() version");

	/* Read error */
	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackKernelRead(&version), TPM_E_IOERROR,
		"RollbackKernelRead() error");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* Wrong permission or UID will return error */
	ResetMocks(0, 0);
	mock_rsk.uid = ROLLBACK_SPACE_KERNEL_UID + 1;
	mock_permissions = TPM_NV_PER_PPWRITE;
	TEST_EQ(RollbackKernelRead(&version), TPM_E_CORRUPTED_STATE,
		"RollbackKernelRead() bad uid");

	ResetMocks(0, 0);
	mock_rsk.uid = ROLLBACK_SPACE_KERNEL_UID;
	mock_permissions = TPM_NV_PER_PPWRITE + 1;
	TEST_EQ(RollbackKernelRead(&version), TPM_E_CORRUPTED_STATE,
		"RollbackKernelRead() bad permissions");

	/* Test write */
	ResetMocks(0, 0);
	TEST_EQ(RollbackKernelWrite(0xBEAD4321), 0, "RollbackKernelWrite()");
	TEST_EQ(mock_rsk.kernel_versions, 0xBEAD4321,
		"RollbackKernelWrite() version");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n"
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackKernelWrite(123), TPM_E_IOERROR,
		"RollbackKernelWrite() error");

	/* Test lock (recovery off) */
	ResetMocks(0, 0);
	TEST_EQ(RollbackKernelLock(0), 0, "RollbackKernelLock()");
	TEST_STR_EQ(mock_calls,
		    "TlclLockPhysicalPresence()\n",
		    "tlcl calls");

	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackKernelLock(0), TPM_E_IOERROR,
		"RollbackKernelLock() error");

	/* Test lock with recovery on; shouldn't lock PP */
	SetupTPM(0, 0, 0, &rsf);
	ResetMocks(0, 0);
	TEST_EQ(RollbackKernelLock(1), 0, "RollbackKernelLock() in recovery");
	TEST_STR_EQ(mock_calls, "", "no tlcl calls");
}

/* Tests for RollbackS3Resume() */
static void RollbackS3ResumeTest(void)
{
	ResetMocks(0, 0);
	TEST_EQ(RollbackS3Resume(), 0, "RollbackS3Resume()");
	TEST_STR_EQ(mock_calls,
		    "TlclLibInit()\n"
		    "TlclResume()\n",
		    "tlcl calls");

	/* Should ignore postinit error */
	ResetMocks(2, TPM_E_INVALID_POSTINIT);
	TEST_EQ(RollbackS3Resume(), 0, "RollbackS3Resume() postinit");

	/* Resume with other error */
	ResetMocks(2, TPM_E_IOERROR);
	TEST_EQ(RollbackS3Resume(), TPM_E_IOERROR,
		"RollbackS3Resume() other error");
}

int main(int argc, char* argv[])
{
	CrcTestFirmware();
	CrcTestKernel();
	MiscTest();
	OneTimeInitTest();
	SetupTpmTest();
	RollbackFirmwareTest();
	RollbackKernelTest();
	RollbackS3ResumeTest();

	return gTestSuccess ? 0 : 255;
}
