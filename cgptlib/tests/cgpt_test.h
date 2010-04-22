/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_CGPTLIB_TESTS_CGPT_TEST_H_
#define VBOOT_REFERENCE_CGPTLIB_TESTS_CGPT_TEST_H_

#include <stdio.h>

#define TEST_FAIL -1

/* ANSI Color coding sequences. */
#define COL_GREEN "\e[1;32m"
#define COL_RED "\e[0;31m"
#define COL_STOP "\e[m"

#define EXPECT(expr) \
  if (!expr) { \
    printf(COL_RED " fail " COL_STOP "in expression %s in %s() line %d\n",\
           #expr, __FUNCTION__, __LINE__); \
    return TEST_FAIL; \
  }

#endif  /* VBOOT_REFERENCE_CGPTLIB_TESTS_CGPT_TEST_H_ */
