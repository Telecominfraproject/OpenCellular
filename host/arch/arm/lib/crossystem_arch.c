/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

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
  /* TODO: IMPLEMENT ME! */
  return -1;
}


int VbWriteNvStorage(VbNvContext* vnc) {
  /* TODO: IMPLEMENT ME! */
  return -1;
}


VbSharedDataHeader* VbSharedDataRead(void) {
  /* TODO: IMPLEMENT ME! */
  return NULL;
}


int VbGetArchPropertyInt(const char* name) {

  /* TODO: IMPLEMENT ME!  For now, return reasonable defaults for
   * values where reasonable defaults exist. */
  if (!strcasecmp(name,"recovery_reason")) {
  } else if (!strcasecmp(name,"fmap_base")) {
  }
  /* Switch positions */
  else if (!strcasecmp(name,"devsw_cur")) {
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

  /* Saved memory is at a fixed location for all H2C BIOS.  If the CHSW
   * path exists in sysfs, it's a H2C BIOS. */
  else if (!strcasecmp(name,"savedmem_base")) {
  } else if (!strcasecmp(name,"savedmem_size")) {
  }

  return -1;
}


const char* VbGetArchPropertyString(const char* name, char* dest, int size) {
  /* TODO: IMPLEMENT ME!  For now, return reasonable defaults for
   * values where reasonable defaults exist. */
  if (!strcasecmp(name,"arch")) {
    return StrCopy(dest, "arm", size);
  } else if (!strcasecmp(name,"hwid")) {
    return StrCopy(dest, "UnknownArmHwid", size);
  } else if (!strcasecmp(name,"fwid")) {
    return StrCopy(dest, "UnknownArmFwid", size);
  } else if (!strcasecmp(name,"ro_fwid")) {
    return StrCopy(dest, "UnknownArmRoFwid", size);
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
  /* TODO: IMPLEMENT ME! */
  return -1;
}


int VbSetArchPropertyString(const char* name, const char* value) {
  /* If there were settable architecture-dependent string properties,
   * they'd be here. */
  return -1;
}
