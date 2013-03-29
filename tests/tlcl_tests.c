/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for TPM lite library
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tss/tcs.h>
/* Don't use the vboot constants, since they conflict with the TCS lib */
#define VBOOT_REFERENCE_TSS_CONSTANTS_H_

#include "host_common.h"
#include "test_common.h"
#include "tlcl.h"
#include "tlcl_internal.h"
#include "vboot_common.h"

/* Mock data */
static char debug_info[4096];
static VbError_t mock_retval;

/* Call to mocked VbExTpmSendReceive() */
struct srcall
{
	const uint8_t *req;  /* Request */
	uint8_t *rsp;  /* Response */
	uint8_t rsp_buf[32];  /* Default response buffer, if not overridden */
	int req_size;  /* Request size */
	uint32_t req_cmd;  /* Request command code */
	int rsp_size;  /* Response size */
	VbError_t retval;  /* Value to return */
};

#define MAXCALLS 8
static struct srcall calls[MAXCALLS];
static int ncalls;

/**
 * Reset mock data (for use before each test)
 */
static void ResetMocks(void)
{
	int i;

	*debug_info = 0;
	mock_retval = VBERROR_SUCCESS;

	memset(calls, 0, sizeof(calls));
	for (i = 0; i < MAXCALLS; i++)
		calls[i].rsp = calls[i].rsp_buf;
	ncalls = 0;
}

/**
 * Set response code and length for call <call_idx>.
 */
static void SetResponse(int call_idx, uint32_t response_code, int rsp_size)
{
	struct srcall *c = calls + call_idx;

	c->rsp_size = rsp_size;
	ToTpmUint32(c->rsp_buf + 6, response_code);
}

/* Mocks */

VbError_t VbExTpmInit(void)
{
	return mock_retval;
}


VbError_t VbExTpmClose(void)
{
	return mock_retval;
}

VbError_t VbExTpmSendReceive(const uint8_t *request, uint32_t request_length,
                             uint8_t *response, uint32_t *response_length)
{
	struct srcall *c = calls + ncalls++;

	c->req = request;
	c->req_size = request_length;

	/* Parse out the command code */
	FromTpmUint32(request + 6, &c->req_cmd);

	// KLUDGE - remove
	printf("TSR [%d] 0x%x\n", ncalls-1, c->req_cmd);

	memset(response, 0, *response_length);
	if (c->rsp_size)
		memcpy(response, c->rsp, c->rsp_size);
	*response_length = c->rsp_size;

	return c->retval;
}

/**
 * Test assorted tlcl functions
 */
static void TlclTest(void)
{
	uint8_t buf[32], buf2[32];

	ResetMocks();
	TEST_EQ(TlclLibInit(), VBERROR_SUCCESS, "Init");

	ResetMocks();
	mock_retval = VBERROR_SIMULATED;
	TEST_EQ(TlclLibInit(), mock_retval, "Init bad");

	ResetMocks();
	TEST_EQ(TlclLibClose(), VBERROR_SUCCESS, "Close");

	ResetMocks();
	mock_retval = VBERROR_SIMULATED;
	TEST_EQ(TlclLibClose(), mock_retval, "Close bad");

	ResetMocks();
	ToTpmUint32(buf + 2, 123);
	TEST_EQ(TlclPacketSize(buf), 123, "TlclPacketSize");

	ResetMocks();
	ToTpmUint32(buf + 2, 10);
	TEST_EQ(TlclSendReceive(buf, buf2, sizeof(buf2)), 0, "SendReceive");
	TEST_PTR_EQ(calls[0].req, buf, "SendReceive req ptr");
	TEST_EQ(calls[0].req_size, 10, "SendReceive size");

	ResetMocks();
	calls[0].retval = VBERROR_SIMULATED;
	ToTpmUint32(buf + 2, 10);
	TEST_EQ(TlclSendReceive(buf, buf2, sizeof(buf2)), VBERROR_SIMULATED,
		"SendReceive fail");

	ResetMocks();
	SetResponse(0, 123, 10);
	ToTpmUint32(buf + 2, 10);
	TEST_EQ(TlclSendReceive(buf, buf2, sizeof(buf2)), 123,
		"SendReceive error response");

	// TODO: continue self test (if needed or doing)
	// TODO: then retry doing self test

}


/**
 * Test send-command functions
 */
static void SendCommandTest(void)
{
	ResetMocks();
	TEST_EQ(TlclStartup(), 0, "SaveState");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Startup, "  cmd");

	ResetMocks();
	TEST_EQ(TlclSaveState(), 0, "SaveState");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_SaveState, "  cmd");

	ResetMocks();
	TEST_EQ(TlclResume(), 0, "Resume");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Startup, "  cmd");

	ResetMocks();
	TEST_EQ(TlclSelfTestFull(), 0, "SelfTestFull");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_SelfTestFull, "  cmd");

	ResetMocks();
	TEST_EQ(TlclContinueSelfTest(), 0, "ContinueSelfTest");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ContinueSelfTest, "  cmd");

	ResetMocks();
	TEST_EQ(TlclAssertPhysicalPresence(), 0,
		"AssertPhysicalPresence");
	TEST_EQ(calls[0].req_cmd, TSC_ORD_PhysicalPresence, "  cmd");

	ResetMocks();
	TEST_EQ(TlclPhysicalPresenceCMDEnable(), 0,
		"PhysicalPresenceCMDEnable");
	TEST_EQ(calls[0].req_cmd, TSC_ORD_PhysicalPresence, "  cmd");

	ResetMocks();
	TEST_EQ(TlclFinalizePhysicalPresence(), 0,
		"FinalizePhysicalPresence");
	TEST_EQ(calls[0].req_cmd, TSC_ORD_PhysicalPresence, "  cmd");

	ResetMocks();
	TEST_EQ(TlclAssertPhysicalPresenceResult(), 0,
		"AssertPhysicalPresenceResult");
	TEST_EQ(calls[0].req_cmd, TSC_ORD_PhysicalPresence, "  cmd");

	ResetMocks();
	TEST_EQ(TlclLockPhysicalPresence(), 0,
		"LockPhysicalPresence");
	TEST_EQ(calls[0].req_cmd, TSC_ORD_PhysicalPresence, "  cmd");

	ResetMocks();
	TEST_EQ(TlclIsOwned(), 0, "IsOwned");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	ResetMocks();
	calls[0].retval = VBERROR_SIMULATED;
	TEST_NEQ(TlclIsOwned(), 0, "IsOwned");

	ResetMocks();
	TEST_EQ(TlclForceClear(), 0, "ForceClear");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ForceClear, "  cmd");

	ResetMocks();
	TEST_EQ(TlclSetEnable(), 0, "SetEnable");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_PhysicalEnable, "  cmd");

	ResetMocks();
	TEST_EQ(TlclClearEnable(), 0, "ClearEnable");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_PhysicalDisable, "  cmd");

	ResetMocks();
	TEST_EQ(TlclSetDeactivated(0), 0, "SetDeactivated");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_PhysicalSetDeactivated, "  cmd");
}

/**
 * NV spaces test
 *
 * TODO: check params/data read/written.
 */
static void ReadWriteTest(void)
{
	uint8_t buf[32];

	ResetMocks();
	TEST_EQ(TlclDefineSpace(1, 2, 3), 0, "DefineSpace");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_DefineSpace, "  cmd");

	ResetMocks();
	TEST_EQ(TlclSetNvLocked(), 0, "SetNvLocked");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_DefineSpace, "  cmd");

	ResetMocks();
	TEST_EQ(TlclWrite(1, buf, 3), 0, "Write");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_WriteValue, "  cmd");

	ResetMocks();
	TEST_EQ(TlclRead(1, buf, 3), 0, "Read");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_ReadValue, "  cmd");

	ResetMocks();
	TEST_EQ(TlclWriteLock(1), 0, "WriteLock");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_WriteValue, "  cmd");

	ResetMocks();
	TEST_EQ(TlclReadLock(1), 0, "ReadLock");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_ReadValue, "  cmd");

	ResetMocks();
	TEST_EQ(TlclSetGlobalLock(), 0, "SetGlobalLock");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_NV_WriteValue, "  cmd");
}

/**
 * Test PCR funcs
 *
 * TODO: check params/data read/written.
 */
static void PcrTest(void)
{
	uint8_t buf[kPcrDigestLength], buf2[kPcrDigestLength];

	ResetMocks();
	TEST_EQ(TlclPCRRead(1, buf, kPcrDigestLength), 0, "PCRRead");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_PcrRead, "  cmd");

	ResetMocks();
	TEST_EQ(TlclPCRRead(1, buf, kPcrDigestLength - 1), TPM_E_IOERROR,
		"PCRRead too small");

	ResetMocks();
	TEST_EQ(TlclExtend(1, buf, buf2), 0, "Extend");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Extend, "  cmd");
}

/**
 * Test flags / capabilities
 *
 * TODO: check params/data read/written.
 */
static void FlagsTest(void)
{
	TPM_PERMANENT_FLAGS pflags;
	TPM_STCLEAR_FLAGS vflags;
	uint8_t disable = 0, deactivated = 0, nvlocked = 0;
	uint32_t u;
	uint8_t buf[32];

	ResetMocks();
	TEST_EQ(TlclGetPermanentFlags(&pflags), 0, "GetPermanentFlags");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");

	ResetMocks();
	TEST_EQ(TlclGetSTClearFlags(&vflags), 0, "GetSTClearFlags");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");

	ResetMocks();
	TEST_EQ(TlclGetFlags(NULL, NULL, NULL), 0, "GetFlags NULL");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");
	ResetMocks();
	TEST_EQ(TlclGetFlags(&disable, &deactivated, &nvlocked), 0, "GetFlags");

	ResetMocks();
	TEST_EQ(TlclGetPermissions(1, &u), 0, "GetPermissions");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");

	ResetMocks();
	TEST_EQ(TlclGetOwnership(buf), 0, "GetOwnership");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");
}

/**
 * Test random
 *
 * TODO: check params/data read/written.
 * TODO: check overflow tests.
 */
static void RandomTest(void)
{
	uint8_t buf[32];
	uint32_t size;

	ResetMocks();
	size = sizeof(buf);
	TEST_EQ(TlclGetRandom(buf, sizeof(buf), &size), 0, "GetRandom");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetRandom, "  cmd");
	TEST_EQ(size, 0, "  size 0");
}

int main(void)
{
	TlclTest();
	SendCommandTest();
	ReadWriteTest();
	PcrTest();
	FlagsTest();
	RandomTest();

	return gTestSuccess ? 0 : 255;
}
