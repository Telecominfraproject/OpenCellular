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

int test_eq(int result, int expected,
	    const char *preamble, const char *desc, const char *comment)
{
  if (result == expected) {
    fprintf(stderr, "%s: %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, comment ? comment : desc);
    return 1;
  } else {
    fprintf(stderr, "%s: %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, comment ? comment : desc);
    fprintf(stderr, "  Expected: 0x%x (%d), got: 0x%x (%d)\n",
	    expected, expected, result, result);
    gTestSuccess = 0;
    return 0;
  }
}

int test_neq(int result, int not_expected,
	     const char *preamble, const char *desc, const char *comment)
{
  if (result != not_expected) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
    return 1;
  } else {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  Didn't expect 0x%x (%d), but got it.\n",
	    not_expected, not_expected);
    gTestSuccess = 0;
    return 0;
  }
}

int test_ptr_eq(const void* result, const void* expected,
		const char *preamble, const char *desc, const char *comment)
{
  if (result == expected) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
    return 1;
  } else {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  Expected: 0x%lx, got: 0x%lx\n", (long)expected,
            (long)result);
    gTestSuccess = 0;
    return 0;
  }
}

int test_ptr_neq(const void* result, const void* not_expected,
		 const char *preamble, const char *desc, const char *comment)
{
  if (result != not_expected) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
    return 1;
  } else {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  Didn't expect 0x%lx, but got it\n",
            (long)not_expected);
    gTestSuccess = 0;
    return 0;
  }
}

int test_str_eq(const char* result, const char* expected,
		const char *preamble, const char *desc, const char *comment)
{
  if (!result || !expected) {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  String compare with NULL\n");
    gTestSuccess = 0;
    return 0;
  } else if (!strcmp(result, expected)) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
    return 1;
  } else {
    fprintf(stderr, "%s " COL_RED "FAILED\n" COL_STOP, comment);
    fprintf(stderr, "  Expected: \"%s\", got: \"%s\"\n", expected,
            result);
    gTestSuccess = 0;
    return 0;
  }
}

int test_str_neq(const char* result, const char* not_expected,
		 const char *preamble, const char *desc, const char *comment)
{
  if (!result || !not_expected) {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  String compare with NULL\n");
    gTestSuccess = 0;
    return 0;
  } else if (strcmp(result, not_expected)) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
    return 1;
  } else {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  Didn't expect: \"%s\", but got it\n", not_expected);
    gTestSuccess = 0;
    return 0;
  }
}

int test_succ(int result,
	      const char *preamble, const char *desc, const char *comment)
{
  if (result == 0) {
    fprintf(stderr, "%s: %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, comment ? comment : desc);
  } else {
    fprintf(stderr, "%s: %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, comment ? comment : desc);
    fprintf(stderr, "  Expected SUCCESS, got: 0x%x (%d)\n", result, result);
    gTestSuccess = 0;
  }
  return !result;
}

int test_true(int result,
	      const char *preamble, const char *desc, const char *comment)
{
  if (result) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
  } else {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  Expected TRUE, got 0\n");
    gTestSuccess = 0;
  }
  return result;
}

int test_false(int result,
	       const char *preamble, const char *desc, const char *comment)
{
  if (!result) {
    fprintf(stderr, "%s: %s, %s ... " COL_GREEN "PASSED\n" COL_STOP,
	    preamble, desc, comment);
  } else {
    fprintf(stderr, "%s: %s, %s ... " COL_RED "FAILED\n" COL_STOP,
	    preamble, desc, comment);
    fprintf(stderr, "  Expected FALSE, got: 0x%lx\n", (long)result);
    gTestSuccess = 0;
  }
  return !result;
}
