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

#define DO_ON_FAILURE(tpm_command, action) do {         \
    uint32_t result;                                    \
    if ((result = (tpm_command)) != TPM_SUCCESS) {      \
      action;                                           \
    }                                                   \
  } while (0)

/* Prints error and returns on failure */
#define TPM_CHECK(tpm_command) \
  DO_ON_FAILURE(tpm_command,                                   \
                printf("TEST FAILED: line %d: " #tpm_command ": 0x%x\n", \
                       __LINE__, result); return result)

/* Executes TlclStartup(), but ignores POSTINIT error if the
 * TLCL_RESILIENT_STARTUP environment variable is set.
 */
uint32_t TlclStartupIfNeeded(void);

#endif // TLCL_TESTS_H
