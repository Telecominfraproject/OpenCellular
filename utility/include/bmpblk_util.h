// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VBOOT_REFERENCE_BMPBLK_UTIL_H_
#define VBOOT_REFERENCE_BMPBLK_UTIL_H_

#include "bmpblk_header.h"

int dump_bmpblock(const char *infile, int show_as_yaml,
                  const char *todir, int overwrite);

#endif // VBOOT_REFERENCE_BMPBLK_UTIL_H_
