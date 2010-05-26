#include <stdio.h>

#include "firmware_image_fw.h"
#include "kernel_image_fw.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "tlcl.h"

int main(void)
{

  // firmware_image_fw.h
  VerifyFirmwareHeader(0, 0, 0, 0);
  VerifyFirmwarePreamble(0, 0, 0, 0);
  VerifyFirmwareData(0, 0, 0, 0, 0);
  VerifyFirmware(0, 0, 0);
  GetLogicalFirmwareVersion(0);
  VerifyFirmwareDriver_f(0, 0, 0, 0, 0);

  // kernel_image_fw.h
  VerifyKernelHeader(0, 0, 0, 0, 0, 0);
  VerifyKernelConfig(0, 0, 0, 0);
  VerifyKernelData(0, 0, 0, 0, 0);
  VerifyKernel(0, 0, 0);
  GetLogicalKernelVersion(0);
  VerifyKernelDriver_f(0, 0, 0, 0);

  // load_kernel_fw.h
  // FIXME: LoadKernel(0);

  // rollback_index.h
  SetupTPM();
  GetStoredVersion(0);
  WriteStoredVersion(0, 0);
  LockStoredVersion(0);

  // tlcl.h
  TlclLibinit();
  TlclStartup();
  TlclSelftestfull();
  TlclDefineSpace(0, 0, 0);
  TlclWrite(0, 0, 0);
  TlclRead(0, 0, 0);
  TlclWriteLock(0);
  TlclReadLock(0);
  TlclAssertPhysicalPresence();
  TlclSetNvLocked();
  TlclIsOwned();
  TlclForceClear();
  TlclPhysicalEnable();
  TlclPhysicalSetDeactivated(0);
  TlclGetFlags(0, 0);

  return 0;
}
