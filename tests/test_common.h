/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef VBOOT_REFERENCE_TEST_COMMON_H_
#define VBOOT_REFERENCE_TEST_COMMON_H_

int TEST_EQ(int result, int expected_result, char* testname);
extern int gTestSuccess;

#endif  /* VBOOT_REFERENCE_TEST_COMMON_H_ */
