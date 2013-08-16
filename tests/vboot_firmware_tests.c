/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_firmware library.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbb_header.h"
#include "host_common.h"
#include "load_firmware_fw.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Mock data */
static VbCommonParams cparams;
static VbSelectFirmwareParams fparams;
static VbKeyBlockHeader vblock[2];
static VbFirmwarePreambleHeader mpreamble[2];
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader* shared = (VbSharedDataHeader*)shared_data;
static uint8_t gbb_data[sizeof(GoogleBinaryBlockHeader) + 2048];
static GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)gbb_data;
static RSAPublicKey data_key;
static uint32_t digest_size;
static uint8_t* digest_returned;
static uint8_t* digest_expect_ptr;
static int hash_fw_index;

#define TEST_KEY_DATA	\
	"Test contents for the root key this should be 64 chars long."

/* Reset mock data (for use before each test) */
static void ResetMocks(void) {
  VbPublicKey *root_key;
  uint8_t *root_key_data;
  int i;

  Memset(&cparams, 0, sizeof(cparams));
  cparams.shared_data_blob = shared_data;
  cparams.gbb_data = gbb_data;
  cparams.gbb_size = sizeof(gbb_data);
  cparams.gbb = gbb;

  Memset(&fparams, 0, sizeof(fparams));
  fparams.verification_block_A = vblock;
  fparams.verification_size_A = sizeof(VbKeyBlockHeader);
  fparams.verification_block_B = vblock + 1;
  fparams.verification_size_B = sizeof(VbKeyBlockHeader);

  Memset(vblock, 0, sizeof(vblock));
  Memset(mpreamble, 0, sizeof(mpreamble));
  for (i = 0; i < 2; i++) {
    /* Default verification blocks to working in all modes */
    vblock[i].key_block_flags = 0x0F;
    vblock[i].data_key.key_version = 2;
    /* Fix up offsets to preambles */
    vblock[i].key_block_size =
        (uint8_t*)(mpreamble + i) - (uint8_t*)(vblock + i);

    mpreamble[i].header_version_minor = 1;  /* Supports preamble flags */
    mpreamble[i].firmware_version = 4;
    /* Point kernel subkey to some data following the key header */
    PublicKeyInit(&mpreamble[i].kernel_subkey,
                  (uint8_t*)&mpreamble[i].body_signature, 20);
    mpreamble[i].kernel_subkey.algorithm = 7 + i;
    mpreamble[i].body_signature.data_size = 20000 + 1000 * i;
  }

  Memset(&vnc, 0, sizeof(vnc));
  VbNvSetup(&vnc);

  Memset(&shared_data, 0, sizeof(shared_data));
  VbSharedDataInit(shared, sizeof(shared_data));
  shared->fw_version_tpm = 0x00020004;

  Memset(&gbb_data, 0, sizeof(gbb_data));
  gbb->rootkey_offset = sizeof(GoogleBinaryBlockHeader);
  root_key = (VbPublicKey *)(gbb_data + gbb->rootkey_offset);
  root_key_data = (uint8_t *)(root_key + 1);
  strcpy((char *)root_key_data, TEST_KEY_DATA);
  PublicKeyInit(root_key, (uint8_t *)root_key_data, sizeof(TEST_KEY_DATA));

  gbb->major_version = GBB_MAJOR_VER;
  gbb->minor_version = GBB_MINOR_VER;
  gbb->flags = 0;

  Memset(&data_key, 0, sizeof(data_key));

  digest_size = 1234;
  digest_returned = NULL;
  digest_expect_ptr = NULL;
  hash_fw_index = -1;
}

/****************************************************************************/
/* Mocked verification functions */

int KeyBlockVerify(const VbKeyBlockHeader* block, uint64_t size,
                   const VbPublicKey *key, int hash_only) {

  TEST_EQ(hash_only, 0, "  Don't verify firmware with hash");

  /*
   * We cannot check the address of key, since it will be allocated. We
   * check the contents instead.
   */
  TEST_STR_EQ((char *)GetPublicKeyDataC(key), TEST_KEY_DATA,
              "  Verify with root key");
  TEST_NEQ(block==vblock || block==vblock+1, 0, "  Verify a valid key block");

  /* Mock uses header_version_major to hold return value */
  return block->header_version_major;
}

int VerifyFirmwarePreamble(const VbFirmwarePreambleHeader* preamble,
                           uint64_t size, const RSAPublicKey* key) {
  TEST_PTR_EQ(key, &data_key, "  Verify preamble data key");
  TEST_NEQ(preamble==mpreamble || preamble==mpreamble+1, 0,
           "  Verify a valid preamble");

  /* Mock uses header_version_major to hold return value */
  return preamble->header_version_major;
}

RSAPublicKey* PublicKeyToRSA(const VbPublicKey* key) {
  /* Mock uses algorithm!0 to mean invalid key */
  if (key->algorithm)
    return NULL;
  /* Mock uses data key len to hold number of alloc'd keys */
  data_key.len++;
  return &data_key;
}

void RSAPublicKeyFree(RSAPublicKey* key) {
  TEST_PTR_EQ(key, &data_key, "  RSA data key");
  data_key.len--;
}

void DigestInit(DigestContext* ctx, int sig_algorithm) {
  digest_size = 0;
}

void DigestUpdate(DigestContext* ctx, const uint8_t* data, uint32_t len) {
  TEST_PTR_EQ(data, digest_expect_ptr, "  Digesting expected data");
  digest_size += len;
}

uint8_t* DigestFinal(DigestContext* ctx) {
  digest_returned = (uint8_t*)VbExMalloc(4);
  return digest_returned;
}

VbError_t VbExHashFirmwareBody(VbCommonParams* cparams,
                               uint32_t firmware_index) {
  if (VB_SELECT_FIRMWARE_A == firmware_index)
    hash_fw_index = 0;
  else if (VB_SELECT_FIRMWARE_B == firmware_index)
    hash_fw_index = 1;
  else
    return VBERROR_INVALID_PARAMETER;

  digest_expect_ptr = (uint8_t*)(vblock + hash_fw_index) + 5;
  VbUpdateFirmwareBodyHash(
      cparams, digest_expect_ptr,
      mpreamble[hash_fw_index].body_signature.data_size - 1024);
  VbUpdateFirmwareBodyHash(cparams, digest_expect_ptr, 1024);

  /* If signature offset is 42, hash the wrong amount and return success */
  if (42 == mpreamble[hash_fw_index].body_signature.sig_offset) {
    VbUpdateFirmwareBodyHash(cparams, digest_expect_ptr, 4);
    return VBERROR_SUCCESS;
  }

  /* Otherwise, mocked function uses body signature offset as returned value */
  return mpreamble[hash_fw_index].body_signature.sig_offset;
}

int VerifyDigest(const uint8_t* digest, const VbSignature *sig,
                 const RSAPublicKey* key) {
  TEST_PTR_EQ(digest, digest_returned, "Verifying expected digest");
  TEST_PTR_EQ(key, &data_key, "Verifying using data key");
  TEST_PTR_EQ(sig, &mpreamble[hash_fw_index].body_signature, "Verifying sig");
  /* Mocked function uses sig size as return value for verifying digest */
  return sig->sig_size;
}

/****************************************************************************/
/* Test LoadFirmware() and check expected return value and recovery reason */
static void TestLoadFirmware(VbError_t expected_retval,
                             uint8_t expected_recovery, const char* desc) {
  uint32_t rr = 256;

  TEST_EQ(LoadFirmware(&cparams, &fparams, &vnc), expected_retval, desc);
  VbNvGet(&vnc, VBNV_RECOVERY_REQUEST, &rr);
  TEST_EQ(rr, expected_recovery, "  recovery request");
  TEST_EQ(data_key.len, 0, "  Data key free must be paired with alloc");
}

/****************************************************************************/

static void LoadFirmwareTest(void) {
  uint32_t u;

  /* Require GBB */
  ResetMocks();
  cparams.gbb_data = NULL;
  TestLoadFirmware(VBERROR_INVALID_GBB, VBNV_RECOVERY_RO_UNSPECIFIED,
                   "No GBB");

  /* Key block flags must match */
  /* Normal boot */
  ResetMocks();
  vblock[0].key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_1;
  vblock[1].key_block_flags =
      KEY_BLOCK_FLAG_DEVELOPER_0 | KEY_BLOCK_FLAG_RECOVERY_1;
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_REC_MISMATCH),
                   "Flags mismatch dev=0");
  TEST_EQ(shared->flags & VBSD_LF_DEV_SWITCH_ON, 0,
          "Dev flag in shared.flags dev=0");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_DEV_MISMATCH,
          "Key block flag mismatch for dev=0");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_REC_MISMATCH,
          "Key block flag mismatch for rec=0");
  /* Developer boot */
  ResetMocks();
  shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
  vblock[0].key_block_flags = KEY_BLOCK_FLAG_DEVELOPER_0;
  vblock[1].key_block_flags =
      KEY_BLOCK_FLAG_DEVELOPER_1 | KEY_BLOCK_FLAG_RECOVERY_1;
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_REC_MISMATCH),
                   "Flags mismatch dev=1");
  TEST_NEQ(shared->flags & VBSD_LF_DEV_SWITCH_ON, 0,
          "Dev flag in shared.flags dev=1");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_DEV_MISMATCH,
          "Key block flag mismatch for dev=1");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_REC_MISMATCH,
          "Key block flag mismatch for rec=1");

  /* Test key block verification with A and key version rollback with B */
  ResetMocks();
  vblock[0].header_version_major = 1;  /* Simulate failure */
  vblock[1].data_key.key_version = 1;  /* Simulate rollback */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_KEY_ROLLBACK),
                   "Key block invalid / key version rollback");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VERIFY_KEYBLOCK,
          "Key block invalid");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_KEY_ROLLBACK,
          "Key version rollback ");
  TEST_EQ(shared->fw_version_lowest, 0, "Lowest valid version");
  TEST_EQ(shared->fw_version_tpm, 0x20004, "TPM version");

  /* Test invalid key version with A and bad data key with B */
  ResetMocks();
  vblock[0].data_key.key_version = 0x10003;  /* Version > 0xFFFF is invalid */
  vblock[1].data_key.algorithm = 1;  /* Simulate invalid data key */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_DATA_KEY_PARSE),
                   "Key version overflow / invalid data key");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_KEY_ROLLBACK,
          "Key version overflow");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_DATA_KEY_PARSE,
          "Data key invalid");

  /* Test invalid key version with GBB bypass-rollback flag */
  ResetMocks();
  vblock[0].data_key.key_version = 1;  /* Simulate rollback */
  gbb->flags = GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK;
  TestLoadFirmware(VBERROR_SUCCESS, 0, "Key version check + GBB override");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID,
          "Key version rollback + GBB override");

  /* Test invalid preamble with A */
  ResetMocks();
  mpreamble[0].header_version_major = 1;  /* Simulate failure */
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_VERIFY_PREAMBLE),
                   "Preamble invalid");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VERIFY_PREAMBLE,
          "Preamble invalid A");

  /* Test invalid firmware versions */
  ResetMocks();
  mpreamble[0].firmware_version = 3;  /* Simulate rollback */
  mpreamble[1].firmware_version = 0x10001;  /* Check overflow */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_FW_ROLLBACK),
                   "Firmware version check");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_FW_ROLLBACK,
          "Firmware version rollback");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_FW_ROLLBACK,
          "Firmware version overflow");

  /* Test invalid firmware versions with GBB bypass-rollback flag */
  ResetMocks();
  mpreamble[0].firmware_version = 3;  /* Simulate rollback */
  mpreamble[1].firmware_version = 0x10001;  /* Check overflow */
  gbb->flags = GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK;
  TestLoadFirmware(VBERROR_SUCCESS, 0, "Firmware version check + GBB bypass");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID,
          "Firmware version rollback + GBB override");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_HEADER_VALID,
          "Firmware version overflow + GBB override");

  /* Test RO normal with A */
  ResetMocks();
  mpreamble[0].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_NO_RO_NORMAL),
                   "Preamble asked for RO normal but fw doesn't support it");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_NO_RO_NORMAL,
          "No RO normal A");

  /* If RO normal is supported, don't need to verify the firmware body */
  ResetMocks();
  mpreamble[0].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  /* Mock bad sig, to ensure we didn't use it */
  mpreamble[0].body_signature.sig_size = 1;
  shared->flags |= VBSD_BOOT_RO_NORMAL_SUPPORT;
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_SUCCESS, 0, "RO normal A");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID, "RO normal A valid");
  TEST_EQ(shared->firmware_index, 0, "Boot A shared index");
  TEST_EQ(shared->fw_keyblock_flags, vblock[0].key_block_flags,
          "Copy key block flags");
  TEST_EQ(shared->kernel_subkey.algorithm, 7, "Copy kernel subkey");

  /* If both A and B are valid and same version as TPM, A is selected
   * and B isn't attempted. */
  ResetMocks();
  mpreamble[0].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  mpreamble[1].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  shared->flags |= VBSD_BOOT_RO_NORMAL_SUPPORT;
  TestLoadFirmware(VBERROR_SUCCESS, 0, "Check A then B");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID, "RO normal A valid");
  TEST_EQ(shared->check_fw_b_result, 0, "RO normal B not checked ");
  TEST_EQ(shared->firmware_index, 0, "Boot A");
  TEST_EQ(shared->flags & VBSD_FWB_TRIED, 0, "Didn't try firmware B");
  TEST_EQ(shared->kernel_subkey.algorithm, 7, "Copy kernel subkey");
  /* But if try B flag is set, B is selected and A not attempted */
  ResetMocks();
  mpreamble[0].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  mpreamble[1].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  shared->flags |= VBSD_BOOT_RO_NORMAL_SUPPORT;
  VbNvSet(&vnc, VBNV_TRY_B_COUNT, 5);
  TestLoadFirmware(VBERROR_SUCCESS, 0, "Check B then A");
  TEST_EQ(shared->check_fw_a_result, 0, "RO normal A not checked ");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_VALID, "RO normal B valid");
  TEST_EQ(shared->firmware_index, 1, "Boot B");
  TEST_NEQ(shared->flags & VBSD_FWB_TRIED, 0, "Tried firmware B");
  TEST_EQ(shared->kernel_subkey.algorithm, 8, "Copy kernel subkey");
  VbNvGet(&vnc, VBNV_TRY_B_COUNT, &u);
  TEST_EQ(u, 4, "Used up a try");

  /* If both A and B are valid and grater version than TPM, A is
   * selected and B preamble (but not body) is verified. */
  ResetMocks();
  mpreamble[0].flags = VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL;
  mpreamble[1].flags = 0;
  mpreamble[0].firmware_version = 5;
  mpreamble[1].firmware_version = 6;
  shared->flags |= VBSD_BOOT_RO_NORMAL_SUPPORT;
  TestLoadFirmware(VBERROR_SUCCESS, 0, "Check A then B advancing version");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID, "RO normal A valid");
  TEST_EQ(shared->check_fw_b_result, VBSD_LF_CHECK_HEADER_VALID,
          "RO normal B header valid");
  TEST_EQ(shared->firmware_index, 0, "Boot A");
  TEST_EQ(shared->fw_keyblock_flags, vblock[0].key_block_flags, "Key block A");
  TEST_EQ(shared->kernel_subkey.algorithm, 7, "Copy kernel subkey");
  TEST_EQ(shared->fw_version_lowest, 0x20005, "Lowest valid version");
  TEST_EQ(shared->fw_version_tpm, 0x20005, "TPM version advanced");

  /* Verify firmware data */
  ResetMocks();
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_SUCCESS, 0, "Verify firmware body");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID,
          "Firmware body A valid");
  TEST_EQ(shared->firmware_index, 0, "Boot A shared index");
  TEST_EQ(hash_fw_index, 0, "Hash firmware data A");
  TEST_EQ(digest_size, mpreamble[0].body_signature.data_size,
          "Verified all data expected");

  /* Test error getting firmware body */
  ResetMocks();
  mpreamble[0].body_signature.sig_offset = VBERROR_UNKNOWN;
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_GET_FW_BODY),
                   "Error getting firmware body");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_GET_FW_BODY,
          "Firmware body A");

  /* Test digesting the wrong amount */
  ResetMocks();
  mpreamble[0].body_signature.sig_offset = 42;  /* Mock hashing wrong amount */
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_HASH_WRONG_SIZE),
                   "Hash wrong size");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_HASH_WRONG_SIZE,
          "Firmware hash wrong size A");

  /* Test bad signature */
  ResetMocks();
  mpreamble[0].body_signature.sig_size = 1;  /* Mock bad sig */
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_VERIFY_BODY),
                   "Bad signature");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VERIFY_BODY,
          "Bad signature A");

  /* Test unable to store kernel data key */
  ResetMocks();
  mpreamble[0].kernel_subkey.key_size = VB_SHARED_DATA_MIN_SIZE + 1;
  vblock[1].key_block_flags = 0;  /* Invalid */
  TestLoadFirmware(VBERROR_LOAD_FIRMWARE,
                   (VBNV_RECOVERY_RO_INVALID_RW_CHECK_MIN +
                    VBSD_LF_CHECK_VALID),
                   "Kernel key too big");
  TEST_EQ(shared->check_fw_a_result, VBSD_LF_CHECK_VALID,
          "Kernel key too big");
}


int main(int argc, char* argv[]) {
  int error_code = 0;

  LoadFirmwareTest();

  if (vboot_api_stub_check_memory())
    error_code = 255;
  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
