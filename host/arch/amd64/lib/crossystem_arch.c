/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* crossystem is only valid on devices, not on the host, but since
 * it's part of the host library, we need a dummy implementation (all
 * functions return errors) */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include "host_common.h"

#include "crossystem.h"
#include "crossystem_arch.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

int VbReadNvStorage(VbNvContext* vnc) {
  return -1;
}


int VbWriteNvStorage(VbNvContext* vnc) {
  return -1;
}


VbSharedDataHeader* VbSharedDataRead(void) {
  return NULL;
}


int VbGetArchPropertyInt(const char* name) {
  return -1;
}


const char* VbGetArchPropertyString(const char* name, char* dest, int size) {
  return NULL;
}


int VbSetArchPropertyInt(const char* name, int value) {
  return -1;
}


int VbSetArchPropertyString(const char* name, const char* value) {
  return -1;
}
