/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure of Google Binary Block (GBB)
 */

#ifndef VBOOT_REFERENCE_GBB_HEADER_H_
#define VBOOT_REFERENCE_GBB_HEADER_H_

#include "sysincludes.h"

#define GBB_HEADER_SIZE    (0x80)

#define GBB_SIGNATURE      "$GBB"
#define GBB_SIGNATURE_SIZE (4)

#define GBB_MAJOR_VER      (0x01)
#define GBB_MINOR_VER      (0x00)

/* Maximum length of a HWID in bytes, counting terminating null. */
#define GBB_HWID_MAX_SIZE  256

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct GoogleBinaryBlockHeader {
  uint8_t  signature[GBB_SIGNATURE_SIZE]; // GBB_SIGNATURE "$GBB"
  uint16_t major_version;   // see GBB_MAJOR_VER
  uint16_t minor_version;   // see GBB_MINOR_VER
  uint32_t header_size;     // size of GBB header in bytes
  uint32_t reserved;

  uint32_t hwid_offset;     // HWID offset from header
  uint32_t hwid_size;       // HWID size in bytes
  uint32_t rootkey_offset;  // Root Key offset from header
  uint32_t rootkey_size;    // Root Key size in bytes
  uint32_t bmpfv_offset;    // BMP FV offset from header
  uint32_t bmpfv_size;      // BMP FV size in bytes
  uint32_t recovery_key_offset;  // Recovery Key offset from header
  uint32_t recovery_key_size;    // Recovery Key size in bytes

  uint8_t  pad[80];         // to match GBB_HEADER_SIZE
} GoogleBinaryBlockHeader;

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  /* VBOOT_REFERENCE_GBB_HEADER_H_ */
