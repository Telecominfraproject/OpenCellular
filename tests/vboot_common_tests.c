/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "test_common.h"
#include "vboot_common.h"

/* Test struct packing */
static void StructPackingTest(void) {
  TEST_EQ(EXPECTED_VBPUBLICKEY_SIZE, sizeof(VbPublicKey),
          "sizeof(VbPublicKey)");
  TEST_EQ(EXPECTED_VBSIGNATURE_SIZE, sizeof(VbSignature),
          "sizeof(VbSignature)");
  TEST_EQ(EXPECTED_VBKEYBLOCKHEADER_SIZE, sizeof(VbKeyBlockHeader),
          "sizeof(VbKeyBlockHeader)");
  TEST_EQ(EXPECTED_VBFIRMWAREPREAMBLEHEADER2_0_SIZE,
          sizeof(VbFirmwarePreambleHeader2_0),
          "sizeof(VbFirmwarePreambleHeader)");
  TEST_EQ(EXPECTED_VBFIRMWAREPREAMBLEHEADER2_1_SIZE,
          sizeof(VbFirmwarePreambleHeader),
          "sizeof(VbFirmwarePreambleHeader)");
  TEST_EQ(EXPECTED_VBKERNELPREAMBLEHEADER_SIZE,
          sizeof(VbKernelPreambleHeader), "sizeof(VbKernelPreambleHeader)");
}


/* Helper functions not dependent on specific key sizes */
static void VerifyHelperFunctions(void) {

  {
    uint8_t *p = (uint8_t *)VerifyHelperFunctions;
    TEST_EQ((int)OffsetOf(p, p), 0, "OffsetOf() equal");
    TEST_EQ((int)OffsetOf(p, p+10), 10, "OffsetOf() positive");
    TEST_EQ((int)OffsetOf(p, p+0x12345678), 0x12345678, "OffsetOf() large");
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
    TEST_EQ(VerifyMemberInside(p, 20, p, 6, 11, 3), 0, "MemberInside ok 1");
    TEST_EQ(VerifyMemberInside(p, 20, p+4, 4, 8, 4), 0, "MemberInside ok 2");
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

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  StructPackingTest();
  VerifyHelperFunctions();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
