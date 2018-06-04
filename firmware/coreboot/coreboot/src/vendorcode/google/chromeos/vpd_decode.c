/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
#include <assert.h>
#include "lib_vpd.h"

int decodeLen(
    const int32_t max_len,
    const uint8_t *in,
    int32_t *length,
    int32_t *decoded_len) {
  uint8_t more;
  int i = 0;

  assert(length);
  assert(decoded_len);

  *length = 0;
  do {
    if (i >= max_len) return VPD_FAIL;
    more = in[i] & 0x80;
    *length <<= 7;
    *length |= in[i] & 0x7f;
    ++i;
  } while (more);

  *decoded_len = i;

  return VPD_OK;
}

/* Sequentially decodes type, key, and value.
 */
int decodeVpdString(
    const int32_t max_len,
    const uint8_t *input_buf,
    int32_t *consumed,
    VpdDecodeCallback callback,
    void *callback_arg) {
  int type;
  int32_t key_len, value_len;
  int32_t decoded_len;
  const uint8_t *key, *value;

  /* type */
  if (*consumed >= max_len)
    return VPD_FAIL;

  type = input_buf[*consumed];
  switch (type) {
    case VPD_TYPE_INFO:
    case VPD_TYPE_STRING:
      (*consumed)++;

      /* key */
      if (VPD_OK != decodeLen(max_len - *consumed, &input_buf[*consumed],
                              &key_len, &decoded_len) ||
          *consumed + decoded_len >= max_len) {
        return VPD_FAIL;
      }

      *consumed += decoded_len;
      key = &input_buf[*consumed];
      *consumed += key_len;

      /* value */
      if (VPD_OK != decodeLen(max_len - *consumed, &input_buf[*consumed],
                              &value_len, &decoded_len) ||
          *consumed + decoded_len > max_len) {
        return VPD_FAIL;
      }
      *consumed += decoded_len;
      value = &input_buf[*consumed];
      *consumed += value_len;

      if (type == VPD_TYPE_STRING)
        return callback(key, key_len, value, value_len, callback_arg);

      return VPD_OK;

    default:
      return VPD_FAIL;
      break;
  }
  return VPD_OK;
}
