/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_CGPTLIB_TEST_H_
#define VBOOT_REFERENCE_CGPTLIB_TEST_H_

#include <stdio.h>
#include "sysincludes.h"

enum {
  TEST_FAIL = -1,
  TEST_OK = 0,
};

#define TEST_CASE(func) #func, func
typedef int (*test_func)();

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define EXPECT(expr) \
  do { \
    if (!(expr)) { \
      printf(COL_RED " fail " COL_STOP "in expression %s in %s() line %d\n",\
             #expr, __FUNCTION__, __LINE__); \
      return TEST_FAIL; \
    } \
  } while (0)

#endif  /* VBOOT_REFERENCE_CGPTLIB_TEST_H_ */
