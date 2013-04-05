/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include "test_common.h"

int main(int argc, char *argv[])
{
  TEST_EQ(0, 0, "Not Really A");

  return !gTestSuccess;
}

