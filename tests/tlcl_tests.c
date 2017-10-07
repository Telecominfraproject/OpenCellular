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
	TEST_EQ(TlclIFXFieldUpgradeInfo(&info), TPM_E_IOERROR,
		"IFXFieldUpgradeInfo - short");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_FieldUpgrade, "  cmd");

	/* Adjust version response to claim a non-IFX vendor. */
	ToTpmUint32(version_response + kTpmResponseHeaderLength +
		    sizeof(uint32_t), 0);
	ResetMocks();
	calls[0].rsp = version_response;
	calls[0].rsp_size = sizeof(version_response);
	TEST_EQ(TlclIFXFieldUpgradeInfo(&info), TPM_E_IOERROR,
		"IFXFieldUpgradeInfo - bad vendor");
	TEST_EQ(calls[1].req_cmd, TPM_ORD_FieldUpgrade, "  cmd");
}

int main(void)
{
	TlclTest();
	SendCommandTest();
	ReadWriteTest();
	PcrTest();
	FlagsTest();
	RandomTest();
	GetVersionTest();
	IFXFieldUpgradeInfoTest();

	return gTestSuccess ? 0 : 255;
}
