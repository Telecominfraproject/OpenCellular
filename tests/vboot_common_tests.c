/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware vboot_common.c
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"
#include "utility.h"
#include "vboot_common.h"

/*
 * Test struct packing for vboot_struct.h structs which are passed between
 * firmware and OS, or passed between different phases of firmware.
 */
static void StructPackingTest(void)
{
	TEST_EQ(EXPECTED_VBPUBLICKEY_SIZE, sizeof(VbPublicKey),
		"sizeof(VbPublicKey)");
	TEST_EQ(EXPECTED_VBSIGNATURE_SIZE, sizeof(VbSignature),
		"sizeof(VbSignature)");
	TEST_EQ(EXPECTED_VBKEYBLOCKHEADER_SIZE, sizeof(VbKeyBlockHeader),
		"sizeof(VbKeyBlockHeader)");
	TEST_EQ(EXPECTED_VBFIRMWAREPREAMBLEHEADER2_0_SIZE,
		sizeof(VbFirmwarePreambleHeader2_0),
		"sizeof(VbFirmwarePreambleHeader2_0)");
	TEST_EQ(EXPECTED_VBFIRMWAREPREAMBLEHEADER2_1_SIZE,
		sizeof(VbFirmwarePreambleHeader),
		"sizeof(VbFirmwarePreambleHeader)");
	TEST_EQ(EXPECTED_VBKERNELPREAMBLEHEADER2_2_SIZE,
		sizeof(VbKernelPreambleHeader),
		"sizeof(VbKernelPreambleHeader)");

	TEST_EQ(VB_SHARED_DATA_HEADER_SIZE_V1,
		(long)&((VbSharedDataHeader*)NULL)->recovery_reason,
		"sizeof(VbSharedDataHeader) V1");

	TEST_EQ(VB_SHARED_DATA_HEADER_SIZE_V2,
		sizeof(VbSharedDataHeader),
		"sizeof(VbSharedDataHeader) V2");
}

/* Test array size macro */
static void ArraySizeTest(void)
{
	uint8_t arr1[12];
	uint32_t arr2[7];
	uint64_t arr3[9];

	TEST_EQ(ARRAY_SIZE(arr1), 12, "ARRAYSIZE(uint8_t)");
	TEST_EQ(ARRAY_SIZE(arr2), 7, "ARRAYSIZE(uint32_t)");
	TEST_EQ(ARRAY_SIZE(arr3), 9, "ARRAYSIZE(uint64_t)");
}

/* Helper functions not dependent on specific key sizes */
static void VerifyHelperFunctions(void)
{
	{
		uint8_t *p = (uint8_t *)VerifyHelperFunctions;
		TEST_EQ((int)OffsetOf(p, p), 0, "OffsetOf() equal");
		TEST_EQ((int)OffsetOf(p, p+10), 10, "OffsetOf() positive");
		TEST_EQ((int)OffsetOf(p, p+0x12345678), 0x12345678,
			"OffsetOf() large");
	}

	{
		VbPublicKey k = {sizeof(k), 2, 3, 4};
		TEST_EQ((int)OffsetOf(&k, GetPublicKeyData(&k)), sizeof(k),
			"GetPublicKeyData() adjacent");
		TEST_EQ((int)OffsetOf(&k, GetPublicKeyDataC(&k)), sizeof(k),
			"GetPublicKeyDataC() adjacent");
	}

	{
		VbPublicKey k = {123, 2, 3, 4};
		TEST_EQ((int)OffsetOf(&k, GetPublicKeyData(&k)), 123,
			"GetPublicKeyData() spaced");
		TEST_EQ((int)OffsetOf(&k, GetPublicKeyDataC(&k)), 123,
			"GetPublicKeyDataC() spaced");
	}

	{
		uint8_t *p = (uint8_t *)VerifyHelperFunctions;
		TEST_EQ(VerifyMemberInside(p, 20, p, 6, 11, 3), 0,
			"MemberInside ok 1");
		TEST_EQ(VerifyMemberInside(p, 20, p+4, 4, 8, 4), 0,
			"MemberInside ok 2");
		TEST_EQ(VerifyMemberInside(p, 20, p-4, 4, 8, 4), 1,
			"MemberInside member before parent");
		TEST_EQ(VerifyMemberInside(p, 20, p+20, 4, 8, 4), 1,
			"MemberInside member after parent");
		TEST_EQ(VerifyMemberInside(p, 20, p, 21, 0, 0), 1,
			"MemberInside member too big");
		TEST_EQ(VerifyMemberInside(p, 20, p, 4, 21, 0), 1,
			"MemberInside data after parent");
		TEST_EQ(VerifyMemberInside(p, 20, p, 4, (uint64_t)-1, 0), 1,
			"MemberInside data before parent");
		TEST_EQ(VerifyMemberInside(p, 20, p, 4, 4, 17), 1,
			"MemberInside data too big");
		TEST_EQ(VerifyMemberInside(p, (uint64_t)-1,
					   p+(uint64_t)-10, 12, 5, 0), 1,
			"MemberInside wraparound 1");
		TEST_EQ(VerifyMemberInside(p, (uint64_t)-1,
					   p+(uint64_t)-10, 5, 12, 0), 1,
			"MemberInside wraparound 2");
		TEST_EQ(VerifyMemberInside(p, (uint64_t)-1,
					   p+(uint64_t)-10, 5, 0, 12), 1,
			"MemberInside wraparound 3");
	}

	{
		VbPublicKey k = {sizeof(k), 128, 0, 0};
		TEST_EQ(VerifyPublicKeyInside(&k, sizeof(k)+128, &k), 0,
			"PublicKeyInside ok 1");
		TEST_EQ(VerifyPublicKeyInside(&k - 1, 2*sizeof(k)+128, &k), 0,
			"PublicKeyInside ok 2");
		TEST_EQ(VerifyPublicKeyInside(&k, 128, &k), 1,
			"PublicKeyInside key too big");
	}

	{
		VbPublicKey k = {100, 4, 0, 0};
		TEST_EQ(VerifyPublicKeyInside(&k, 99, &k), 1,
			"PublicKeyInside offset too big");
	}

	{
		VbSignature s = {sizeof(s), 128, 2000};
		TEST_EQ(VerifySignatureInside(&s, sizeof(s)+128, &s), 0,
			"SignatureInside ok 1");
		TEST_EQ(VerifySignatureInside(&s - 1, 2*sizeof(s)+128, &s), 0,
			"SignatureInside ok 2");
		TEST_EQ(VerifySignatureInside(&s, 128, &s), 1,
			"SignatureInside sig too big");
	}

	{
		VbSignature s = {100, 4, 0};
		TEST_EQ(VerifySignatureInside(&s, 99, &s), 1,
			"SignatureInside offset too big");
	}
}

/* Public key utility functions */
static void PublicKeyTest(void)
{
	VbPublicKey k[3];
	VbPublicKey j[5];

	/* Fill some bits of the public key data */
	Memset(j, 0, sizeof(j));
	Memset(k, 0x42, sizeof(k));
	k[1].key_size = 12345;
	k[2].key_version = 67;

	PublicKeyInit(k, (uint8_t*)(k + 1), 2 * sizeof(VbPublicKey));
	TEST_EQ(k->key_offset, sizeof(VbPublicKey), "PublicKeyInit key_offset");
	TEST_EQ(k->key_size, 2 * sizeof(VbPublicKey), "PublicKeyInit key_size");
	TEST_EQ(k->algorithm, kNumAlgorithms, "PublicKeyInit algorithm");
	TEST_EQ(k->key_version, 0, "PublicKeyInit key_version");

	/* Set algorithm and version, so we can tell if they get copied */
	k->algorithm = 3;
	k->key_version = 21;

	/* Copying to a smaller destination should fail */
	PublicKeyInit(j, (uint8_t*)(j + 1), 2 * sizeof(VbPublicKey) - 1);
	TEST_NEQ(0, PublicKeyCopy(j, k), "PublicKeyCopy too small");

	/* Copying to same or larger size should succeed */
	PublicKeyInit(j, (uint8_t*)(j + 2), 2 * sizeof(VbPublicKey) + 1);
	TEST_EQ(0, PublicKeyCopy(j, k), "PublicKeyCopy same");
	/* Offset in destination shouldn't have been modified */
	TEST_EQ(j->key_offset, 2 * sizeof(VbPublicKey),
		"PublicKeyCopy key_offset");
	/* Size should have been reduced to match the source */
	TEST_EQ(k->key_size, 2 * sizeof(VbPublicKey), "PublicKeyCopy key_size");
	/* Other fields should have been copied */
	TEST_EQ(k->algorithm, j->algorithm, "PublicKeyCopy algorithm");
	TEST_EQ(k->key_version, j->key_version, "PublicKeyCopy key_version");
	/* Data should have been copied */
	TEST_EQ(0,
		Memcmp(GetPublicKeyData(k), GetPublicKeyData(j), k->key_size),
		"PublicKeyCopy data");
}

/* VbSharedData utility tests */
static void VbSharedDataTest(void)
{
	uint8_t buf[VB_SHARED_DATA_MIN_SIZE + 1];
	VbSharedDataHeader* d = (VbSharedDataHeader*)buf;

	TEST_NEQ(VBOOT_SUCCESS,
		 VbSharedDataInit(d, sizeof(VbSharedDataHeader) - 1),
		 "VbSharedDataInit too small");
	TEST_NEQ(VBOOT_SUCCESS,
		 VbSharedDataInit(d, VB_SHARED_DATA_MIN_SIZE - 1),
		 "VbSharedDataInit too small 2");
	TEST_NEQ(VBOOT_SUCCESS,
		 VbSharedDataInit(NULL, VB_SHARED_DATA_MIN_SIZE),
		 "VbSharedDataInit null");

	Memset(buf, 0x68, sizeof(buf));
	TEST_EQ(VBOOT_SUCCESS, VbSharedDataInit(d, VB_SHARED_DATA_MIN_SIZE),
		"VbSharedDataInit");

	/* Check fields that should have been initialized */
	TEST_EQ(d->magic, VB_SHARED_DATA_MAGIC, "VbSharedDataInit magic");
	TEST_EQ(d->struct_version, VB_SHARED_DATA_VERSION,
		"VbSharedDataInit version");
	TEST_EQ(d->struct_size, sizeof(VbSharedDataHeader),
		"VbSharedDataInit struct_size");
	TEST_EQ(d->data_size, VB_SHARED_DATA_MIN_SIZE,
		"VbSharedDataInit data_size");
	TEST_EQ(d->data_used, d->struct_size, "VbSharedDataInit data_used");
	TEST_EQ(d->firmware_index, 0xFF, "VbSharedDataInit firmware index");

	/* Sample some other fields to make sure they were zeroed */
	TEST_EQ(d->flags, 0, "VbSharedDataInit firmware flags");
	TEST_EQ(d->lk_call_count, 0, "VbSharedDataInit lk_call_count");
	TEST_EQ(d->kernel_version_lowest, 0,
		"VbSharedDataInit kernel_version_lowest");

	TEST_NEQ(VBOOT_SUCCESS, VbSharedDataSetKernelKey(NULL, NULL),
		 "VbSharedDataSetKernelKey null");
}

int main(int argc, char* argv[])
{
	StructPackingTest();
	ArraySizeTest();
	VerifyHelperFunctions();
	PublicKeyTest();
	VbSharedDataTest();

	if (vboot_api_stub_check_memory())
		return 255;

	return gTestSuccess ? 0 : 255;
}
