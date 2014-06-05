/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions used by tests.
 */

#include "test_common.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "utility.h"

/* Global test success flag. */
int gTestSuccess = 1;

int TEST_EQ(int result, int expected_result, const char* testname) {
  if (result == expected_result) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
    return 1;
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Expected: 0x%x (%d), got: 0x%x (%d)\n",
	    expected_result, expected_result, result, result);
    gTestSuccess = 0;
    return 0;
  }
}

int TEST_NEQ(int result, int not_expected_result, const char* testname) {
  if (result != not_expected_result) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
    return 1;
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Didn't expect 0x%x (%d), but got it.\n",
	    not_expected_result, not_expected_result);
    gTestSuccess = 0;
    return 0;
  }
}

int TEST_PTR_EQ(const void* result, const void* expected_result,
                const char* testname) {
  if (result == expected_result) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
    return 1;
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Expected: 0x%lx, got: 0x%lx\n", (long)expected_result,
            (long)result);
    gTestSuccess = 0;
    return 0;
  }
}

int TEST_PTR_NEQ(const void* result, const void* not_expected_result,
                const char* testname) {
  if (result != not_expected_result) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
    return 1;
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Didn't expect 0x%lx, but got it\n",
            (long)not_expected_result);
    gTestSuccess = 0;
    return 0;
  }
}

int TEST_STR_EQ(const char* result, const char* expected_result,
                const char* testname) {

  if (!result || !expected_result) {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  String compare with NULL\n");
    gTestSuccess = 0;
    return 0;
  } else if (!strcmp(result, expected_result)) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
    return 1;
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Expected: \"%s\", got: \"%s\"\n", expected_result,
            result);
    gTestSuccess = 0;
    return 0;
  }

}

int TEST_SUCC(int result, const char* testname) {
  if (result == 0) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Expected SUCCESS, got: 0x%lx\n", (long)result);
    gTestSuccess = 0;
  }
  return !result;
}

int TEST_TRUE(int result, const char* testname) {
  if (result) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Expected TRUE, got 0\n");
    gTestSuccess = 0;
  }
  return result;
}

int TEST_FALSE(int result, const char* testname) {
  if (!result) {
    fprintf(stderr, "%s Test " COL_GREEN "PASSED\n" COL_STOP, testname);
  } else {
    fprintf(stderr, "%s Test " COL_RED "FAILED\n" COL_STOP, testname);
    fprintf(stderr, "  Expected FALSE, got: 0x%lx\n", (long)result);
    gTestSuccess = 0;
  }
  return !result;
}
