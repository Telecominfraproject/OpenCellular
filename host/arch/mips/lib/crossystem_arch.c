/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "host_common.h"
#include "crossystem.h"
#include "crossystem_arch.h"

// TODO: Currently these are stub implementations providing reasonable defaults
// wherever possible. They will need real implementation as part of of MIPS
// firmware bringup.

int VbReadNvStorage(VbNvContext* vnc) {
  return -1;
}

int VbWriteNvStorage(VbNvContext* vnc) {
  return -1;
}

VbSharedDataHeader *VbSharedDataRead(void) {
  return NULL;
}

int VbGetArchPropertyInt(const char* name) {
  if (!strcasecmp(name,"devsw_cur")) {
    return 1;
  } else if (!strcasecmp(name,"recoverysw_cur")) {
    return 0;
  } else if (!strcasecmp(name,"wpsw_cur")) {
    return 1;
  } else if (!strcasecmp(name,"devsw_boot")) {
    return 1;
  } else if (!strcasecmp(name,"recoverysw_boot")) {
    return 0;
  } else if (!strcasecmp(name,"recoverysw_ec_boot")) {
    return 0;
  } else if (!strcasecmp(name,"wpsw_boot")) {
    return 1;
  }
  return -1;
}

const char* VbGetArchPropertyString(const char* name, char* dest, size_t size) {
  if (!strcasecmp(name,"hwid")) {
    return StrCopy(dest, "UnknownMipsHwid", size);
  } else if (!strcasecmp(name,"fwid")) {
    return StrCopy(dest, "UnknownMipsFwid", size);
  } else if (!strcasecmp(name,"ro_fwid")) {
    return StrCopy(dest, "UnknownMipsRoFwid", size);
  } else if (!strcasecmp(name,"mainfw_act")) {
    return StrCopy(dest, "A", size);
  } else if (!strcasecmp(name,"mainfw_type")) {
    return StrCopy(dest, "developer", size);
  } else if (!strcasecmp(name,"ecfw_act")) {
    return StrCopy(dest, "RO", size);
  }
  return NULL;
}

int VbSetArchPropertyInt(const char* name, int value) {
  /* All is handled in arch independent fashion */
  return -1;
}

int VbSetArchPropertyString(const char* name, const char* value) {
  /* All is handled in arch independent fashion */
  return -1;
}
