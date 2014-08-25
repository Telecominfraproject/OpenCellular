/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* *** THIS CODE HAS NOT BEEN SECURITY REVIEWED ***
 * It lives in the firmware directory because that's where it needs to go
 * eventually, but at the moment it is used only by usermode tools.
 * Security review must be completed before this code is used in the
 * firmware.
 * See issue 246680
 */

#include "flash_ts.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "utility.h"

// These match the linux driver
#define FLASH_TS_MAGIC    0x53542a46

#define FLASH_TS_HEADER_SIZE 16
#define FLASH_TS_MAX_SIZE 16384
#define FLASH_TS_MAX_ELEMENT_SIZE (FLASH_TS_MAX_SIZE - FLASH_TS_HEADER_SIZE)

typedef struct {
  uint32_t magic;
  uint32_t crc;
  uint32_t length;
  uint32_t version;
  char data[FLASH_TS_MAX_ELEMENT_SIZE];
} __attribute__((packed)) flash_ts;

typedef struct {
  size_t start_block; // Partition start offset (in erase blocks)
  size_t end_block;   // Partition end offset (in erase blocks)
  size_t chunk_size;  // Minimum element size
  size_t pages_per_block, chunks_per_block, pages_per_chunk;
  nand_geom nand;

  size_t cached_block;
  size_t current_block;

  flash_ts current;
  flash_ts temp;
} flash_ts_state;


static flash_ts_state state;

size_t pow2(size_t x) {
  size_t v = 1;
  while (v < x)
    v <<= 1;
  return v;
}

static inline uint32_t flash_ts_crc(const flash_ts *cache)
{
  const unsigned char *p;
  uint32_t crc = 0;
  size_t len;

  /* skip magic and crc fields */
  len = cache->length + 2 * sizeof(uint32_t);
  p = (const unsigned char*)&cache->length;

  while (len--) {
    int i;

    crc ^= *p++;
    for (i = 0; i < 8; i++)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
  }
  return crc ^ ~0;
}

static inline int flash_ts_check_crc(const flash_ts *ts) {
  return ts->crc == flash_ts_crc(ts);
}

static int is_blank(const void *ptr, size_t sz) {
  const unsigned char *p = (const unsigned char*)ptr;
  const unsigned char *end = p + sz;
  while (p < end)
    if (*p++ != 0xff)
      return 0;
  return 1;
}

static int is_pow2(size_t v) {
  return v && (v & (v - 1)) == 0;
}

/* Scan the entire partition to find the latest version */
static void flash_ts_scan_partition(flash_ts_state *ts) {
  size_t block;

  for (block = ts->start_block; block < ts->end_block; block++) {
    if (!nand_is_bad_block(&ts->nand, block)) {
      size_t chunk;
      size_t page_base = block * ts->pages_per_block;

      for (chunk = 0; chunk < ts->chunks_per_block;
           chunk++, page_base += ts->pages_per_chunk) {
        if (nand_read_page(&ts->nand, page_base,
                            &ts->temp, sizeof(ts->temp))) {
          continue;
        }
        if (ts->temp.magic != FLASH_TS_MAGIC ||
            ts->temp.version <= ts->current.version ||
            !flash_ts_check_crc(&ts->temp)) {
          if (is_blank(&ts->temp, sizeof(ts->temp))) {
            // Since we only write sequentially, a blank chunk means no more
            // data in this block.
            break;
          }
          continue;
        }

        // It's good & newer than our current version
        VBDEBUG(("Found good version %d\n", ts->temp.version));
        ts->current_block = block;
        Memcpy(&ts->current, &ts->temp, sizeof(ts->current));
      }
    }
  }
}

static char *flash_ts_search(flash_ts *ts, const char *key) {
  char *str = &ts->data[0];
  size_t keylen = strlen(key);

  while(*str && str + keylen < &ts->data[ts->length]) {
    // Format: name=value\0name2=value2\0 ... keyn=valuen\0\0
    if (!Memcmp(str, key, keylen) && str[keylen] == '=') {
      return &str[keylen + 1];
    } else {
      str += strlen(str) + 1; // Skip to next key
    }
  }
  return NULL;
}

static int flash_ts_find_writeable_chunk(flash_ts_state *ts, uint32_t block) {
  uint32_t page_base = block * ts->pages_per_block;
  uint32_t page_end = (block + 1) * ts->pages_per_block;

  for(; page_base < page_end; page_base += ts->pages_per_chunk) {
    if(!nand_read_page(&ts->nand, page_base,
       &ts->temp, sizeof(ts->temp))) {
      if (is_blank(&ts->temp, sizeof(ts->temp)))
        return page_base;
    }
  }

  return -1;
}

static int in_range(const flash_ts_state *ts, uint32_t block) {
  return block >= ts->start_block && block < ts->end_block;
}

static int flash_try_write(flash_ts_state *ts, uint32_t page) {
  return nand_write_page(&ts->nand, page, &ts->current, sizeof(ts->current)) ||
         nand_read_page(&ts->nand, page, &ts->temp, sizeof(ts->temp)) ||
         Memcmp(&ts->current, &ts->temp, sizeof(ts->current));
}


static int flash_ts_find_writeable_spot(flash_ts_state *ts,
                                        uint32_t *page_ofs) {
  uint32_t block;
  if (in_range(ts, ts->cached_block)) {
    // We have a starting position to scan from
    block = ts->cached_block;
  } else {
    block = ts->start_block;
    VBDEBUG(("Cached block not in range - starting from %u\n", block));
  }
  for (; block < ts->end_block; block++) {
    int chunk;
    if (nand_is_bad_block(&ts->nand, block)) {
      VBDEBUG(("Skipping bad block %u\n", block));
      continue;
    }

    chunk = flash_ts_find_writeable_chunk(ts, block);
    if (chunk < 0) {
      VBDEBUG(("No free chunks in block %u\n", block));
      continue;
    }

    VBDEBUG(("Free chunk %d in block %u\n", chunk, block));
    *page_ofs = chunk;
    ts->cached_block = block;
    return 0;
  }
  return -1;
}

static int flash_try_erase(flash_ts_state *ts, int block) {
  return nand_is_bad_block(&ts->nand, block) ||
         nand_erase_block(&ts->nand, block);
}

static int flash_erase_any_block(flash_ts_state *ts, uint32_t hint) {
  uint32_t block;
  for (block = hint; block < ts->end_block; block++) {
    if (!flash_try_erase(ts, block)) {
      ts->cached_block = block;
      VBDEBUG(("Erased block %u\n", block));
      return 0;
    }
  }

  if (hint > ts->end_block)
    hint = ts->end_block;

  for (block = ts->start_block; block < hint; block++) {
    if (!flash_try_erase(ts, block)) {
      ts->cached_block = block;
      VBDEBUG(("Erased block %u\n", block));
      return 0;
    }
  }
  return -1;
}

static int flash_ts_write(flash_ts_state *ts) {
  int passes = 3;
  uint32_t page;


  ts->cached_block = ts->current_block;
  ts->current.version++;
  ts->current.crc = flash_ts_crc(&ts->current);
  VBDEBUG(("flash_ts_write() - %u bytes, crc %08X\n",
          ts->current.length, ts->current.crc));

  while(passes--) {
    if (flash_ts_find_writeable_spot(ts, &page)) {
      if (ts->cached_block == ts->end_block) {
        uint32_t block;

        // Partition full!
        // Erase a block to get some space
        if (in_range(ts, ts->current_block) &&
            ts->current_block != ts->end_block - 1) {
          // We don't want to overwrite our good copy if we can avoid it.
          block = ts->current_block + 1;
        } else {
          block = ts->start_block;
        }
        VBDEBUG(("Partition full - begin erasing from block %u\n", block));

        // Erase block, and try again.
        if (flash_erase_any_block(ts, block)) {
          // Failed to erase anything, so abort.
          VBDEBUG(("All erases failed, aborting\n"));
          return -ENOMEM;
        }
        continue;
      } else {
        // Try again, re-scan everything.
        ts->cached_block = ts->end_block;
        continue;
      }
    }

    if (flash_try_write(ts, page)) {
      // Write failure, or read-back failure, try again with the next block.
      VBDEBUG(("Write failure, retry\n"));
      ts->cached_block++;
      continue;
    }

    VBDEBUG(("Successfully written v%u @ %u\n", ts->current.version, page));
    ts->current_block = ts->cached_block;
    return 0;
  }

  VBDEBUG(("Out of tries\n"));
  return -EAGAIN;
}

// Set value, returns 0 on success
int flash_ts_set(const char *key, const char *value) {
  flash_ts *ts = &state.current;
  char *at;
  size_t keylen = strlen(key);
  size_t value_len = strlen(value);

  if (keylen == 0) {
    VBDEBUG(("0-length key - illegal\n"));
    return -1;
  }

  if (strchr(key, '=')) {
    VBDEBUG(("key contains '=' - illegal\n"));
    return -1;
  }

  Memcpy(&state.temp, &state.current, sizeof(state.temp));

  at = flash_ts_search(ts, key);
  if (at) {
    size_t old_value_len;

    // Already exists
    if (!strcmp(at, value)) {
      // No change
      VBDEBUG(("Values are the same, not writing\n"));
      return 0;
    }

    old_value_len = strlen(at);
    if (value_len == old_value_len) {
      // Overwrite it
      Memcpy(at, value, value_len);
      VBDEBUG(("Values are the same length, overwrite\n"));
    } else {
      // Remove it
      // if value_len == 0, then we're done
      // if value_len != old_value_len, then we do the append below
      char *src = at - (keylen + 1);
      char *end = &ts->data[ts->length];
      char *from = at + old_value_len + 1;

      VBDEBUG(("Delete old value\n"));
      memmove(src, from, end - from);
      ts->length -= (from-src);
      ts->data[ts->length - 1] = '\0';
      at = NULL; // Enter the append branch below
    }
  } else if (value_len == 0) {
    // Removing non-existent entry
    return 0;
  }

  if (!at && value_len > 0) {
    // Append it

    if (ts->length + keylen + 1 + value_len + 1 > FLASH_TS_MAX_ELEMENT_SIZE) {
      // Not enough space, restore previous
      VBDEBUG(("Not enough space to write %d data bytes\n", (int)value_len));
      Memcpy(&state.current, &state.temp, sizeof(state.temp));
      return -1;
    }

    VBDEBUG(("Append new value\n"));
    at = &ts->data[ts->length - 1];
    strcpy(at, key);
    at[keylen] = '=';
    strcpy(at + keylen + 1, value);
    ts->length += keylen + 1 + value_len + 1;
    ts->data[ts->length-1] = '\0';
  }

  return flash_ts_write(&state);
}

void flash_ts_get(const char *key, char *value, unsigned int size) {
  flash_ts_state *ts = &state;
  const char *at;

  at = flash_ts_search(&ts->current, key);
  if (at) {
    strncpy(value, at, size);
  } else {
    *value = '\0';
  }
}

int flash_ts_init(unsigned int start_block, unsigned int blocks,
                  unsigned int szofpg, unsigned int szofblk,
                  unsigned int szofsector, void *user) {
  flash_ts_state *ts = &state;

  if (!is_pow2(szofpg) || !is_pow2(szofblk) || !is_pow2(szofsector) ||
      szofsector > szofpg || szofpg > szofblk || blocks == 0)
    return -ENODEV;

  Memset(ts, 0, sizeof(*ts));

  // Page <= chunk <= block
  // Page is minimum writable unit
  // Chunk is actual write unit
  // Block is erase unit
  ts->start_block = start_block;
  ts->end_block = start_block + blocks;
  ts->pages_per_block = szofblk / szofpg;

  ts->nand.user = user;
  ts->nand.szofpg = szofpg;
  ts->nand.szofblk = szofblk;
  ts->nand.szofsector = szofsector;

  // Calculate our write size, this mirrors the linux driver's logic
  ts->chunk_size = pow2((sizeof(flash_ts) + szofpg - 1) & ~(szofpg - 1));
  if (!is_pow2(ts->chunk_size))
    return -ENODEV;

  ts->pages_per_chunk = ts->chunk_size / szofpg;
  if (ts->pages_per_chunk == 0 || ts->chunk_size > szofblk)
    return -ENODEV;

  ts->chunks_per_block = szofblk / ts->chunk_size;

  ts->current.version = 0;
  ts->current.length = 1;
  ts->current.magic = FLASH_TS_MAGIC;
  ts->current.crc = flash_ts_crc(&ts->current);
  ts->current.data[0] = '\0';
  ts->current_block = ts->end_block;

  flash_ts_scan_partition(ts);

  return 0;
}
