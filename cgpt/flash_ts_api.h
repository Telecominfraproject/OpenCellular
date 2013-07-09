/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _LINUX_FLASH_TS_H_
#define _LINUX_FLASH_TS_H_

#include <asm/ioctl.h>
#include <asm/types.h>

#define FLASH_TS_MAX_KEY_SIZE 64
#define FLASH_TS_MAX_VAL_SIZE 2048

struct flash_ts_io_req {
  char key[FLASH_TS_MAX_KEY_SIZE];
  char val[FLASH_TS_MAX_VAL_SIZE];
};

#define FLASH_TS_IO_MAGIC    0xFE
#define FLASH_TS_IO_SET      _IOW(FLASH_TS_IO_MAGIC, 0, struct flash_ts_io_req)
#define FLASH_TS_IO_GET      _IOWR(FLASH_TS_IO_MAGIC, 1, struct flash_ts_io_req)

#define FTS_DEVICE           "/dev/fts"

#endif  /* _LINUX_FLASH_TS_H_ */
