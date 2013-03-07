/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "futility.h"

static int do_something(int argc, char *argv[])
{
  int i;
  printf("this is %s\n", __func__);
  for (i = 0; i < argc; i++)
    printf("argv[%d] = %s\n", i, argv[i]);
  return 0;
}

DECLARE_FUTIL_COMMAND(foo, do_something, "invoke a foo");
DECLARE_FUTIL_COMMAND(bar, do_something, "go to bar");
DECLARE_FUTIL_COMMAND(hey, do_something, "shout");
