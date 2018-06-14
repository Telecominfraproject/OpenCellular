/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Temporary fix for running the TPM selftest and other essential
 * intializations that we forgot to put in the BIOS.
 *
 * This works in a very specific situation: we assume TPM_Startup has been
 * executed, but we don't know if the self test has run.  On a ST TPM version
 * 1.2.7.0, GetCapabilities fails before the self test has run, so we use that
 * to check if we need to run it.
 *
 * This also enables the TPM if it is disabled, and activates it if it is
 * deactivated.
 *
 * Exit status always 0.  Prints "reboot" to request reboot, "fail" for errors,
 * "success" when everything worked.
 */

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>

#include "tlcl.h"

int main(int argc, char* argv[]) {
  uint32_t result;
  uint8_t disable, deactivated;
  int pri = LOG_USER | LOG_ERR;

  TlclLibInit();
  TlclStartup();        /* ignore result */

  /* On the dogfood device, GetFlags causes an assertion failure because the
   * device uses an older TPM which is not compatible with the current spec.
   * We take advantage of this to cause the program to exit and not run the
   * self test again (which takes 1 second).
   */
  result = TlclGetFlags(NULL, NULL, NULL);

  result = TlclSelfTestFull();
  if (result != 0) {
    syslog(pri, "TPM selftest failed with code 0x%x\n", result);
    printf("fail\n");
    return 0;
  }

  /* Optional one-time enabling of TPM. */
  result = TlclAssertPhysicalPresence();
  if (result != 0) {
    syslog(pri, "TPM assertpp failed with code 0x%x\n", result);
    printf("fail\n");
    return 0;
  }
  result = TlclGetFlags(&disable, &deactivated, NULL);
  if (result != 0) {
    syslog(pri, "TPM getflags failed with code 0x%x\n", result);
    printf("fail\n");
    return 0;
  }
  if (disable) {
    result = TlclSetEnable();
    if (result != 0) {
      syslog(pri, "TPM physical enable failed with code 0x%x\n", result);
      printf("fail\n");
      return 0;
    }
  }
  if (deactivated) {
    result = TlclSetDeactivated(0);
    if (result != 0) {
      syslog(pri, "TPM physical activate failed with code 0x%x\n", result);
      printf("fail\n");
    } else {
      printf("reboot\n");
    }
    return 0; /* needs reboot */
  }
  printf("success\n");
  return 0;
}
