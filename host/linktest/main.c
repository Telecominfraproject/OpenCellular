#include <stdio.h>

#include "host_common.h"

int main(void)
{
  /* host_key.h */
  PrivateKeyRead(0, 0);
  PrivateKeyFree(0);
  PublicKeyInit(0, 0, 0);
  PublicKeyAlloc(0, 0, 0);
  PublicKeyCopy(0, 0);
  PublicKeyRead(0);
  PublicKeyReadKeyb(0, 0, 0);
  PublicKeyWrite(0, 0);

  /* host_misc.h */
  ReadFile(0, 0);
  WriteFile(0, 0, 0);

  /* host_signature.h */
  SignatureInit(0, 0, 0, 0);
  SignatureAlloc(0, 0);
  SignatureCopy(0, 0);
  CalculateChecksum(0, 0);
  CalculateSignature(0, 0, 0);

  /* host_common.h */
  CreateKeyBlock(0, 0, 0);
  CreateFirmwarePreamble(0, 0, 0, 0);
  CreateKernelPreamble(0, 0, 0, 0, 0, 0, 0);

  return 0;
}
