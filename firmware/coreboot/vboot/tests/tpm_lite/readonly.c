/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This program mimicks the TPM usage from read-only firmware.  It exercises
 * the TPM functionality needed in the read-only firmware.  It is meant to be
 * integrated with the rest of the read-only firmware.  It is also provided as
 * a test.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tlcl.h"
#include "utility.h"

/* These index values are used to create NVRAM spaces.  They only need to be
 * unique.
 */
#define INDEX0 0xda70
#define INDEX1 0xda71
#define INDEX2 0xda72
#define INDEX3 0xda73

#define INDEX_INITIALIZED 0xda80

/* This is called once at initialization time.  It may be called again from
 * recovery mode to rebuild the spaces if something incomprehensible happened
 * and the spaces are gone or messed up.  This is called after TPM_Startup and
 * before the spaces are write-locked, so there is a chance that they can be
 * recreated (but who knows---if anything can happen, there are plenty of ways
 * of making this FUBAR).
 */
void InitializeSpaces(void) {
  uint32_t zero = 0;
  uint32_t perm = TPM_NV_PER_WRITE_STCLEAR | TPM_NV_PER_PPWRITE;

  printf("Initializing spaces\n");
  TlclSetNvLocked();  /* useful only the first time */

  TlclDefineSpace(INDEX0, perm, 4);
  TlclWrite(INDEX0, (uint8_t *) &zero, 4);
  TlclDefineSpace(INDEX1, perm, 4);
  TlclWrite(INDEX1, (uint8_t *) &zero, 4);
  TlclDefineSpace(INDEX2, perm, 4);
  TlclWrite(INDEX2, (uint8_t *) &zero, 4);
  TlclDefineSpace(INDEX3, perm, 4);
  TlclWrite(INDEX3, (uint8_t *) &zero, 4);

  perm = TPM_NV_PER_READ_STCLEAR | TPM_NV_PER_WRITE_STCLEAR |
    TPM_NV_PER_PPWRITE;
  TlclDefineSpace(INDEX_INITIALIZED, perm, 1);
}


void EnterRecoveryMode(void) {
  printf("entering recovery mode");
  exit(0);
}


int main(int argc, char** argv) {
  uint8_t c;
  uint32_t index_0, index_1, index_2, index_3;

  TlclLibInit();

  TlclStartup();
  TlclSelfTestFull();

  TlclAssertPhysicalPresence();

  /* Checks if initialization has completed by trying to read-lock a space
   * that's created at the end of initialization.
   */
  if (TlclRead(INDEX_INITIALIZED, &c, 0) == TPM_E_BADINDEX) {
    /* The initialization did not complete.
     */
    InitializeSpaces();
  }

  /* Checks if spaces are OK or messed up.
   */
  if (TlclRead(INDEX0, (uint8_t*) &index_0, sizeof(index_0)) != TPM_SUCCESS ||
      TlclRead(INDEX1, (uint8_t*) &index_1, sizeof(index_1)) != TPM_SUCCESS ||
      TlclRead(INDEX2, (uint8_t*) &index_2, sizeof(index_2)) != TPM_SUCCESS ||
      TlclRead(INDEX3, (uint8_t*) &index_3, sizeof(index_3)) != TPM_SUCCESS) {
    EnterRecoveryMode();
  }

  /* Writes space, and locks it.  Then attempts to write again.  I really wish
   * I could use the imperative.
   */
  index_0 += 1;
  if (TlclWrite(INDEX0, (uint8_t*) &index_0, sizeof(index_0) != TPM_SUCCESS)) {
    error("could not write index 0\n");
  }
  TlclWriteLock(INDEX0);
  if (TlclWrite(INDEX0, (uint8_t*) &index_0, sizeof(index_0)) == TPM_SUCCESS) {
    error("index 0 is not locked\n");
  }

  /* Done for now.
   */
  printf("TEST SUCCEEDED\n");
  exit(0);
}
