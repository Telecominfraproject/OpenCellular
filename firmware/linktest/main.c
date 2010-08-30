/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgptlib.h"
#include "load_firmware_fw.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "tlcl.h"
#include "vboot_common.h"
#include "vboot_kernel.h"


int main(void)
{
  /* cgptlib.h */
  GptInit(0);
  GptNextKernelEntry(0, 0, 0);
  GptUpdateKernelEntry(0, 0);

  /* load_firmware_fw.h */
  UpdateFirmwareBodyHash(0, 0, 0);
  LoadFirmware(0);

  /* load_kernel_fw.h */
  LoadKernel(0);

  /* rollback_index.h */
  RollbackFirmwareSetup(0, 0);
  RollbackFirmwareWrite(0);
  RollbackFirmwareLock();
  RollbackKernelRecovery(0);
  RollbackKernelRead(0);
  RollbackKernelWrite(0);
  RollbackKernelLock();

  /* tlcl.h */
  TlclLibInit();
  TlclCloseDevice();
  TlclOpenDevice();
  TlclStartup();
  TlclSelfTestFull();
  TlclContinueSelfTest();
  TlclDefineSpace(0, 0, 0);
  TlclWrite(0, 0, 0);
  TlclRead(0, 0, 0);
  TlclWriteLock(0);
  TlclReadLock(0);
  TlclAssertPhysicalPresence();
  TlclSetNvLocked();
  TlclIsOwned();
  TlclForceClear();
  TlclSetEnable();
  TlclClearEnable();
  TlclSetDeactivated(0);
  TlclGetFlags(0, 0, 0);
  TlclSetGlobalLock();
  TlclExtend(0, 0, 0);
  TlclGetPermissions(0, 0);

  /* vboot_common.h */
  OffsetOf(0, 0);
  GetPublicKeyData(0);
  GetPublicKeyDataC(0);
  GetSignatureData(0);
  GetSignatureDataC(0);
  VerifyMemberInside(0, 0, 0, 0, 0, 0);
  VerifyPublicKeyInside(0, 0, 0);
  VerifySignatureInside(0, 0, 0);
  PublicKeyInit(0, 0, 0);
  PublicKeyCopy(0, 0);
  PublicKeyToRSA(0);
  VerifyData(0, 0, 0, 0);
  VerifyDigest(0, 0, 0);
  KeyBlockVerify(0, 0, 0, 0);
  VerifyFirmwarePreamble(0, 0, 0);
  VerifyKernelPreamble(0, 0, 0);

  return 0;
}
