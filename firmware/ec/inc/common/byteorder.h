/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef INC_COMMON_BYTEORDER_H_
#define INC_COMMON_BYTEORDER_H_

/* Detect endianness if using TI compiler */
#ifndef __BYTE_ORDER__
#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__ 4321
#ifdef __little_endian__
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#else
#ifdef __big_endian__
#define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
#else
#error Unable to detect byte order!
#endif
#endif
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
/* Little endian host functions here */
#define htobe16(a) ((((a) >> 8) & 0xff) + (((a) << 8) & 0xff00))
#define betoh16(a) htobe16(a)

#define htobe32(a)                                        \
    ((((a)&0xff000000) >> 24) | (((a)&0x00ff0000) >> 8) | \
     (((a)&0x0000ff00) << 8) | (((a)&0x000000ff) << 24))
#define betoh32(a) htobe32(a)

#define htole16(a) a; // Host is a little endian.
#define letoh16(a) htole16(a)

#else
/* Big endian host functions here */
#define htole16(a) ((((a) >> 8) & 0xff) + (((a) << 8) & 0xff00))
#define letoh16(a) htobe16(a)

#define htole32(a)                                        \
    ((((a)&0xff000000) >> 24) | (((a)&0x00ff0000) >> 8) | \
     (((a)&0x0000ff00) << 8) | (((a)&0x000000ff) << 24))
#define letoh32(a) htobe32(a)

#define htobe16(a) a; // Host is a little endian.
#define betoh16(a) htole16(a)

#endif

#endif /* INC_COMMON_BYTEORDER_H_ */
