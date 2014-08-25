/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for ChromeOS-specific GPT partitions, Please see corresponding .c
 * files for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "flash_ts.h"
#include "cgpt.h"

inline int page_to_sector(const nand_geom *nand, int page) {
  return page * (nand->szofpg / nand->szofsector);
}

int nand_read_page(const nand_geom *nand, int page, void *buf, int size) {
  uint8_t *page_buff;

  if (Load((struct drive *)nand->user, &page_buff,
           page_to_sector(nand, page), nand->szofsector,
           (size + nand->szofsector - 1) / nand->szofsector)) {

    // page may be not erased. return default data.
    memset(buf, 0xff, size);
    return 0;
  }
  memcpy(buf, page_buff, size);
  free(page_buff);
  return 0;
}

int nand_write_page(const nand_geom *nand,
                    int page, const void *buf, int buf_size) {
  void *page_buff;
  int ret;
  int sectors = (buf_size + nand->szofsector - 1) / nand->szofsector;
  int size = nand->szofsector * sectors;

  page_buff  = malloc(size);
  if (!page_buff)
    return -1;

  memset(page_buff, 0xff, size);
  memcpy(page_buff, buf, buf_size);

  ret = Save((struct drive *)nand->user, page_buff, page_to_sector(nand, page),
             nand->szofsector, sectors);
  free(page_buff);
  return ret;
}

int nand_erase_block(const nand_geom *nand, int block) {
  int sector = block * (nand->szofblk / nand->szofsector);
  int res;
  void *erase_buff = malloc(nand->szofblk);
  if (!erase_buff)
    return -1;

  memset(erase_buff, 0xff, nand->szofblk);
  res = Save((struct drive *)nand->user, erase_buff, sector,
             nand->szofsector, nand->szofblk / nand->szofsector);
  free(erase_buff);
  return res;
}

int nand_is_bad_block(const nand_geom *nand, int block) {
  return 0;
}
