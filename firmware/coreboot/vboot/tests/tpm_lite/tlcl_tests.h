/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Common definitions for test programs.
 */

#ifndef TLCL_TESTS_H
#define TLCL_TESTS_H

/* Standard testing indexes. */
#define INDEX0 0xcafe
#define INDEX1 0xcaff

/* Prints error and returns on failure */
#define TPM_CHECK(tpm_command) TPM_EXPECT(tpm_command, TPM_SUCCESS)

#define TPM_EXPECT(tpm_command, expected_result) do {          \
  uint32_t _result = (tpm_command);                            \
  uint32_t _exp = (expected_result);                           \
  if (_result != _exp) {                                       \
    printf("TEST FAILED: line %d: " #tpm_command ": 0x%x"      \
           " (expecting 0x%x)\n", __LINE__, _result, _exp);    \
    return _result;                                            \
  }                                                            \
} while (0)


/* Executes TlclStartup(), but ignores POSTINIT error if the
 * TLCL_RESILIENT_STARTUP environment variable is set.
 */
uint32_t TlclStartupIfNeeded(void);

#endif // TLCL_TESTS_H
