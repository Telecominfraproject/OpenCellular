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

VbError_t VbExTpmGetRandom(uint8_t *buf, uint32_t length)
{
	memset(buf, 0xa5, length);
	return VBERROR_SUCCESS;
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
 * Test DefineSpaceEx
 */
void DefineSpaceExTest(void) {
	uint8_t osap_response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00,
		0x00, 0x00, 0x02, 0x41, 0x3d, 0xce, 0x20, 0xa2,
		0x5a, 0xa5, 0x95, 0xbe, 0x26, 0xe8, 0x76, 0x74,
		0x6c, 0x61, 0xf7, 0xa7, 0x24, 0x17, 0xa1, 0x06,
		0xcf, 0x53, 0x6d, 0xd4, 0x26, 0x98, 0x68, 0x86,
		0xe6, 0xf6, 0x62, 0x58, 0xdb, 0xa2, 0x9f, 0x5b,
		0x18, 0xa6, 0xae, 0x36, 0x32, 0x5d,
	};
	uint8_t define_space_response[] = {
		0x00, 0xc5, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00,
		0x00, 0x00, 0x42, 0xe6, 0x38, 0xc6, 0x37, 0x2a,
		0xf2, 0xfe, 0xb4, 0x01, 0x4b, 0x29, 0x63, 0x30,
		0x4e, 0x2f, 0x2e, 0x74, 0x58, 0xcd, 0x00, 0x40,
		0x42, 0x10, 0x40, 0xac, 0x93, 0x0c, 0xff, 0x8a,
		0xc4, 0x98, 0x78, 0xe3, 0xfe, 0x48, 0x5b, 0xb7,
		0xc8, 0x8d, 0xf4,
	};
	uint8_t owner_secret[TPM_AUTH_DATA_LEN] = { 0 };
	TPM_NV_AUTH_POLICY policy;

	ResetMocks();
	calls[0].rsp = osap_response;
	calls[0].rsp_size = sizeof(osap_response);
	calls[1].rsp = define_space_response;
	calls[1].rsp_size = sizeof(define_space_response);
	TEST_EQ(TlclDefineSpaceEx(owner_secret, sizeof(owner_secret),
				  0x20000005, 0x2000, 0x10, NULL, 0),
		TPM_SUCCESS, "DefineSpace");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OSAP, "  osap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_NV_DefineSpace, "  definespace cmd");

	/* Pass an auth policy. */
	ResetMocks();
	calls[0].rsp = osap_response;
	calls[0].rsp_size = sizeof(osap_response);
	calls[1].rsp = define_space_response;
	calls[1].rsp_size = sizeof(define_space_response);
	TEST_EQ(TlclDefineSpaceEx(owner_secret, sizeof(owner_secret),
				  0x20000005, 0x2000, 0x10, &policy,
				  sizeof(policy)),
		TPM_SUCCESS, "DefineSpace");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OSAP, "  osap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_NV_DefineSpace, "  definespace cmd");

	/* Verify that the response gets authenticated. */
	ResetMocks();
	calls[0].rsp = osap_response;
	calls[0].rsp_size = sizeof(osap_response);
	calls[1].rsp = define_space_response;
	calls[1].rsp_size = sizeof(define_space_response);
	define_space_response[31] = 0;
	TEST_EQ(TlclDefineSpaceEx(owner_secret, sizeof(owner_secret),
				  0x20000005, 0x2000, 0x10, NULL, 0),
		TPM_E_AUTHFAIL, "DefineSpace - response auth");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OSAP, "  osap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_NV_DefineSpace, "  definespace cmd");
	define_space_response[31] = 0x40;

	/* Verify that a short OSAP response gets caught. */
	ResetMocks();
	calls[0].rsp = osap_response;
	calls[0].rsp_size = sizeof(osap_response);
	ToTpmUint32(osap_response + sizeof(uint16_t),
		    kTpmRequestHeaderLength + sizeof(uint32_t) +
		    2 * sizeof(TPM_NONCE) - 1);
	TEST_EQ(TlclDefineSpaceEx(owner_secret, sizeof(owner_secret),
				  0x20000005, 0x2000, 0x10, NULL, 0),
		TPM_E_INVALID_RESPONSE, "DefineSpace - short OSAP response");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OSAP, "  osap cmd");
	ToTpmUint32(osap_response + sizeof(uint16_t), sizeof(osap_response));

	/* Verify that a short DefineSpace response gets caught. */
	ResetMocks();
	calls[0].rsp = osap_response;
	calls[0].rsp_size = sizeof(osap_response);
	calls[1].rsp = define_space_response;
	calls[1].rsp_size = sizeof(define_space_response);
	ToTpmUint32(define_space_response + sizeof(uint16_t),
		    kTpmResponseHeaderLength + kTpmResponseAuthBlockLength - 1);
	TEST_EQ(TlclDefineSpaceEx(owner_secret, sizeof(owner_secret),
				  0x20000005, 0x2000, 0x10, NULL, 0),
		TPM_E_INVALID_RESPONSE,
		"DefineSpace - short DefineSpace response");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OSAP, "  osap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_NV_DefineSpace, "  definespace cmd");
	ToTpmUint32(define_space_response + sizeof(uint16_t),
		    sizeof(define_space_response));
}

/**
 * Test TlclInitNvAuthPolicy.
 */
void InitNvAuthPolicyTest(void) {
	const uint8_t empty_selection_digest[] = {
		0x79, 0xdd, 0xda, 0xfd, 0xc1, 0x97, 0xdc, 0xcc,
		0xe9, 0x98, 0x9a, 0xee, 0xf5, 0x52, 0x89, 0xee,
		0x24, 0x96, 0x4c, 0xac,
	};
	const uint8_t pcr0_selection_digest[] = {
		0xb3, 0x2b, 0x96, 0x30, 0xd3, 0x21, 0x1e, 0x99,
		0x78, 0x9e, 0xd3, 0x1f, 0x11, 0x8e, 0x96, 0xbc,
		0xf7, 0x7e, 0x7b, 0x06,
	};
	const uint8_t empty_selection_encoding[] = { 0x0, 0x0, 0x0 };
	const uint8_t pcr0_selection_encoding[] = { 0x1, 0x0, 0x0 };
	const uint8_t pcr_values[][TPM_PCR_DIGEST] = {
		{ 0x06, 0x4a, 0xec, 0x9b, 0xbd, 0x94, 0xde, 0xa1,
		  0x23, 0x1a, 0xe7, 0x57, 0x67, 0x64, 0x7f, 0x09,
		  0x8c, 0x39, 0x8e, 0x79, },
	};
	TPM_NV_AUTH_POLICY policy;

	/* Test empty selection. */
	uint32_t policy_size = sizeof(policy);
	TlclInitNvAuthPolicy(0x0, NULL, &policy, &policy_size);
	TEST_EQ(policy_size, sizeof(policy), "policy size");

	uint16_t size_of_select;
	FromTpmUint16(
		(uint8_t*)&policy.pcr_info_read.pcrSelection.sizeOfSelect,
		&size_of_select);
	TEST_EQ(size_of_select, 3, "empty PCR selection read size of select");
	TEST_EQ(memcmp(policy.pcr_info_read.pcrSelection.pcrSelect,
		       empty_selection_encoding,
		       sizeof(empty_selection_encoding)), 0,
		"empty PCR selection read selection encoding");
	TEST_EQ(policy.pcr_info_read.localityAtRelease,
		TPM_ALL_LOCALITIES & ~TPM_LOC_THREE,
		"empty PCR selection read locality");
	TEST_EQ(memcmp(empty_selection_digest,
		       policy.pcr_info_read.digestAtRelease.digest,
		       TPM_PCR_DIGEST),
		0, "empty PCR selection read digest");

	FromTpmUint16(
		(uint8_t*)&policy.pcr_info_write.pcrSelection.sizeOfSelect,
		&size_of_select);
	TEST_EQ(size_of_select, 3, "empty PCR selection write size of select");
	TEST_EQ(memcmp(policy.pcr_info_write.pcrSelection.pcrSelect,
		       empty_selection_encoding,
		       sizeof(empty_selection_encoding)), 0,
		"empty PCR selection write selection encoding");
	TEST_EQ(policy.pcr_info_write.localityAtRelease,
		TPM_ALL_LOCALITIES & ~TPM_LOC_THREE,
		"empty PCR selection write locality");
	TEST_EQ(memcmp(empty_selection_digest,
		       policy.pcr_info_write.digestAtRelease.digest,
		       TPM_PCR_DIGEST),
		0, "empty PCR selection write digest");

	/* Test PCR0 selection. */
	TlclInitNvAuthPolicy(0x1, pcr_values, &policy, &policy_size);
	TEST_EQ(policy_size, sizeof(policy), "policy size");

	TEST_EQ(memcmp(policy.pcr_info_read.pcrSelection.pcrSelect,
		       pcr0_selection_encoding,
		       sizeof(pcr0_selection_encoding)), 0,
		"PCR0 selection read selection encoding");
	TEST_EQ(memcmp(pcr0_selection_digest,
		       policy.pcr_info_read.digestAtRelease.digest,
		       TPM_PCR_DIGEST),
		0, "PCR0 selection read digest");

	TEST_EQ(memcmp(policy.pcr_info_write.pcrSelection.pcrSelect,
		       pcr0_selection_encoding,
		       sizeof(pcr0_selection_encoding)), 0,
		"PCR0 selection write selection encoding");
	TEST_EQ(memcmp(pcr0_selection_digest,
		       policy.pcr_info_write.digestAtRelease.digest,
		       TPM_PCR_DIGEST),
		0, "PCR0 selection write digest");
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
 * Test TlclGetSpaceInfo.
 */
static void GetSpaceInfoTest(void)
{
	uint8_t response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x18,
		0x20, 0x00, 0x00, 0x04, 0x00, 0x03, 0x01, 0x00,
		0x00, 0x01, 0xb3, 0x2b, 0x96, 0x30, 0xd3, 0x21,
		0x1e, 0x99, 0x78, 0x9e, 0xd3, 0x1f, 0x11, 0x8e,
		0x96, 0xbc, 0xf7, 0x7e, 0x7b, 0x06, 0x00, 0x03,
		0x20, 0x00, 0x00, 0x10, 0x3b, 0xb2, 0x69, 0x03,
		0x3d, 0x12, 0xe1, 0x99, 0x87, 0xe9, 0x3d, 0xf1,
		0x11, 0xe8, 0x69, 0xcb, 0x7f, 0xe7, 0xb7, 0x60,
		0x00, 0x17, 0x00, 0x00, 0x20, 0x00, 0x00, 0x01,
		0x01, 0x00, 0x00, 0x00, 0x45,
	};

	uint32_t attributes = 0;
	uint32_t size = 0;
	TPM_NV_AUTH_POLICY policy;
	uint32_t policy_size = sizeof(policy);

	/* Test successful parsing. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclGetSpaceInfo(0x20000004, &attributes, &size, &policy,
				 &policy_size),
		TPM_SUCCESS, "GetSpaceInfo");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");
	TEST_EQ(policy_size, sizeof(policy), "  policy_size");
	TEST_EQ(attributes, TPM_NV_PER_WRITEDEFINE, "  attributes");
	TEST_EQ(size, 0x45, "  size");
	TEST_EQ(memcmp(&policy, response + 20, sizeof(policy)), 0, "  policy");

	/* Test that GetPermissions returns the attributes as well. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclGetPermissions(0x20000004, &attributes),
		TPM_SUCCESS, "GetPermissions");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");
	TEST_EQ(attributes, TPM_NV_PER_WRITEDEFINE, "  attributes");

	/* Test whether a short response gets detected. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + 10, 0x46);
	TEST_EQ(TlclGetSpaceInfo(0x20000004, &attributes, &size, &policy,
				 &policy_size),
		TPM_E_INVALID_RESPONSE, "GetSpaceInfo short length");
	ToTpmUint32(response + 10, 0x47);

	/* Test whether an overlong PCR selection length causes failure. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint16(response + 20, 4);
	TEST_EQ(TlclGetSpaceInfo(0x20000004, &attributes, &size, &policy,
				 &policy_size),
		TPM_E_INVALID_RESPONSE, "GetSpaceInfo overlong pcr selection");
	ToTpmUint16(response + 20, 3);

	/* Test that a short policy buffer triggers an error. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	policy_size = sizeof(policy) - 1;
	TEST_EQ(TlclGetSpaceInfo(0x20000004, &attributes, &size, &policy,
				 &policy_size),
		TPM_E_BUFFER_SIZE, "GetSpaceInfo short policy buffer");
	TEST_EQ(sizeof(policy), policy_size, "  policy_size");
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

/**
 * Test GetVersion
 */
static void GetVersionTest(void)
{
	uint8_t response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x30,
		0x01, 0x02, 0x04, 0x20, 0x00, 0x02, 0x03, 0x49,
		0x46, 0x58, 0x00, 0x00, 0x0d, 0x04, 0x20, 0x03,
		0x6f, 0x00, 0x74, 0x70, 0x6d, 0x33, 0x38, 0xff,
		0xff, 0xff
	};

	uint32_t vendor;
	uint64_t firmware_version;
	uint8_t vendor_specific[32];
	size_t vendor_specific_size;

	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version, NULL, NULL), 0,
		"GetVersion");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");
	TEST_EQ(vendor, 0x49465800, "  vendor");
	TEST_EQ(firmware_version, 0x420, "  firmware_version");

	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	vendor_specific_size = 100;
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version,
		NULL, &vendor_specific_size), 0,
		"GetVersion - vendor specific size");
	TEST_EQ(vendor_specific_size, 0xd, "  vendor specific size");

	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	vendor_specific_size = sizeof(vendor_specific);
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version,
		vendor_specific, &vendor_specific_size), 0,
		"GetVersion - vendor specific data");
	TEST_EQ(vendor_specific_size, 0xd, "  vendor specific size");
	TEST_EQ(memcmp(vendor_specific, response + 29, 0xd), 0,
		"  vendor specific data check");

	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	vendor_specific_size = 4;
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version,
		vendor_specific, &vendor_specific_size), 0,
		"GetVersion - vendor specific data, short buf");
	TEST_EQ(vendor_specific_size, 4,
		"  min(vendor specific size, buf size)");
	TEST_EQ(memcmp(vendor_specific, response + 29, 4), 0,
		"  vendor specific data check");

	ResetMocks();
	SetResponse(0, TPM_E_IOERROR, 0);
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version, NULL, NULL),
		TPM_E_IOERROR, "GetVersion - error");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");

	/* Adjust response to indicate a 1 byte too short payload size. */
	ToTpmUint32(response + kTpmResponseHeaderLength, 14);
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version, NULL, NULL),
		TPM_E_IOERROR, "GetVersion -- short");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");

	/* Adjust response to indicate a payload size too long for the
	 * response buffer. */
	ToTpmUint32(response + kTpmResponseHeaderLength,
			TPM_LARGE_ENOUGH_COMMAND_SIZE - sizeof(uint32_t) -
			kTpmResponseHeaderLength + 1);
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version, NULL, NULL),
		TPM_E_IOERROR, "GetVersion -- long");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");

	/* Restore the original payload length and adjust response to contain
	 * less vendor specific data than indicated in its size. */
	ToTpmUint32(response + kTpmResponseHeaderLength, 0x1c);
	ToTpmUint16(response + 27, 0xd + 1);
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclGetVersion(&vendor, &firmware_version,
		NULL, &vendor_specific_size), TPM_E_IOERROR,
		"GetVersion -- short with vendor specific");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_GetCapability, "  cmd");
}

/**
 * Test IFX FieldUpgradeInfoRequest2
 */
static void IFXFieldUpgradeInfoTest(void)
{
	uint8_t version_response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x30,
		0x01, 0x02, 0x04, 0x20, 0x00, 0x02, 0x03, 0x49,
		0x46, 0x58, 0x00, 0x00, 0x0d, 0x04, 0x20, 0x03,
		0x6f, 0x00, 0x74, 0x70, 0x6d, 0x33, 0x38, 0xff,
		0xff, 0xff
	};
	uint8_t upgrade_info_response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x6a, 0x03, 0x02, 0x04, 0x9c,
		0x04, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x08,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x01,
		0x01, 0x00, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x00,
		0x00, 0x00, 0x04, 0x01, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xff, 0xff, 0xee, 0xee, 0x5a, 0x3c,
		0x04, 0x01, 0x02, 0x00, 0x00, 0x00, 0x08, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x3f
	};

	ResetMocks();
	calls[0].rsp = version_response;
	calls[0].rsp_size = sizeof(version_response);
	calls[1].rsp = upgrade_info_response;
	calls[1].rsp_size = sizeof(upgrade_info_response);
	TPM_IFX_FIELDUPGRADEINFO info;
	TEST_EQ(TlclIFXFieldUpgradeInfo(&info), 0, "IFXFieldUpgradeInfo");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_FieldUpgrade, "  cmd");
	TEST_EQ(info.wMaxDataSize, 1180, "  wMaxDatasize");
	TEST_EQ(info.sBootloaderFirmwarePackage.FwPackageIdentifier, 0x50100,
		"  bootloader FWPackageIdeintifier");
	TEST_EQ(info.sBootloaderFirmwarePackage.Version, 0xffff,
		"  bootloader Version");
	TEST_EQ(info.sBootloaderFirmwarePackage.StaleVersion, 0x0,
		"  bootloader StaleVersion");
	TEST_EQ(info.sFirmwarePackages[0].FwPackageIdentifier, 0x4010100,
		"  fw[0] FWPackageIdeintifier");
	TEST_EQ(info.sFirmwarePackages[0].Version, 0xbe,
		"  fw[0] Version");
	TEST_EQ(info.sFirmwarePackages[0].StaleVersion, 0x0,
		"  fw[0] StaleVersion");
	TEST_EQ(info.sFirmwarePackages[1].FwPackageIdentifier, 0x4010200,
		"  fw[1] FWPackageIdeintifier");
	TEST_EQ(info.sFirmwarePackages[1].Version, 0x0,
		"  fw[1] Version");
	TEST_EQ(info.sFirmwarePackages[1].StaleVersion, 0xffffeeee,
		"  fw[1] StaleVersion");
	TEST_EQ(info.wSecurityModuleStatus, 0x5a3c, "  wSecurityModuleStatus");
	TEST_EQ(info.sProcessFirmwarePackage.FwPackageIdentifier, 0x4010200,
		"  process FWPackageIdeintifier");
	TEST_EQ(info.sProcessFirmwarePackage.Version, 0x832,
		"  process Version");
	TEST_EQ(info.sProcessFirmwarePackage.StaleVersion, 0x0,
		"  process StaleVersion");
	TEST_EQ(info.wFieldUpgradeCounter, 0x3f, "  wFieldUpgradeCounter");

	ResetMocks();
	calls[0].rsp = version_response;
	calls[0].rsp_size = sizeof(version_response);
	SetResponse(1, TPM_E_IOERROR, sizeof(upgrade_info_response) - 1);
	TEST_EQ(TlclIFXFieldUpgradeInfo(&info), TPM_E_IOERROR,
		"IFXFieldUpgradeInfo - error");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_FieldUpgrade, "  cmd");

	/* Adjust response to indicate a 1 byte too short payload size. */
	ToTpmUint16(upgrade_info_response + kTpmRequestHeaderLength,
		    sizeof(upgrade_info_response) - kTpmRequestHeaderLength -
		    sizeof(uint16_t) - 1);
	ResetMocks();
	calls[0].rsp = version_response;
	calls[0].rsp_size = sizeof(version_response);
	calls[1].rsp = upgrade_info_response;
	calls[1].rsp_size = sizeof(upgrade_info_response);
	TEST_EQ(TlclIFXFieldUpgradeInfo(&info), TPM_E_INVALID_RESPONSE,
		"IFXFieldUpgradeInfo - short");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_FieldUpgrade, "  cmd");

	/* Adjust version response to claim a non-IFX vendor. */
	ToTpmUint32(version_response + kTpmResponseHeaderLength +
		    sizeof(uint32_t), 0);
	ResetMocks();
	calls[0].rsp = version_response;
	calls[0].rsp_size = sizeof(version_response);
	TEST_EQ(TlclIFXFieldUpgradeInfo(&info), TPM_E_INVALID_RESPONSE,
		"IFXFieldUpgradeInfo - bad vendor");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_FieldUpgrade, "  cmd");
}

/**
 * Test ReadPubek
 */
void ReadPubekTest(void) {
	uint8_t response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x01, 0x3a, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
		0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
		0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x9c, 0xa8,
		0x8f, 0x15, 0x6d, 0xbf, 0x37, 0x6f, 0x8d, 0xb7,
		0xb2, 0xe2, 0x79, 0x81, 0xf7, 0xc2, 0x3c, 0x84,
		0x64, 0x35, 0x38, 0x59, 0x6a, 0x87, 0x23, 0xe0,
		0x2a, 0xca, 0x40, 0x37, 0x78, 0x75, 0x18, 0xfc,
		0x2d, 0xbe, 0x2b, 0xd9, 0x77, 0x49, 0x4b, 0x8c,
		0xea, 0xe3, 0xcd, 0xa5, 0x27, 0x2b, 0x48, 0x2f,
		0xbe, 0x3b, 0x14, 0xca, 0xe5, 0x22, 0x31, 0xb8,
		0xa1, 0x14, 0xc6, 0x06, 0x8d, 0x14, 0xe8, 0x4a,
		0x36, 0x4d, 0xd3, 0x5f, 0xde, 0x23, 0xd1, 0x7b,
		0xec, 0x3b, 0xdd, 0x84, 0xa4, 0xd4, 0xe9, 0x9a,
		0x89, 0x94, 0x5c, 0xa0, 0x01, 0xfb, 0x95, 0x61,
		0x01, 0xaf, 0x6b, 0x17, 0x39, 0x44, 0x7e, 0x25,
		0x9b, 0x73, 0xfa, 0xde, 0x20, 0xb6, 0x92, 0x95,
		0x7b, 0x76, 0x9f, 0x44, 0xb0, 0xfb, 0x86, 0x9f,
		0xc5, 0xe2, 0x38, 0x9d, 0xcf, 0xce, 0x32, 0x53,
		0x3b, 0x3d, 0x10, 0xb7, 0x89, 0x53, 0x54, 0xc0,
		0x28, 0x52, 0xfc, 0x7a, 0xa9, 0x5b, 0x03, 0xd7,
		0x55, 0x2b, 0x07, 0xa7, 0x22, 0x5b, 0x1e, 0xe5,
		0x4f, 0x43, 0x70, 0x7d, 0x81, 0xbb, 0x3d, 0x56,
		0xfe, 0x5c, 0x47, 0xcc, 0xc0, 0x6e, 0xc8, 0xc7,
		0xd9, 0x96, 0x53, 0x2d, 0xd2, 0x28, 0xc7, 0xdf,
		0x5a, 0x07, 0x50, 0x3b, 0x17, 0x25, 0xe7, 0x51,
		0xed, 0xf7, 0x94, 0x02, 0x2a, 0x4c, 0x31, 0x57,
		0x34, 0x51, 0x05, 0x26, 0x43, 0xd4, 0x40, 0x47,
		0x3e, 0x02, 0xfe, 0xa5, 0x08, 0xc7, 0x94, 0xaa,
		0xd3, 0x14, 0x02, 0x1b, 0x41, 0x4c, 0xcd, 0xd6,
		0x8c, 0xad, 0x8e, 0x72, 0x1a, 0x36, 0xc7, 0x23,
		0xd1, 0x38, 0x83, 0x9f, 0xac, 0x66, 0xc5, 0x25,
		0x82, 0x9d, 0x18, 0x67, 0x78, 0xca, 0x15, 0x63,
		0x15, 0xd8, 0x83, 0xbd, 0xcc, 0xe7, 0xf7, 0xe9,
		0xba, 0xda, 0x23, 0xdf, 0x53, 0x30, 0x51, 0x1a,
		0xf1, 0x0c, 0x02, 0xe7, 0x65, 0x0d, 0x95, 0x52,
		0x76, 0xd1, 0x1b, 0xb0, 0x77, 0xba, 0x31, 0x0b,
		0x17, 0xb5, 0x63, 0x0d, 0x50, 0x7c, 0xbc, 0x63,
		0xbf, 0xc3,
	};
	uint32_t exponent = 0;
	uint8_t modulus[TPM_RSA_2048_LEN];
	uint32_t modulus_size = TPM_RSA_2048_LEN;

	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	TEST_EQ(TlclReadPubek(&exponent, modulus, &modulus_size), TPM_SUCCESS,
		"ReadPubek");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	TEST_EQ(exponent, 0x10001, "  exponent");
	TEST_EQ(memcmp(modulus, response + 38, sizeof(modulus)), TPM_SUCCESS,
		"  modulus");
	TEST_EQ(modulus_size, 0x100, "  modulus_size");

	/* Test that the command returns the full size of the modulus if the
	 * input buffer is too small. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	modulus_size = 0;
	TEST_EQ(TlclReadPubek(&exponent, NULL, &modulus_size),
		TPM_E_BUFFER_SIZE, "ReadPubek - returns modulus size");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	TEST_EQ(modulus_size, 0x100, "  modulus_size");

	/* Test that a too large parm_size value gets handled correctly. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + 18, 1 << 24);
	TEST_EQ(TlclReadPubek(&exponent, NULL, &modulus_size),
		TPM_E_INVALID_RESPONSE, "ReadPubek - large parm_size");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	ToTpmUint32(response + 18, 12);

	/* Test that a too small parm_size value gets handled correctly. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + 18, 11);
	TEST_EQ(TlclReadPubek(&exponent, NULL, &modulus_size),
		TPM_E_INVALID_RESPONSE, "ReadPubek - small parm_size");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	ToTpmUint32(response + 18, 12);

	/* Test that an overlong modulus size gets handled correctly. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + 34, 1 << 24);
	modulus_size = sizeof(modulus);
	TEST_EQ(TlclReadPubek(&exponent, NULL, &modulus_size),
		TPM_E_INVALID_RESPONSE, "ReadPubek - large modulus size");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	ToTpmUint32(response + 34, TPM_RSA_2048_LEN);

	/* Test that a large exponent size gets handled correctly. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + 20, 1 << 24);
	TEST_EQ(TlclReadPubek(&exponent, NULL, &modulus_size),
		TPM_E_INVALID_RESPONSE, "ReadPubek - large exponent size");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	ToTpmUint32(response + 20, 0);

	/* Test that an incorrect auth tag causes an error. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	response[294] = 0;
	TEST_EQ(TlclReadPubek(&exponent, NULL, &modulus_size),
		TPM_E_INVALID_RESPONSE, "ReadPubek - incorrect auth");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_ReadPubek, "  cmd");
	response[294] = 0x60;
}

/**
 * Test TakeOwnership
 */
void TakeOwnershipTest(void) {
	uint8_t oiap_response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x4c, 0x04, 0x1a, 0x18, 0xa9,
		0xf7, 0x9b, 0x2e, 0xe1, 0xf2, 0x16, 0x99, 0xa0,
		0x27, 0x5f, 0x0c, 0x8f, 0x24, 0x55, 0x1d, 0xaf,
		0x96, 0x49,
	};
	uint8_t take_ownership_response[] = {
		0x00, 0xc5, 0x00, 0x00, 0x01, 0x62, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x11,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x0c, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x92, 0x61, 0xa5,
		0x30, 0x5f, 0x39, 0xb7, 0xc3, 0x51, 0x53, 0x84,
		0xaf, 0x51, 0x64, 0x65, 0xd7, 0x58, 0xda, 0x15,
		0xb0, 0xb8, 0xe8, 0xea, 0xf5, 0xb2, 0x21, 0x34,
		0x09, 0x71, 0xa0, 0xd5, 0x2b, 0x21, 0xd6, 0x16,
		0xbd, 0x03, 0xc3, 0x83, 0x7e, 0x48, 0x36, 0xd7,
		0xfa, 0xc7, 0x5e, 0x5e, 0xb4, 0xc3, 0x9f, 0x00,
		0xcc, 0x7a, 0x3e, 0x0a, 0x00, 0x34, 0xbd, 0xbc,
		0x7b, 0x28, 0x0e, 0x4a, 0xef, 0xa5, 0x86, 0x50,
		0xa5, 0xfe, 0x8f, 0x7d, 0xbc, 0x86, 0xf3, 0x3d,
		0x8c, 0x65, 0x4c, 0x3b, 0x29, 0x3b, 0x40, 0x8b,
		0xca, 0xf9, 0xa1, 0xc8, 0x62, 0x52, 0xe1, 0x1f,
		0x0d, 0x89, 0x71, 0xca, 0xbb, 0x64, 0xef, 0x3f,
		0x97, 0x97, 0xed, 0x57, 0xb3, 0xd8, 0x67, 0x4c,
		0x46, 0x1c, 0x35, 0x0c, 0xba, 0x12, 0xae, 0x2e,
		0x6d, 0xa7, 0x48, 0xd6, 0x9a, 0x8a, 0x60, 0x22,
		0xd9, 0xe5, 0x97, 0x50, 0xc1, 0x24, 0xaf, 0xb6,
		0x41, 0xfe, 0x6d, 0xfe, 0x28, 0x3f, 0xce, 0x35,
		0x9b, 0x77, 0xe9, 0xd5, 0x36, 0xdb, 0x70, 0x5e,
		0xd7, 0xb9, 0x89, 0xac, 0xae, 0x55, 0x59, 0x00,
		0x7d, 0x47, 0x5d, 0x73, 0x4f, 0x1b, 0x17, 0xfa,
		0xae, 0xb0, 0xf7, 0xb7, 0x63, 0x4d, 0xa9, 0x94,
		0x0c, 0x1e, 0x02, 0x59, 0x8d, 0x34, 0x1f, 0x01,
		0x6d, 0xa7, 0x05, 0xa7, 0xae, 0xbd, 0x9b, 0xfa,
		0xed, 0xe6, 0xe3, 0xf2, 0xc0, 0xa9, 0x16, 0xb6,
		0xd2, 0x23, 0x37, 0x2e, 0x43, 0x5e, 0x5f, 0xe6,
		0x77, 0x0d, 0x49, 0x48, 0x07, 0x57, 0x64, 0xd2,
		0xd9, 0x60, 0xff, 0xe3, 0x60, 0xb6, 0xd7, 0xa5,
		0xe3, 0xd8, 0xa3, 0x93, 0xb3, 0xe9, 0xeb, 0x1c,
		0x53, 0x42, 0x08, 0x9f, 0x0c, 0x13, 0x72, 0x3c,
		0x80, 0xf8, 0xa1, 0x8c, 0x4d, 0xe5, 0x1e, 0xe7,
		0xef, 0x2b, 0x33, 0x23, 0x1e, 0x5a, 0xf6, 0xc1,
		0x46, 0x78, 0x06, 0x7e, 0xe7, 0x00, 0x00, 0x00,
		0x00, 0xef, 0x84, 0x26, 0xd3, 0xb6, 0x27, 0x4a,
		0x4a, 0x0f, 0x84, 0x65, 0x4b, 0xff, 0x80, 0x7e,
		0xb5, 0xf6, 0xbe, 0x8c, 0xed, 0x00, 0xad, 0xd1,
		0x73, 0x8a, 0x55, 0x9f, 0x50, 0xb4, 0x34, 0xba,
		0x2d, 0x6d, 0x80, 0x3a, 0xdc, 0x82, 0x94, 0x3b,
		0x96, 0x58,
	};
	uint8_t encrypted_secret[] = {
		0x46, 0x9a, 0x17, 0x31, 0x04, 0x72, 0x58, 0xcd,
		0xac, 0xe7, 0xa4, 0x1f, 0x48, 0xa3, 0x89, 0x10,
		0xac, 0x40, 0xe2, 0x66, 0xfa, 0xfd, 0xe9, 0xab,
		0x7a, 0x55, 0xd3, 0xc0, 0x61, 0xca, 0x28, 0x0d,
		0x29, 0x4a, 0xe4, 0x9a, 0xbe, 0x62, 0x51, 0xe8,
		0x3f, 0xbf, 0x84, 0xae, 0x4e, 0x6c, 0x0e, 0x11,
		0x2b, 0xba, 0x62, 0x5d, 0xf5, 0x9d, 0xf8, 0xcd,
		0x5c, 0x9d, 0x5b, 0xee, 0x5e, 0xdc, 0xaf, 0xc1,
		0xbf, 0x22, 0x14, 0x0d, 0x68, 0xdf, 0xe1, 0x94,
		0x6b, 0x06, 0xc4, 0x5b, 0xdd, 0xee, 0xd3, 0xef,
		0x67, 0xb5, 0xb0, 0xee, 0x58, 0x88, 0x2d, 0x5c,
		0x7d, 0xda, 0x83, 0xd5, 0xb5, 0x72, 0x43, 0x33,
		0xf7, 0x9e, 0xf0, 0x52, 0x8c, 0xc1, 0xf1, 0xea,
		0xcf, 0x9f, 0x0e, 0xfb, 0xb3, 0x03, 0xfe, 0xb3,
		0xb4, 0x38, 0xa2, 0xfb, 0x2f, 0x64, 0xb6, 0x42,
		0x4c, 0x76, 0x70, 0xfa, 0x67, 0xc0, 0x48, 0x98,
		0x52, 0x3e, 0xdb, 0xe6, 0xfe, 0x44, 0x96, 0x14,
		0x5a, 0x6a, 0x19, 0x53, 0x46, 0x13, 0xe6, 0xc9,
		0x21, 0xee, 0x8e, 0xc2, 0xf2, 0x39, 0x2d, 0xba,
		0x6f, 0xeb, 0x80, 0x89, 0xf3, 0xea, 0xfa, 0x5c,
		0x9c, 0x88, 0xe0, 0xb1, 0x53, 0xa0, 0xe5, 0xe0,
		0x90, 0x33, 0x9d, 0x9d, 0x5f, 0xba, 0x6d, 0x68,
		0xb2, 0x9f, 0x4f, 0xa1, 0x28, 0xf9, 0xc4, 0x53,
		0x72, 0x51, 0x48, 0x4b, 0xb3, 0xf9, 0x18, 0x43,
		0x3a, 0x85, 0xdc, 0x70, 0x46, 0x0c, 0x3c, 0xe1,
		0x17, 0x1c, 0x18, 0x6f, 0xfd, 0xff, 0x77, 0x8d,
		0x04, 0xfc, 0xb3, 0xc0, 0x9a, 0x03, 0x74, 0x1d,
		0x06, 0x8f, 0xb6, 0x0a, 0x3e, 0xea, 0x91, 0x87,
		0xa9, 0x68, 0x26, 0x91, 0x81, 0x02, 0xe4, 0x10,
		0x66, 0xb6, 0x5f, 0x43, 0x47, 0x55, 0x25, 0xe0,
		0xbd, 0xd3, 0xab, 0xbd, 0xfd, 0x15, 0x85, 0x39,
		0x22, 0x93, 0xfc, 0x9d, 0x74, 0x0e, 0xcf, 0x5a,
	};
	uint8_t owner_secret[TPM_AUTH_DATA_LEN] = { 0 };

	ResetMocks();
	calls[0].rsp = oiap_response;
	calls[0].rsp_size = sizeof(oiap_response);
	calls[1].rsp = take_ownership_response;
	calls[1].rsp_size = sizeof(take_ownership_response);
	TEST_EQ(TlclTakeOwnership(encrypted_secret, encrypted_secret,
				  owner_secret), TPM_SUCCESS, "TakeOwnership");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OIAP, "  oiap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_TakeOwnership, "  takeownership cmd");

	/* Verify that the response gets authenticated. */
	ResetMocks();
	calls[0].rsp = oiap_response;
	calls[0].rsp_size = sizeof(oiap_response);
	calls[1].rsp = take_ownership_response;
	calls[1].rsp_size = sizeof(take_ownership_response);
	take_ownership_response[334] = 0;
	TEST_EQ(TlclTakeOwnership(encrypted_secret, encrypted_secret,
				  owner_secret),
		TPM_E_AUTHFAIL, "TakeOwnership - response auth");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OIAP, "  oiap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_TakeOwnership, "  takeownership cmd");
	take_ownership_response[334] = 0xad;

	/* Verify that a short OIAP response gets caught. */
	ResetMocks();
	calls[0].rsp = oiap_response;
	calls[0].rsp_size = sizeof(oiap_response);
	ToTpmUint32(oiap_response + sizeof(uint16_t),
		    kTpmRequestHeaderLength + sizeof(uint32_t) +
		    sizeof(TPM_NONCE) - 1);
	TEST_EQ(TlclTakeOwnership(encrypted_secret, encrypted_secret,
				  owner_secret),
		TPM_E_INVALID_RESPONSE, "TakeOwnership - short OIAP response");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OIAP, "  oiap cmd");
	ToTpmUint32(oiap_response + sizeof(uint16_t), sizeof(oiap_response));

	/* Verify that a short TakeOwnership response gets caught. */
	ResetMocks();
	calls[0].rsp = oiap_response;
	calls[0].rsp_size = sizeof(oiap_response);
	calls[1].rsp = take_ownership_response;
	calls[1].rsp_size = sizeof(take_ownership_response);
	ToTpmUint32(take_ownership_response + sizeof(uint16_t),
		    kTpmResponseHeaderLength + kTpmResponseAuthBlockLength - 1);
	TEST_EQ(TlclTakeOwnership(encrypted_secret, encrypted_secret,
				  owner_secret),
		TPM_E_INVALID_RESPONSE,
		"TakeOwnership - short TakeOwnership response");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_OIAP, "  oiap cmd");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_TakeOwnership, "  takeownership cmd");
	ToTpmUint32(take_ownership_response + sizeof(uint16_t),
		    sizeof(take_ownership_response));
}

/**
 * Test ReadDelegationFamilyTable
 */
void ReadDelegationFamilyTableTest(void) {
	uint8_t response[] = {
		0x00, 0xc4, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x25,
		0x17, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x25, 0x42,
		0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	TPM_FAMILY_TABLE_ENTRY table[20];
	uint32_t table_size;

	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	table_size = 8;
	TEST_EQ(TlclReadDelegationFamilyTable(table, &table_size),
		TPM_SUCCESS, "ReadDelegationFamilyTable");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Delegate_ReadTable, "  cmd");
	TEST_EQ(table_size, 2, "  table_size");
	TEST_EQ(table[0].tag, 0x25, "  table[0].tag");
	TEST_EQ(table[0].familyLabel, 0x17, "  table[0].familyLabel");
	TEST_EQ(table[0].familyID, 0x4f, "  table[0].familyID");
	TEST_EQ(table[0].verificationCount, 0x1,
		"  table[0].verificationCount");
	TEST_EQ(table[0].flags, 0x2, "  table[0].flags");
	TEST_EQ(table[1].tag, 0x25, "  table[1].tag");
	TEST_EQ(table[1].familyLabel, 0x42, "  table[1].familyLabel");
	TEST_EQ(table[1].familyID, 0x50, "  table[1].familyID");
	TEST_EQ(table[1].verificationCount, 0x1,
		"  table[1].verificationCount");
	TEST_EQ(table[1].flags, 0x0, "  table[1].flags");

	/* Test that required table size is returned if more space required. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	table_size = 1;
	TEST_EQ(TlclReadDelegationFamilyTable(table, &table_size),
		TPM_E_BUFFER_SIZE, "ReadDelegationFamilyTable");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Delegate_ReadTable, "  cmd");
	TEST_EQ(table_size, 2, "  table_size");

	/* Test that an overlong response gets caught. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + sizeof(uint16_t), TPM_LARGE_ENOUGH_COMMAND_SIZE +
		    1);
	TEST_EQ(TlclReadDelegationFamilyTable(table, &table_size),
		TPM_E_INVALID_RESPONSE,
		"ReadDelegationFamilyTable - too long response");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Delegate_ReadTable, "  cmd");
	ToTpmUint32(response + sizeof(uint16_t), sizeof(response));

	/* Test that a short response gets caught. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	ToTpmUint32(response + sizeof(uint16_t),
		    kTpmRequestHeaderLength + sizeof(uint32_t) - 1);
	TEST_EQ(TlclReadDelegationFamilyTable(table, &table_size),
		TPM_E_INVALID_RESPONSE,
		"ReadDelegationFamilyTable - too short response");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Delegate_ReadTable, "  cmd");
	ToTpmUint32(response + sizeof(uint16_t), sizeof(response));

	/* Test that long table size in response gets caught. */
	ResetMocks();
	calls[0].rsp = response;
	calls[0].rsp_size = sizeof(response);
	table_size = 20;
	ToTpmUint32(response + kTpmResponseHeaderLength,
		    TPM_LARGE_ENOUGH_COMMAND_SIZE);
	TEST_EQ(TlclReadDelegationFamilyTable(table, &table_size),
		TPM_E_INVALID_RESPONSE,
		"ReadDelegationFamilyTable - overlong family table");
	TEST_EQ(calls[0].req_cmd, TPM_ORD_Delegate_ReadTable, "  cmd");
	ToTpmUint32(response + kTpmResponseHeaderLength, 0x1e);
}

int main(void)
{
	TlclTest();
	SendCommandTest();
	ReadWriteTest();
	DefineSpaceExTest();
	InitNvAuthPolicyTest();
	PcrTest();
	GetSpaceInfoTest();
	FlagsTest();
	RandomTest();
	GetVersionTest();
	IFXFieldUpgradeInfoTest();
	ReadPubekTest();
	TakeOwnershipTest();

	return gTestSuccess ? 0 : 255;
}
