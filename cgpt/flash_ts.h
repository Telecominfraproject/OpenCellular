/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _FLASH_TS_H
#define _FLASH_TS_H

typedef struct {
  unsigned int szofpg; /* Erase unit */
  unsigned int szofblk; /* Write unit */
  unsigned int szofsector; /* Sector size used by the rest of cgpt */
  void *user;
} nand_geom;

int flash_ts_init(unsigned int start_block, unsigned int blocks,
                  unsigned int szofpg, unsigned int szofblk,
                  unsigned int szofsector, void *user);

/* Get/set value, returns 0 on success */
int flash_ts_set(const char *key, const char *value);
void flash_ts_get(const char *key, char *value, unsigned int size);

/* Get value as an integer, if missing/invalid return 'default_value' */
int flash_ts_get_int(const char *key, int default_value);


/* These must be implemented outside the driver. */
int nand_read_page(const nand_geom *nand, int page, void *buf, int size);
int nand_write_page(const nand_geom *nand, int page, const void *buf, int size);
int nand_erase_block(const nand_geom *nand, int block);
int nand_is_bad_block(const nand_geom *nand, int block);



#endif  /* _FLASH_TS_H */
