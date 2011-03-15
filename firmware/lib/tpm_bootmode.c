/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for updating the TPM state with the status of boot path.
 */

#include "tpm_bootmode.h"

#include "tlcl.h"
#include "utility.h"

/* TPM PCR to use for storing boot mode measurements. */
#define BOOT_MODE_PCR 0

/* Input digests for PCR extend.
 * These are calculated as:
 *    SHA1("|Developer_Mode||Recovery_Mode||Keyblock_Mode|").
 * Developer_Mode can be 0 or 1.
 * Recovery_Mode can be 0 or 1.
 * Keyblock flags are defined in vboot_struct.h
 *
 * We map them to Keyblock_Mode as follows:
 *   -----------------------------------------
 *   Keyblock Flags            | Keyblock Mode
 *   -----------------------------------------
 *   6 (Dev-signed firmware)   |     2
 *   7 Normal-signed firmware  |     1
 *   (anything else)           |     0
 */
const uint8_t kBootStateSHA1Digests[][20] = {
  /* SHA1("\x00\x00\x00") */
  "\x29\xe2\xdc\xfb\xb1\x6f\x63\xbb\x02\x54\xdf\x75\x85\xa1\x5b\xb6"
  "\xfb\x5e\x92\x7d",

  /* SHA1("\x00\x00\x01") */
  "\x25\x47\xcc\x73\x6e\x95\x1f\xa4\x91\x98\x53\xc4\x3a\xe8\x90\x86"
  "\x1a\x3b\x32\x64",

  /* SHA1("\x00\x00\x02") */
  "\x1e\xf6\x24\x48\x2d\x62\x0e\x43\xe6\xd3\x4d\xa1\xaf\xe4\x62\x67"
  "\xfc\x69\x5d\x9b",

  /* SHA1("\x00\x01\x00") */
  "\x62\x57\x18\x91\x21\x5b\x4e\xfc\x1c\xea\xb7\x44\xce\x59\xdd\x0b"
  "\x66\xea\x6f\x73",

  /* SHA1("\x00\x01\x01") */
  "\xee\xe4\x47\xed\xc7\x9f\xea\x1c\xa7\xc7\xd3\x4e\x46\x32\x61\xcd"
  "\xa4\xba\x33\x9e",

  /* SHA1("\x00\x01\x02") */
  "\x0c\x7a\x62\x3f\xd2\xbb\xc0\x5b\x06\x42\x3b\xe3\x59\xe4\x02\x1d"
  "\x36\xe7\x21\xad",

  /* SHA1("\x01\x00\x00") */
  "\x95\x08\xe9\x05\x48\xb0\x44\x0a\x4a\x61\xe5\x74\x3b\x76\xc1\xe3"
  "\x09\xb2\x3b\x7f",

  /* SHA1("\x01\x00\x01") */
  "\xc4\x2a\xc1\xc4\x6f\x1d\x4e\x21\x1c\x73\x5c\xc7\xdf\xad\x4f\xf8"
  "\x39\x11\x10\xe9",

  /* SHA1("\x01\x00\x02") */
  "\xfa\x01\x0d\x26\x64\xcc\x5b\x3b\x82\xee\x48\x8f\xe2\xb9\xf5\x0f"
  "\x49\x32\xeb\x8f",

  /* SHA1("\x01\x01\x00") */
  "\x47\xec\x8d\x98\x36\x64\x33\xdc\x00\x2e\x77\x21\xc9\xe3\x7d\x50"
  "\x67\x54\x79\x37",

  /* SHA1("\x01\x01\x01") */
  "\x28\xd8\x6c\x56\xb3\xbf\x26\xd2\x36\x56\x9b\x8d\xc8\xc3\xf9\x1f"
  "\x32\xf4\x7b\xc7",

  /* SHA1("\x01\x01\x02") */
  "\x12\xa3\x40\xd7\x89\x7f\xe7\x13\xfc\x8f\x02\xac\x53\x65\xb8\x6e"
  "\xbf\x35\x31\x78",
};

#define MAX_BOOT_STATE_INDEX (sizeof(kBootStateSHA1Digests)/sizeof(char[20]))

/* Used for PCR extend when the passed-in boot state is invalid or
 * if there is an internal error. */
const uint8_t kBootInvalidSHA1Digest[20] = {
  "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
  "\xff\xff\xff\xff"
};

/* Given the boot state, return the correct SHA1 digest index for TPMExtend
 * in kBootStateSHA1Digests[]. */
int GetBootStateIndex(int dev_mode, int rec_mode, int keyblock_flags) {
  int index = 0;

  /* Convert keyblock flags into keyblock mode which we use to index into
   * kBootStateSHA1Digest[]. */
  switch(keyblock_flags) {
    case 6:  /* KEY_BLOCK_FLAG_RECOVERY_0 | KEY_BLOCK_FLAG_DEVELOPER_1 */
      /* Developer firmware. */
      index = 2;
      break;
    case 7:  /* KEY_BLOCK_FLAG_RECOVERY_0 | KEY_BLOCK_FLAG_DEVELOPER_0
              *  | KEY_BLOCK_FLAGS_DEVELOPER_1 */
      index = 1;
      break;
    default:
      index = 0;  /* Any other keyblock flags. */
  };

  if (rec_mode)
    index += 3;
  if (dev_mode)
    index += 6;
  return index;
}

uint32_t SetTPMBootModeState(int developer_mode, int recovery_mode,
                             int fw_keyblock_flags) {
  uint32_t result;
  const uint8_t* in_digest = NULL;
  uint8_t out_digest[20];  /* For PCR extend output. */
  int digest_index = GetBootStateIndex(developer_mode, recovery_mode,
                                       fw_keyblock_flags);

  if (digest_index >= 0 && digest_index < MAX_BOOT_STATE_INDEX)
    in_digest = kBootStateSHA1Digests[digest_index];
  else
    in_digest = kBootInvalidSHA1Digest;  /* Internal out of bounds error. */
  result = TlclExtend(BOOT_MODE_PCR, in_digest, out_digest);
  VBDEBUG(("TPM: SetTPMBootModeState boot mode PCR out_digest %02x %02x %02x "
           "%02x\n", out_digest, out_digest+1, out_digest+2, out_digest+3));
  return result;
}
