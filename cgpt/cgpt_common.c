/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for ChromeOS-specific GPT partitions, Please see corresponding .c
 * files for more details.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cgpt.h"
#include "cgptlib_internal.h"
#include "crc32.h"
#include "flash_ts.h"
#include "flash_ts_api.h"
#include "vboot_host.h"

struct nand_layout nand;

void EnableNandImage(int bytes_per_page, int pages_per_block,
                     int fts_block_offset, int fts_block_size) {
  nand.enabled = 1;
  nand.bytes_per_page = bytes_per_page;
  nand.pages_per_block = pages_per_block;
  nand.fts_block_offset = fts_block_offset;
  nand.fts_block_size = fts_block_size;
}

void Error(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
}

int CheckValid(const struct drive *drive) {
  if ((drive->gpt.valid_headers != MASK_BOTH) ||
      (drive->gpt.valid_entries != MASK_BOTH)) {
	  fprintf(stderr,
		  "\nWARNING: one of the GPT header/entries is invalid\n\n");
    return CGPT_FAILED;
  }
  return CGPT_OK;
}

int Load(struct drive *drive, uint8_t **buf,
                const uint64_t sector,
                const uint64_t sector_bytes,
                const uint64_t sector_count) {
  int count;  /* byte count to read */
  int nread;
  int fd = drive->fd;

  require(buf);
  if (!sector_count || !sector_bytes) {
    Error("%s() failed at line %d: sector_count=%d, sector_bytes=%d\n",
          __FUNCTION__, __LINE__, sector_count, sector_bytes);
    return CGPT_FAILED;
  }
  /* Make sure that sector_bytes * sector_count doesn't roll over. */
  if (sector_bytes > (UINT64_MAX / sector_count)) {
    Error("%s() failed at line %d: sector_count=%d, sector_bytes=%d\n",
          __FUNCTION__, __LINE__, sector_count, sector_bytes);
    return CGPT_FAILED;
  }
  count = sector_bytes * sector_count;
  *buf = malloc(count);
  require(*buf);

  if (-1 == lseek(fd, sector * sector_bytes, SEEK_SET)) {
    Error("Can't lseek: %s\n", strerror(errno));
    goto error_free;
  }

  nread = read(fd, *buf, count);
  if (nread < count) {
    Error("Can't read enough: %d, not %d\n", nread, count);
    goto error_free;
  }

  return CGPT_OK;

error_free:
  free(*buf);
  *buf = 0;
  return CGPT_FAILED;
}


int ReadPMBR(struct drive *drive) {
  if (-1 == lseek(drive->fd, 0, SEEK_SET))
    return CGPT_FAILED;

  int nread = read(drive->fd, &drive->pmbr, sizeof(struct pmbr));
  if (nread != sizeof(struct pmbr))
    return CGPT_FAILED;

  return CGPT_OK;
}

int WritePMBR(struct drive *drive) {
  if (-1 == lseek(drive->fd, 0, SEEK_SET))
    return CGPT_FAILED;

  int nwrote = write(drive->fd, &drive->pmbr, sizeof(struct pmbr));
  if (nwrote != sizeof(struct pmbr))
    return CGPT_FAILED;

  return CGPT_OK;
}

int Save(struct drive *drive, const uint8_t *buf,
                const uint64_t sector,
                const uint64_t sector_bytes,
                const uint64_t sector_count) {
  int count;  /* byte count to write */
  int nwrote;
  int fd = drive->fd;

  require(buf);
  count = sector_bytes * sector_count;

  if (-1 == lseek(fd, sector * sector_bytes, SEEK_SET))
    return CGPT_FAILED;

  nwrote = write(fd, buf, count);
  if (nwrote < count)
    return CGPT_FAILED;

  return CGPT_OK;
}

static int get_hex_char_value(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  return -1;
}

int TryInitMtd(const char *dev) {
  static int already_inited = 0;
  if (already_inited)
    return nand.use_host_ioctl;

  already_inited = 1;

  /* If we're running on the live system, we can just use /dev/fts and not
   * actually need the specific parameters. This needs to be accessed via
   * ioctl and not normal I/O.
   */
  if (!strcmp(dev, FTS_DEVICE) && !access(FTS_DEVICE, R_OK | W_OK)) {
    nand.enabled = 1;
    nand.use_host_ioctl = 1;
    return 1;
  }
  return 0;
}

int FlashGet(const char *key, uint8_t *data, uint32_t *bufsz) {
  char *hex = (char*)malloc(*bufsz * 2);
  char *read;
  uint32_t written = 0;

  if (nand.use_host_ioctl) {
    struct flash_ts_io_req req;
    strncpy(req.key, key, sizeof(req.key));
    int fd = open(FTS_DEVICE, O_RDWR);
    if (fd < 0)
      return -1;
    if (ioctl(fd, FLASH_TS_IO_GET, &req))
      return -1;
    strncpy(hex, req.val, *bufsz * 2);
    close(fd);
  } else {
    flash_ts_get(key, hex, *bufsz * 2);
  }

  /* Hex -> binary */
  for (read = hex; read < hex + *bufsz * 2 && *read != '\0'; read += 2) {
    int c0, c1;
    c0 = get_hex_char_value(read[0]);
    c1 = get_hex_char_value(read[1]);
    if (c0 < 0 || c1 < 0) {
      free(hex);
      return -1;
    }

    data[written++] = (c0 << 4) + c1;
  }
  *bufsz = written;
  free(hex);
  return 0;
}

int FlashSet(const char *key, const uint8_t *data, uint32_t bufsz) {
  char *hex = (char*)malloc(bufsz * 2 + 1);
  const char *hex_chars = "0123456789ABCDEF";
  int ret;
  uint32_t i;

  /* Binary -> hex, we need some encoding because FTS only stores C strings */
  for (i = 0; i < bufsz; i++) {
    hex[i * 2] = hex_chars[data[i] >> 4];
    hex[i * 2 + 1] = hex_chars[data[i] & 0xF];
  }
  /* Buffer must be NUL-terminated. */
  hex[bufsz * 2] = '\0';
  if (nand.use_host_ioctl) {
    struct flash_ts_io_req req;
    strncpy(req.key, key, sizeof(req.key));
    strncpy(req.val, hex, sizeof(req.val));
    free(hex);
    int fd = open(FTS_DEVICE, O_RDWR);
    if (fd < 0)
      return -1;
    if (ioctl(fd, FLASH_TS_IO_SET, &req))
      return -1;
    close(fd);
    return 0;
  }
  ret = flash_ts_set(key, hex);
  free(hex);
  return ret;
}

int MtdLoad(struct drive *drive, int sector_bytes) {
  int ret;
  uint32_t sz;
  MtdData *mtd = &drive->mtd;

  mtd->sector_bytes = sector_bytes;
  mtd->drive_sectors = drive->size / mtd->sector_bytes;

  if (!nand.use_host_ioctl) {
    ret = flash_ts_init(mtd->fts_block_offset,
                        mtd->fts_block_size,
                        mtd->flash_page_bytes,
                        mtd->flash_block_bytes,
                        mtd->sector_bytes, /* Needed for Load() and Save() */
                        drive);
    if (ret)
      return ret;
  }

  memset(&mtd->primary, 0, sizeof(mtd->primary));
  sz = sizeof(mtd->primary);
  ret = FlashGet(MTD_DRIVE_SIGNATURE, (uint8_t *)&mtd->primary, &sz);
  if (ret)
    return ret;

  /* Read less than expected */
  if (sz < MTD_DRIVE_V1_SIZE)
    memset(&mtd->primary, 0, sizeof(mtd->primary));

  if (nand.use_host_ioctl) {
    /* If we are using /dev/fts, we can't stat() the size, so re-use
     * our internal value to set it.
     */
    drive->size = mtd->primary.last_offset + 1;
    mtd->drive_sectors = drive->size / mtd->sector_bytes;
  }

  mtd->current_kernel = -1;
  mtd->current_priority = 0;
  mtd->modified = 0;
  return 0;
}

int MtdSave(struct drive *drive) {
  MtdData *mtd = &drive->mtd;

  if (!mtd->modified)
    return 0;

  mtd->primary.crc32 = 0;
  mtd->primary.crc32 = Crc32(&mtd->primary, MTD_DRIVE_V1_SIZE);

  return FlashSet(MTD_DRIVE_SIGNATURE, (uint8_t *)&mtd->primary,
                  sizeof(mtd->primary));
}

int GptLoad(struct drive *drive, uint32_t sector_bytes) {
  drive->gpt.sector_bytes = sector_bytes;
  if (drive->size % drive->gpt.sector_bytes) {
    Error("Media size (%llu) is not a multiple of sector size(%d)\n",
          (long long unsigned int)drive->size, drive->gpt.sector_bytes);
    return -1;
  }
  drive->gpt.drive_sectors = drive->size / drive->gpt.sector_bytes;

  // Read the data.
  if (CGPT_OK != Load(drive, &drive->gpt.primary_header,
                      GPT_PMBR_SECTOR,
                      drive->gpt.sector_bytes, GPT_HEADER_SECTOR)) {
    return -1;
  }
  if (CGPT_OK != Load(drive, &drive->gpt.secondary_header,
                      drive->gpt.drive_sectors - GPT_PMBR_SECTOR,
                      drive->gpt.sector_bytes, GPT_HEADER_SECTOR)) {
    return -1;
  }
  if (CGPT_OK != Load(drive, &drive->gpt.primary_entries,
                      GPT_PMBR_SECTOR + GPT_HEADER_SECTOR,
                      drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS)) {
    return -1;
  }
  if (CGPT_OK != Load(drive, &drive->gpt.secondary_entries,
                      drive->gpt.drive_sectors - GPT_HEADER_SECTOR
                      - GPT_ENTRIES_SECTORS,
                      drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS)) {
    return -1;
  }
  return 0;
}

int GptSave(struct drive *drive) {
  int errors = 0;
  if (drive->gpt.modified & GPT_MODIFIED_HEADER1) {
    if (CGPT_OK != Save(drive, drive->gpt.primary_header,
                        GPT_PMBR_SECTOR,
                        drive->gpt.sector_bytes, GPT_HEADER_SECTOR)) {
      errors++;
      Error("Cannot write primary header: %s\n", strerror(errno));
    }
  }

  if (drive->gpt.modified & GPT_MODIFIED_HEADER2) {
    if(CGPT_OK != Save(drive, drive->gpt.secondary_header,
                       drive->gpt.drive_sectors - GPT_PMBR_SECTOR,
                       drive->gpt.sector_bytes, GPT_HEADER_SECTOR)) {
      errors++;
      Error("Cannot write secondary header: %s\n", strerror(errno));
    }
  }
  if (drive->gpt.modified & GPT_MODIFIED_ENTRIES1) {
    if (CGPT_OK != Save(drive, drive->gpt.primary_entries,
                        GPT_PMBR_SECTOR + GPT_HEADER_SECTOR,
                        drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS)) {
      errors++;
      Error("Cannot write primary entries: %s\n", strerror(errno));
    }
  }
  if (drive->gpt.modified & GPT_MODIFIED_ENTRIES2) {
    if (CGPT_OK != Save(drive, drive->gpt.secondary_entries,
                        drive->gpt.drive_sectors - GPT_HEADER_SECTOR
                        - GPT_ENTRIES_SECTORS,
                        drive->gpt.sector_bytes, GPT_ENTRIES_SECTORS)) {
      errors++;
      Error("Cannot write secondary entries: %s\n", strerror(errno));
    }
  }

  if (drive->gpt.primary_header)
    free(drive->gpt.primary_header);
  drive->gpt.primary_header = 0;
  if (drive->gpt.primary_entries)
    free(drive->gpt.primary_entries);
  drive->gpt.primary_entries = 0;
  if (drive->gpt.secondary_header)
    free(drive->gpt.secondary_header);
  drive->gpt.secondary_header = 0;
  if (drive->gpt.secondary_entries)
    free(drive->gpt.secondary_entries);
  drive->gpt.secondary_entries = 0;
  return errors ? -1 : 0;
}


// Opens a block device or file, loads raw GPT data from it.
// mode should be O_RDONLY or O_RDWR
//
// Returns CGPT_FAILED if any error happens.
// Returns CGPT_OK if success and information are stored in 'drive'. */
int DriveOpen(const char *drive_path, struct drive *drive, int mode) {
  struct stat stat;
  uint32_t sector_bytes;
  int is_mtd = nand.enabled;

  require(drive_path);
  require(drive);

  // Clear struct for proper error handling.
  memset(drive, 0, sizeof(struct drive));

  if (TryInitMtd(drive_path)) {
    is_mtd = 1;
    sector_bytes = 512;  /* bytes */
  } else {
    drive->fd = open(drive_path, mode | O_LARGEFILE | O_NOFOLLOW);
    if (drive->fd == -1) {
      Error("Can't open %s: %s\n", drive_path, strerror(errno));
      return CGPT_FAILED;
    }

    if (fstat(drive->fd, &stat) == -1) {
      Error("Can't fstat %s: %s\n", drive_path, strerror(errno));
      goto error_close;
    }
    if ((stat.st_mode & S_IFMT) != S_IFREG) {
      if (ioctl(drive->fd, BLKGETSIZE64, &drive->size) < 0) {
        Error("Can't read drive size from %s: %s\n", drive_path,
              strerror(errno));
        goto error_close;
      }
      if (ioctl(drive->fd, BLKSSZGET, &sector_bytes) < 0) {
        Error("Can't read sector size from %s: %s\n",
              drive_path, strerror(errno));
        goto error_close;
      }
    } else {
      sector_bytes = 512;  /* bytes */
      drive->size = stat.st_size;
    }
  }
  drive->is_mtd = is_mtd;

  if (is_mtd) {
    drive->mtd.fts_block_offset = nand.fts_block_offset;
    drive->mtd.fts_block_size = nand.fts_block_size;
    drive->mtd.flash_page_bytes = nand.bytes_per_page;
    drive->mtd.flash_block_bytes = nand.pages_per_block * nand.bytes_per_page;
    if (MtdLoad(drive, sector_bytes)) {
      goto error_close;
    }
  } else {
    if (GptLoad(drive, sector_bytes)) {
      goto error_close;
    }
  }

  // We just load the data. Caller must validate it.
  return CGPT_OK;

error_close:
  (void) DriveClose(drive, 0);
  return CGPT_FAILED;
}


int DriveClose(struct drive *drive, int update_as_needed) {
  int errors = 0;

  if (update_as_needed) {
    if (drive->is_mtd) {
      if (MtdSave(drive)) {
        errors++;
      }
    } else {
      if (GptSave(drive)) {
        errors++;
      }
    }
  }

  // Sync early! Only sync file descriptor here, and leave the whole system sync
  // outside cgpt because whole system sync would trigger tons of disk accesses
  // and timeout tests.
  fsync(drive->fd);

  close(drive->fd);

  return errors ? CGPT_FAILED : CGPT_OK;
}


/* GUID conversion functions. Accepted format:
 *
 *   "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
 *
 * Returns CGPT_OK if parsing is successful; otherwise CGPT_FAILED.
 */
int StrToGuid(const char *str, Guid *guid) {
  uint32_t time_low;
  uint16_t time_mid;
  uint16_t time_high_and_version;
  unsigned int chunk[11];

  if (11 != sscanf(str, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                   chunk+0,
                   chunk+1,
                   chunk+2,
                   chunk+3,
                   chunk+4,
                   chunk+5,
                   chunk+6,
                   chunk+7,
                   chunk+8,
                   chunk+9,
                   chunk+10)) {
    printf("FAILED\n");
    return CGPT_FAILED;
  }

  time_low = chunk[0] & 0xffffffff;
  time_mid = chunk[1] & 0xffff;
  time_high_and_version = chunk[2] & 0xffff;

  guid->u.Uuid.time_low = htole32(time_low);
  guid->u.Uuid.time_mid = htole16(time_mid);
  guid->u.Uuid.time_high_and_version = htole16(time_high_and_version);

  guid->u.Uuid.clock_seq_high_and_reserved = chunk[3] & 0xff;
  guid->u.Uuid.clock_seq_low = chunk[4] & 0xff;
  guid->u.Uuid.node[0] = chunk[5] & 0xff;
  guid->u.Uuid.node[1] = chunk[6] & 0xff;
  guid->u.Uuid.node[2] = chunk[7] & 0xff;
  guid->u.Uuid.node[3] = chunk[8] & 0xff;
  guid->u.Uuid.node[4] = chunk[9] & 0xff;
  guid->u.Uuid.node[5] = chunk[10] & 0xff;

  return CGPT_OK;
}
void GuidToStr(const Guid *guid, char *str, unsigned int buflen) {
  require(buflen >= GUID_STRLEN);
  require(snprintf(str, buflen,
                  "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                  le32toh(guid->u.Uuid.time_low),
                  le16toh(guid->u.Uuid.time_mid),
                  le16toh(guid->u.Uuid.time_high_and_version),
                  guid->u.Uuid.clock_seq_high_and_reserved,
                  guid->u.Uuid.clock_seq_low,
                  guid->u.Uuid.node[0], guid->u.Uuid.node[1],
                  guid->u.Uuid.node[2], guid->u.Uuid.node[3],
                  guid->u.Uuid.node[4], guid->u.Uuid.node[5]) == GUID_STRLEN-1);
}

/* Convert possibly unterminated UTF16 string to UTF8.
 * Caller must prepare enough space for UTF8, which could be up to
 * twice the byte length of UTF16 string plus the terminating '\0'.
 * See the following table for encoding lengths.
 *
 *     Code point       UTF16       UTF8
 *   0x0000-0x007F     2 bytes     1 byte
 *   0x0080-0x07FF     2 bytes     2 bytes
 *   0x0800-0xFFFF     2 bytes     3 bytes
 *  0x10000-0x10FFFF   4 bytes     4 bytes
 *
 * This function uses a simple state meachine to convert UTF-16 char(s) to
 * a code point. Once a code point is parsed out, the state machine throws
 * out sequencial UTF-8 chars in one time.
 *
 * Return: CGPT_OK --- all character are converted successfully.
 *         CGPT_FAILED --- convert error, i.e. output buffer is too short.
 */
int UTF16ToUTF8(const uint16_t *utf16, unsigned int maxinput,
                uint8_t *utf8, unsigned int maxoutput)
{
  size_t s16idx, s8idx;
  uint32_t code_point = 0;
  int code_point_ready = 1;  // code point is ready to output.
  int retval = CGPT_OK;

  if (!utf16 || !maxinput || !utf8 || !maxoutput)
    return CGPT_FAILED;

  maxoutput--;                             /* plan for termination now */

  for (s16idx = s8idx = 0;
       s16idx < maxinput && utf16[s16idx] && maxoutput;
       s16idx++) {
    uint16_t codeunit = le16toh(utf16[s16idx]);

    if (code_point_ready) {
      if (codeunit >= 0xD800 && codeunit <= 0xDBFF) {
        /* high surrogate, need the low surrogate. */
        code_point_ready = 0;
        code_point = (codeunit & 0x03FF) + 0x0040;
      } else {
        /* BMP char, output it. */
        code_point = codeunit;
      }
    } else {
      /* expect the low surrogate */
      if (codeunit >= 0xDC00 && codeunit <= 0xDFFF) {
        code_point = (code_point << 10) | (codeunit & 0x03FF);
        code_point_ready = 1;
      } else {
        /* the second code unit is NOT the low surrogate. Unexpected. */
        code_point_ready = 0;
        retval = CGPT_FAILED;
        break;
      }
    }

    /* If UTF code point is ready, output it. */
    if (code_point_ready) {
      require(code_point <= 0x10FFFF);
      if (code_point <= 0x7F && maxoutput >= 1) {
        maxoutput -= 1;
        utf8[s8idx++] = code_point & 0x7F;
      } else if (code_point <= 0x7FF && maxoutput >= 2) {
        maxoutput -= 2;
        utf8[s8idx++] = 0xC0 | (code_point >> 6);
        utf8[s8idx++] = 0x80 | (code_point & 0x3F);
      } else if (code_point <= 0xFFFF && maxoutput >= 3) {
        maxoutput -= 3;
        utf8[s8idx++] = 0xE0 | (code_point >> 12);
        utf8[s8idx++] = 0x80 | ((code_point >> 6) & 0x3F);
        utf8[s8idx++] = 0x80 | (code_point & 0x3F);
      } else if (code_point <= 0x10FFFF && maxoutput >= 4) {
        maxoutput -= 4;
        utf8[s8idx++] = 0xF0 | (code_point >> 18);
        utf8[s8idx++] = 0x80 | ((code_point >> 12) & 0x3F);
        utf8[s8idx++] = 0x80 | ((code_point >> 6) & 0x3F);
        utf8[s8idx++] = 0x80 | (code_point & 0x3F);
      } else {
        /* buffer underrun */
        retval = CGPT_FAILED;
        break;
      }
    }
  }
  utf8[s8idx++] = 0;
  return retval;
}

/* Convert UTF8 string to UTF16. The UTF8 string must be null-terminated.
 * Caller must prepare enough space for UTF16, including a terminating 0x0000.
 * See the following table for encoding lengths. In any case, the caller
 * just needs to prepare the byte length of UTF8 plus the terminating 0x0000.
 *
 *     Code point       UTF16       UTF8
 *   0x0000-0x007F     2 bytes     1 byte
 *   0x0080-0x07FF     2 bytes     2 bytes
 *   0x0800-0xFFFF     2 bytes     3 bytes
 *  0x10000-0x10FFFF   4 bytes     4 bytes
 *
 * This function converts UTF8 chars to a code point first. Then, convrts it
 * to UTF16 code unit(s).
 *
 * Return: CGPT_OK --- all character are converted successfully.
 *         CGPT_FAILED --- convert error, i.e. output buffer is too short.
 */
int UTF8ToUTF16(const uint8_t *utf8, uint16_t *utf16, unsigned int maxoutput)
{
  size_t s16idx, s8idx;
  uint32_t code_point = 0;
  unsigned int expected_units = 1;
  unsigned int decoded_units = 1;
  int retval = CGPT_OK;

  if (!utf8 || !utf16 || !maxoutput)
    return CGPT_FAILED;

  maxoutput--;                             /* plan for termination */

  for (s8idx = s16idx = 0;
       utf8[s8idx] && maxoutput;
       s8idx++) {
    uint8_t code_unit;
    code_unit = utf8[s8idx];

    if (expected_units != decoded_units) {
      /* Trailing bytes of multi-byte character */
      if ((code_unit & 0xC0) == 0x80) {
        code_point = (code_point << 6) | (code_unit & 0x3F);
        ++decoded_units;
      } else {
        /* Unexpected code unit. */
        retval = CGPT_FAILED;
        break;
      }
    } else {
      /* parsing a new code point. */
      decoded_units = 1;
      if (code_unit <= 0x7F) {
        code_point = code_unit;
        expected_units = 1;
      } else if (code_unit <= 0xBF) {
        /* 0x80-0xBF must NOT be the heading byte unit of a new code point. */
        retval = CGPT_FAILED;
        break;
      } else if (code_unit >= 0xC2 && code_unit <= 0xDF) {
        code_point = code_unit & 0x1F;
        expected_units = 2;
      } else if (code_unit >= 0xE0 && code_unit <= 0xEF) {
        code_point = code_unit & 0x0F;
        expected_units = 3;
      } else if (code_unit >= 0xF0 && code_unit <= 0xF4) {
        code_point = code_unit & 0x07;
        expected_units = 4;
      } else {
        /* illegal code unit: 0xC0-0xC1, 0xF5-0xFF */
        retval = CGPT_FAILED;
        break;
      }
    }

    /* If no more unit is needed, output the UTF16 unit(s). */
    if ((retval == CGPT_OK) &&
        (expected_units == decoded_units)) {
      /* Check if the encoding is the shortest possible UTF-8 sequence. */
      switch (expected_units) {
        case 2:
          if (code_point <= 0x7F) retval = CGPT_FAILED;
          break;
        case 3:
          if (code_point <= 0x7FF) retval = CGPT_FAILED;
          break;
        case 4:
          if (code_point <= 0xFFFF) retval = CGPT_FAILED;
          break;
      }
      if (retval == CGPT_FAILED) break;  /* leave immediately */

      if ((code_point <= 0xD7FF) ||
          (code_point >= 0xE000 && code_point <= 0xFFFF)) {
        utf16[s16idx++] = code_point;
        maxoutput -= 1;
      } else if (code_point >= 0x10000 && code_point <= 0x10FFFF &&
                 maxoutput >= 2) {
        utf16[s16idx++] = 0xD800 | ((code_point >> 10) - 0x0040);
        utf16[s16idx++] = 0xDC00 | (code_point & 0x03FF);
        maxoutput -= 2;
      } else {
        /* Three possibilities fall into here. Both are failure cases.
         *   a. surrogate pair (non-BMP characters; 0xD800~0xDFFF)
         *   b. invalid code point > 0x10FFFF
         *   c. buffer underrun
         */
        retval = CGPT_FAILED;
        break;
      }
    }
  }

  /* A null-terminator shows up before the UTF8 sequence ends. */
  if (expected_units != decoded_units) {
    retval = CGPT_FAILED;
  }

  utf16[s16idx++] = 0;
  return retval;
}

/* global types to compare against */
const Guid guid_chromeos_firmware = GPT_ENT_TYPE_CHROMEOS_FIRMWARE;
const Guid guid_chromeos_kernel =   GPT_ENT_TYPE_CHROMEOS_KERNEL;
const Guid guid_chromeos_rootfs =   GPT_ENT_TYPE_CHROMEOS_ROOTFS;
const Guid guid_linux_data =        GPT_ENT_TYPE_LINUX_DATA;
const Guid guid_chromeos_reserved = GPT_ENT_TYPE_CHROMEOS_RESERVED;
const Guid guid_efi =               GPT_ENT_TYPE_EFI;
const Guid guid_unused =            GPT_ENT_TYPE_UNUSED;

const static struct {
  const Guid *type;
  char *name;
  char *description;
  int mtd_type;
} supported_types[] = {
  {&guid_chromeos_firmware, "firmware", "ChromeOS firmware",
    MTD_PARTITION_TYPE_CHROMEOS_FIRMWARE},
  {&guid_chromeos_kernel, "kernel", "ChromeOS kernel",
    MTD_PARTITION_TYPE_CHROMEOS_KERNEL},
  {&guid_chromeos_rootfs, "rootfs", "ChromeOS rootfs",
    MTD_PARTITION_TYPE_CHROMEOS_ROOTFS},
  {&guid_linux_data, "data", "Linux data",
    MTD_PARTITION_TYPE_LINUX_DATA},
  {&guid_chromeos_reserved, "reserved", "ChromeOS reserved",
    MTD_PARTITION_TYPE_CHROMEOS_RESERVED},
  {&guid_efi, "efi", "EFI System Partition",
    MTD_PARTITION_TYPE_EFI},
  {&guid_unused, "unused", "Unused (nonexistent) partition",
    MTD_PARTITION_TYPE_UNUSED},
};

int LookupMtdTypeForGuid(const Guid *type) {
  int i;
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    if (!memcmp(type, supported_types[i].type, sizeof(Guid))) {
      return supported_types[i].mtd_type;
    }
  }
  return MTD_PARTITION_TYPE_OTHER;
}

const Guid *LookupGuidForMtdType(int type) {
  int i;
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    if (supported_types[i].mtd_type == type) {
      return supported_types[i].type;
    }
  }
  return NULL;
}

/* Resolves human-readable GPT type.
 * Returns CGPT_OK if found.
 * Returns CGPT_FAILED if no known type found. */
int ResolveType(const Guid *type, char *buf) {
  int i;
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    if (!memcmp(type, supported_types[i].type, sizeof(Guid))) {
      strcpy(buf, supported_types[i].description);
      return CGPT_OK;
    }
  }
  return CGPT_FAILED;
}

int SupportedType(const char *name, Guid *type) {
  int i;
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    if (!strcmp(name, supported_types[i].name)) {
      memcpy(type, supported_types[i].type, sizeof(Guid));
      return CGPT_OK;
    }
  }
  return CGPT_FAILED;
}

void PrintTypes(void) {
  int i;
  printf("The partition type may also be given as one of these aliases:\n\n");
  for (i = 0; i < ARRAY_COUNT(supported_types); ++i) {
    printf("    %-10s  %s\n", supported_types[i].name,
                          supported_types[i].description);
  }
  printf("\n");
}

GptHeader* GetGptHeader(const GptData *gpt) {
  if (gpt->valid_headers & MASK_PRIMARY)
    return (GptHeader*)gpt->primary_header;
  else if (gpt->valid_headers & MASK_SECONDARY)
    return (GptHeader*)gpt->secondary_header;
  else
    return 0;
}

uint32_t GetNumberOfEntries(const struct drive *drive) {
  if (drive->is_mtd)
    return MTD_MAX_PARTITIONS;

  GptHeader *header = GetGptHeader(&drive->gpt);
  if (!header)
    return 0;
  return header->number_of_entries;
}


GptEntry *GetEntry(GptData *gpt, int secondary, uint32_t entry_index) {
  GptHeader *header = GetGptHeader(gpt);
  uint8_t *entries;
  uint32_t stride = header->size_of_entry;
  require(stride);
  require(entry_index < header->number_of_entries);

  if (secondary == PRIMARY) {
    entries = gpt->primary_entries;
  } else if (secondary == SECONDARY) {
    entries = gpt->secondary_entries;
  } else {  /* ANY_VALID */
    require(secondary == ANY_VALID);
    if (gpt->valid_entries & MASK_PRIMARY) {
      entries = gpt->primary_entries;
    } else {
      require(gpt->valid_entries & MASK_SECONDARY);
      entries = gpt->secondary_entries;
    }
  }

  return (GptEntry*)(&entries[stride * entry_index]);
}

MtdDiskPartition* MtdGetEntry(MtdData *mtd, int secondary, uint32_t index) {
  if (index >= MTD_MAX_PARTITIONS)
    return NULL;
  return &mtd->primary.partitions[index];
}

void SetPriority(struct drive *drive, int secondary, uint32_t entry_index,
                 int priority) {
  require(priority >= 0 && priority <= CGPT_ATTRIBUTE_MAX_PRIORITY);
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    MtdSetEntryPriority(e, priority);
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    SetEntryPriority(entry, priority);
  }
}

int GetPriority(struct drive *drive, int secondary, uint32_t entry_index) {
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    return MtdGetEntryPriority(e);
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    return GetEntryPriority(entry);
  }
}

void SetTries(struct drive *drive, int secondary, uint32_t entry_index,
              int tries) {
  require(tries >= 0 && tries <= CGPT_ATTRIBUTE_MAX_TRIES);
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    MtdSetEntryTries(e, tries);
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    SetEntryTries(entry, tries);
  }
}

int GetTries(struct drive *drive, int secondary, uint32_t entry_index) {
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    return MtdGetEntryTries(e);
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    return GetEntryTries(entry);
  }
}

void SetSuccessful(struct drive *drive, int secondary, uint32_t entry_index,
                   int success) {
  require(success >= 0 && success <= CGPT_ATTRIBUTE_MAX_SUCCESSFUL);
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    MtdSetEntrySuccessful(e, success);
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    SetEntrySuccessful(entry, success);
  }
}

int GetSuccessful(struct drive *drive, int secondary, uint32_t entry_index) {
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    return MtdGetEntrySuccessful(e);
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    return GetEntrySuccessful(entry);
  }
}

void SetRaw(struct drive *drive, int secondary, uint32_t entry_index,
            uint32_t raw) {
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, entry_index);
    e->flags = raw;
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, entry_index);
    entry->attrs.fields.gpt_att = (uint16_t)raw;
  }
}

void UpdateAllEntries(struct drive *drive) {
  if (drive->is_mtd) {
    drive->mtd.modified = 1;
    drive->mtd.primary.crc32 = MtdHeaderCrc(&drive->mtd.primary);
  } else {
    RepairEntries(&drive->gpt, MASK_PRIMARY);
    RepairHeader(&drive->gpt, MASK_PRIMARY);

    drive->gpt.modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1 |
                           GPT_MODIFIED_HEADER2 | GPT_MODIFIED_ENTRIES2);
    UpdateCrc(&drive->gpt);
  }
}

int IsUnused(struct drive *drive, int secondary, uint32_t index) {
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, index);
    return MtdGetEntryType(e) == MTD_PARTITION_TYPE_UNUSED;
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, index);
    return GuidIsZero(&entry->type);
  }
}

int IsKernel(struct drive *drive, int secondary, uint32_t index) {
  if (drive->is_mtd) {
    MtdDiskPartition *e = MtdGetEntry(&drive->mtd, secondary, index);
    return MtdGetEntryType(e) == MTD_PARTITION_TYPE_CHROMEOS_KERNEL;
  } else {
    GptEntry *entry;
    entry = GetEntry(&drive->gpt, secondary, index);
    return GuidEqual(&entry->type, &guid_chromeos_kernel);
  }
}


#define TOSTRING(A) #A
const char *GptError(int errnum) {
  const char *error_string[] = {
    TOSTRING(GPT_SUCCESS),
    TOSTRING(GPT_ERROR_NO_VALID_KERNEL),
    TOSTRING(GPT_ERROR_INVALID_HEADERS),
    TOSTRING(GPT_ERROR_INVALID_ENTRIES),
    TOSTRING(GPT_ERROR_INVALID_SECTOR_SIZE),
    TOSTRING(GPT_ERROR_INVALID_SECTOR_NUMBER),
    TOSTRING(GPT_ERROR_INVALID_UPDATE_TYPE)
  };
  if (errnum < 0 || errnum >= ARRAY_COUNT(error_string))
    return "<illegal value>";
  return error_string[errnum];
}

/*  Update CRC value if necessary.  */
void UpdateCrc(GptData *gpt) {
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  if (gpt->modified & GPT_MODIFIED_ENTRIES1 &&
      memcmp(primary_header, GPT_HEADER_SIGNATURE2,
             GPT_HEADER_SIGNATURE_SIZE)) {
    primary_header->entries_crc32 =
        Crc32(gpt->primary_entries, TOTAL_ENTRIES_SIZE);
  }
  if (gpt->modified & GPT_MODIFIED_ENTRIES2) {
    secondary_header->entries_crc32 =
        Crc32(gpt->secondary_entries, TOTAL_ENTRIES_SIZE);
  }
  if (gpt->modified & GPT_MODIFIED_HEADER1) {
    primary_header->header_crc32 = 0;
    primary_header->header_crc32 = Crc32(
        (const uint8_t *)primary_header, sizeof(GptHeader));
  }
  if (gpt->modified & GPT_MODIFIED_HEADER2) {
    secondary_header->header_crc32 = 0;
    secondary_header->header_crc32 = Crc32(
        (const uint8_t *)secondary_header, sizeof(GptHeader));
  }
}
/* Two headers are NOT bitwise identical. For example, my_lba pointers to header
 * itself so that my_lba in primary and secondary is definitely different.
 * Only the following fields should be identical.
 *
 *   first_usable_lba
 *   last_usable_lba
 *   number_of_entries
 *   size_of_entry
 *   disk_uuid
 *
 * If any of above field are not matched, overwrite secondary with primary since
 * we always trust primary.
 * If any one of header is invalid, copy from another. */
int IsSynonymous(const GptHeader* a, const GptHeader* b) {
  if ((a->first_usable_lba == b->first_usable_lba) &&
      (a->last_usable_lba == b->last_usable_lba) &&
      (a->number_of_entries == b->number_of_entries) &&
      (a->size_of_entry == b->size_of_entry) &&
      (!memcmp(&a->disk_uuid, &b->disk_uuid, sizeof(Guid))))
    return 1;
  return 0;
}

/* Primary entries and secondary entries should be bitwise identical.
 * If two entries tables are valid, compare them. If not the same,
 * overwrites secondary with primary (primary always has higher priority),
 * and marks secondary as modified.
 * If only one is valid, overwrites invalid one.
 * If all are invalid, does nothing.
 * This function returns bit masks for GptData.modified field.
 * Note that CRC is NOT re-computed in this function.
 */
uint8_t RepairEntries(GptData *gpt, const uint32_t valid_entries) {
  /* If we have an alternate GPT header signature, don't overwrite
   * the secondary GPT with the primary one as that might wipe the
   * partition table. Also don't overwrite the primary one with the
   * secondary one as that will stop Windows from booting. */
  GptHeader* h = (GptHeader*)(gpt->primary_header);
  if (!memcmp(h->signature, GPT_HEADER_SIGNATURE2, GPT_HEADER_SIGNATURE_SIZE))
    return 0;

  if (valid_entries == MASK_BOTH) {
    if (memcmp(gpt->primary_entries, gpt->secondary_entries,
               TOTAL_ENTRIES_SIZE)) {
      memcpy(gpt->secondary_entries, gpt->primary_entries, TOTAL_ENTRIES_SIZE);
      return GPT_MODIFIED_ENTRIES2;
    }
  } else if (valid_entries == MASK_PRIMARY) {
    memcpy(gpt->secondary_entries, gpt->primary_entries, TOTAL_ENTRIES_SIZE);
    return GPT_MODIFIED_ENTRIES2;
  } else if (valid_entries == MASK_SECONDARY) {
    memcpy(gpt->primary_entries, gpt->secondary_entries, TOTAL_ENTRIES_SIZE);
    return GPT_MODIFIED_ENTRIES1;
  }

  return 0;
}

/* The above five fields are shared between primary and secondary headers.
 * We can recover one header from another through copying those fields. */
void CopySynonymousParts(GptHeader* target, const GptHeader* source) {
  target->first_usable_lba = source->first_usable_lba;
  target->last_usable_lba = source->last_usable_lba;
  target->number_of_entries = source->number_of_entries;
  target->size_of_entry = source->size_of_entry;
  memcpy(&target->disk_uuid, &source->disk_uuid, sizeof(Guid));
}

/* This function repairs primary and secondary headers if possible.
 * If both headers are valid (CRC32 is correct) but
 *   a) indicate inconsistent usable LBA ranges,
 *   b) inconsistent partition entry size and number,
 *   c) inconsistent disk_uuid,
 * we will use the primary header to overwrite secondary header.
 * If primary is invalid (CRC32 is wrong), then we repair it from secondary.
 * If secondary is invalid (CRC32 is wrong), then we repair it from primary.
 * This function returns the bitmasks for modified header.
 * Note that CRC value is NOT re-computed in this function. UpdateCrc() will
 * do it later.
 */
uint8_t RepairHeader(GptData *gpt, const uint32_t valid_headers) {
  GptHeader *primary_header, *secondary_header;

  primary_header = (GptHeader*)gpt->primary_header;
  secondary_header = (GptHeader*)gpt->secondary_header;

  if (valid_headers == MASK_BOTH) {
    if (!IsSynonymous(primary_header, secondary_header)) {
      CopySynonymousParts(secondary_header, primary_header);
      return GPT_MODIFIED_HEADER2;
    }
  } else if (valid_headers == MASK_PRIMARY) {
    memcpy(secondary_header, primary_header, sizeof(GptHeader));
    secondary_header->my_lba = gpt->drive_sectors - 1;  /* the last sector */
    secondary_header->alternate_lba = primary_header->my_lba;
    secondary_header->entries_lba = secondary_header->my_lba -
        GPT_ENTRIES_SECTORS;
    return GPT_MODIFIED_HEADER2;
  } else if (valid_headers == MASK_SECONDARY) {
    memcpy(primary_header, secondary_header, sizeof(GptHeader));
    primary_header->my_lba = GPT_PMBR_SECTOR;  /* the second sector on drive */
    primary_header->alternate_lba = secondary_header->my_lba;
    primary_header->entries_lba = primary_header->my_lba + GPT_HEADER_SECTOR;
    return GPT_MODIFIED_HEADER1;
  }

  return 0;
}

int CgptGetNumNonEmptyPartitions(CgptShowParams *params) {
  struct drive drive;
  int gpt_retval;
  int retval;

  if (params == NULL)
    return CGPT_FAILED;

  if (CGPT_OK != DriveOpen(params->drive_name, &drive, O_RDONLY))
    return CGPT_FAILED;

  if (GPT_SUCCESS != (gpt_retval = GptSanityCheck(&drive.gpt))) {
    Error("GptSanityCheck() returned %d: %s\n",
          gpt_retval, GptError(gpt_retval));
    retval = CGPT_FAILED;
    goto done;
  }

  params->num_partitions = 0;
  int numEntries = GetNumberOfEntries(&drive);
  int i;
  for(i = 0; i < numEntries; i++) {
      GptEntry *entry = GetEntry(&drive.gpt, ANY_VALID, i);
      if (GuidIsZero(&entry->type))
        continue;

      params->num_partitions++;
  }

  retval = CGPT_OK;

done:
  DriveClose(&drive, 0);
  return retval;
}

int GuidEqual(const Guid *guid1, const Guid *guid2) {
  return (0 == memcmp(guid1, guid2, sizeof(Guid)));
}

int GuidIsZero(const Guid *gp) {
  return GuidEqual(gp, &guid_unused);
}

void PMBRToStr(struct pmbr *pmbr, char *str, unsigned int buflen) {
  char buf[GUID_STRLEN];
  if (GuidIsZero(&pmbr->boot_guid)) {
    require(snprintf(str, buflen, "PMBR") < buflen);
  } else {
    GuidToStr(&pmbr->boot_guid, buf, sizeof(buf));
    require(snprintf(str, buflen, "PMBR (Boot GUID: %s)", buf) < buflen);
  }
}

/* Optional */
int __GenerateGuid(Guid *newguid) { return CGPT_FAILED; };
int GenerateGuid(Guid *newguid) __attribute__((weak, alias("__GenerateGuid")));
