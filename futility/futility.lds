/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

SECTIONS
{
  .rodata : {
    . = ALIGN(32);
    futil_cmds = .;                           /* Create the list of commands */
    *(SORT(.futil_cmds.*));
    QUAD(0);                                  /* Null-terminated */
  }
}
