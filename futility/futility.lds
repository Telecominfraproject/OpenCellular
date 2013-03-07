/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

SECTIONS
{
  .rodata : {
    __futil_cmds_start = .;
    *(SORT(.futil_cmds.*));
    __futil_cmds_end = .;
  }
}
