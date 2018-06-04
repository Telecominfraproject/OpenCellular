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

#include "2crc8.h"
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

static union {
	struct RollbackSpaceFwmp fwmp;
	uint8_t buf[FWMP_NV_MAX_SIZE];
} mock_fwmp;

static uint32_t mock_permissions;

/* Recalculate CRC of FWMP data */
static void RecalcFwmpCrc(void)
{
	mock_fwmp.fwmp.crc = vb2_crc8(mock_fwmp.buf + 2,
				  mock_fwmp.fwmp.struct_size - 2);
}

/* Reset the variables for the Tlcl mock functions. */
static void ResetMocks(int fail_on_call, uint32_t fail_with_err)
{
	*mock_calls = 0;
	mock_cnext = mock_calls;
	mock_count = 0;
	fail_at_count = fail_on_call;
	fail_with_error = fail_with_err;
	noise_count = 0;
	memset(&noise_on, 0, sizeof(noise_on));

	memset(&mock_pflags, 0, sizeof(mock_pflags));
	memset(&mock_rsf, 0, sizeof(mock_rsf));
	memset(&mock_rsk, 0, sizeof(mock_rsk));
	mock_permissions = 0;

	memset(mock_fwmp.buf, 0, sizeof(mock_fwmp.buf));
	mock_fwmp.fwmp.struct_size = sizeof(mock_fwmp.fwmp);
	mock_fwmp.fwmp.struct_version = ROLLBACK_SPACE_FWMP_VERSION;
	mock_fwmp.fwmp.flags = 0x1234;
	/* Put some data in the hash */
	mock_fwmp.fwmp.dev_key_hash[0] = 0xaa;
	mock_fwmp.fwmp.dev_key_hash[FWMP_HASH_SIZE - 1] = 0xbb;
	RecalcFwmpCrc();
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
		memcpy(data, &mock_rsf, length);
		MaybeInjectNoise(data, length);
	} else if (KERNEL_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsk), "TlclRead rsk size");
		memcpy(data, &mock_rsk, length);
		MaybeInjectNoise(data, length);
	} else if (FWMP_NV_INDEX == index) {
		memset(data, 0, length);
		if (length > sizeof(mock_fwmp))
			length = sizeof(mock_fwmp);
		memcpy(data, &mock_fwmp, length);
		MaybeInjectNoise(data, length);
	} else {
		memset(data, 0, length);
	}

	return (++mock_count == fail_at_count) ? fail_with_error : TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void *data, uint32_t length)
{
	mock_cnext += sprintf(mock_cnext, "TlclWrite(0x%x, %d)\n",
			      index, length);

	if (FIRMWARE_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsf), "TlclWrite rsf size");
		memcpy(&mock_rsf, data, length);
		MaybeInjectNoise(&mock_rsf, length);
	} else if (KERNEL_NV_INDEX == index) {
		TEST_EQ(length, sizeof(mock_rsk), "TlclWrite rsk size");
		memcpy(&mock_rsk, data, length);
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
	memcpy(pflags, &mock_pflags, sizeof(mock_pflags));
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
	mock_rsf.crc8 = vb2_crc8(&mock_rsf,
				 offsetof(RollbackSpaceFirmware, crc8));
	noise_on[0] = 1;
	TEST_EQ(ReadSpaceFirmware(&rsf), 0,
		"ReadSpaceFirmware(), v2, good CRC");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* A write with version < 2 should convert to v2 and create the CRC */
	ResetMocks(0, 0);
	memset(&rsf, 0, sizeof(rsf));
	TEST_EQ(WriteSpaceFirmware(&rsf), 0, "WriteSpaceFirmware(), v0");
	TEST_EQ(mock_rsf.struct_version, 2, "WriteSpaceFirmware(), check v2");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1007, 10)\n"
		    "TlclRead(0x1007, 10)\n",
		    "tlcl calls");

	/* Same as above, but with some noise during the readback */
	ResetMocks(0, 0);
	memset(&rsf, 0, sizeof(rsf));
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
	memset(&rsf, 0, sizeof(rsf));
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
	memset(&rsf, 0, sizeof(rsf));
	memset(noise_on, 1, sizeof(noise_on));
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
	mock_rsk.crc8 = vb2_crc8(&mock_rsk,
				 offsetof(RollbackSpaceKernel, crc8));
	noise_on[0] = 1;
	TEST_EQ(ReadSpaceKernel(&rsk), 0, "ReadSpaceKernel(), v2, good CRC");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* A write with version < 2 should convert to v2 and create the CRC */
	ResetMocks(0, 0);
	memset(&rsk, 0, sizeof(rsk));
	TEST_EQ(WriteSpaceKernel(&rsk), 0, "WriteSpaceKernel(), v0");
	TEST_EQ(mock_rsk.struct_version, 2, "WriteSpaceKernel(), check v2");
	TEST_STR_EQ(mock_calls,
		    "TlclWrite(0x1008, 13)\n"
		    "TlclRead(0x1008, 13)\n",
		    "tlcl calls");

	/* Same as above, but with some noise during the readback */
	ResetMocks(0, 0);
	memset(&rsk, 0, sizeof(rsk));
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
	memset(&rsk, 0, sizeof(rsk));
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
	memset(&rsk, 0, sizeof(rsk));
	memset(noise_on, 1, sizeof(noise_on));
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
}

/****************************************************************************/
/* Tests for RollbackKernel() calls */

static void RollbackKernelTest(void)
{
	uint32_t version = 0;

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
	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackKernelLock(0), TPM_E_IOERROR,
		"RollbackKernelLock() error");

	/* Test lock with recovery on; shouldn't lock PP */
	ResetMocks(0, 0);
	TEST_EQ(RollbackKernelLock(1), 0, "RollbackKernelLock() in recovery");
	TEST_STR_EQ(mock_calls, "", "no tlcl calls");

	ResetMocks(0, 0);
	TEST_EQ(RollbackKernelLock(0), 0, "RollbackKernelLock()");
	TEST_STR_EQ(mock_calls,
		    "TlclLockPhysicalPresence()\n",
		    "tlcl calls");
}

/****************************************************************************/
/* Tests for RollbackFwmpRead() calls */

static void RollbackFwmpTest(void)
{
	struct RollbackSpaceFwmp fwmp;
	struct RollbackSpaceFwmp fwmp_zero = {0};

	/* Normal read */
	ResetMocks(0, 0);
	TEST_EQ(RollbackFwmpRead(&fwmp), 0, "RollbackFwmpRead()");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x100a, 40)\n",
		    "  tlcl calls");
	TEST_EQ(0, memcmp(&fwmp, &mock_fwmp, sizeof(fwmp)), "  data");

	/* Read error */
	ResetMocks(1, TPM_E_IOERROR);
	TEST_EQ(RollbackFwmpRead(&fwmp), TPM_E_IOERROR,
		"RollbackFwmpRead() error");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x100a, 40)\n",
		    "  tlcl calls");
	TEST_EQ(0, memcmp(&fwmp, &fwmp_zero, sizeof(fwmp)), "  data clear");

	/* Not present isn't an error; just returns empty data */
	ResetMocks(1, TPM_E_BADINDEX);
	TEST_EQ(RollbackFwmpRead(&fwmp), 0, "RollbackFwmpRead() not present");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x100a, 40)\n",
		    "  tlcl calls");
	TEST_EQ(0, memcmp(&fwmp, &fwmp_zero, sizeof(fwmp)), "  data clear");

	/* Struct size too small */
	ResetMocks(0, 0);
	mock_fwmp.fwmp.struct_size--;
	TEST_EQ(RollbackFwmpRead(&fwmp), TPM_E_STRUCT_SIZE,
		"RollbackFwmpRead() too small");

	/* Struct size too large with good CRC */
	ResetMocks(0, 0);
	mock_fwmp.fwmp.struct_size += 4;
	RecalcFwmpCrc();
	TEST_EQ(RollbackFwmpRead(&fwmp), 0, "RollbackFwmpRead() bigger");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x100a, 40)\n"
		    "TlclRead(0x100a, 44)\n",
		    "  tlcl calls");
	TEST_EQ(0, memcmp(&fwmp, &mock_fwmp, sizeof(fwmp)), "  data");

	/* Bad CRC causes retry, then eventual failure */
	ResetMocks(0, 0);
	mock_fwmp.fwmp.crc++;
	TEST_EQ(RollbackFwmpRead(&fwmp), TPM_E_CORRUPTED_STATE,
		"RollbackFwmpRead() crc");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x100a, 40)\n"
		    "TlclRead(0x100a, 40)\n"
		    "TlclRead(0x100a, 40)\n",
		    "  tlcl calls");

	/* Struct size too large with bad CRC */
	ResetMocks(0, 0);
	mock_fwmp.fwmp.struct_size += 4;
	RecalcFwmpCrc();
	mock_fwmp.fwmp.crc++;
	TEST_EQ(RollbackFwmpRead(&fwmp), TPM_E_CORRUPTED_STATE,
		"RollbackFwmpRead() bigger crc");
	TEST_STR_EQ(mock_calls,
		    "TlclRead(0x100a, 40)\n"
		    "TlclRead(0x100a, 44)\n"
		    "TlclRead(0x100a, 40)\n"
		    "TlclRead(0x100a, 44)\n"
		    "TlclRead(0x100a, 40)\n"
		    "TlclRead(0x100a, 44)\n",
		    "  tlcl calls");
	TEST_EQ(0, memcmp(&fwmp, &fwmp_zero, sizeof(fwmp)), "  data");

	/* Minor version difference ok */
	ResetMocks(0, 0);
	mock_fwmp.fwmp.struct_version++;
	RecalcFwmpCrc();
	TEST_EQ(RollbackFwmpRead(&fwmp), 0, "RollbackFwmpRead() minor version");
	TEST_EQ(0, memcmp(&fwmp, &mock_fwmp, sizeof(fwmp)), "  data");

	/* Major version difference not ok */
	ResetMocks(0, 0);
	mock_fwmp.fwmp.struct_version += 0x10;
	RecalcFwmpCrc();
	TEST_EQ(RollbackFwmpRead(&fwmp), TPM_E_STRUCT_VERSION,
		"RollbackFwmpRead() major version");
}

int main(int argc, char* argv[])
{
	CrcTestFirmware();
	CrcTestKernel();
	MiscTest();
	RollbackKernelTest();
	RollbackFwmpTest();

	return gTestSuccess ? 0 : 255;
}
