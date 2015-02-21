/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This utility wraps around "cgpt" execution to work with NAND. If the target
 * device is an MTD device, this utility will read the GPT structures from
 * FMAP, invokes "cgpt" on that, and writes the result back to NOR flash. */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/major.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cgpt.h"
#include "cgpt_nor.h"
#include "cryptolib.h"

// Check if cmdline |argv| has "-D". "-D" signifies that GPT structs are stored
// off device, and hence we should not wrap around cgpt.
static bool has_dash_D(int argc, const char *const argv[]) {
  int i;
  // We go from 2, because the second arg is a cgpt command such as "create".
  for (i = 2; i < argc; ++i) {
    if (strcmp("-D", argv[i]) == 0) {
      return true;
    }
  }
  return false;
}

// Check if |device_path| is an MTD device based on its major number being 90.
static bool is_mtd(const char *device_path) {
  struct stat stat;
  if (lstat(device_path, &stat) != 0) {
    return false;
  }

  if (major(stat.st_rdev) != MTD_CHAR_MAJOR) {
    return false;
  }

  return true;
}

// Return the element in |argv| that is an MTD device.
static const char *find_mtd_device(int argc, const char *const argv[]) {
  int i;
  for (i = 2; i < argc; ++i) {
    if (is_mtd(argv[i])) {
      return argv[i];
    }
  }
  return NULL;
}

static int wrap_cgpt(int argc,
                     const char *const argv[],
                     const char *mtd_device) {
  uint8_t *original_hash = NULL;
  uint8_t *modified_hash = NULL;
  int ret = 0;

  // Create a temp dir to work in.
  ret++;
  char temp_dir[] = "/tmp/cgpt_wrapper.XXXXXX";
  if (ReadNorFlash(temp_dir) != 0) {
    return ret;
  }
  char rw_gpt_path[PATH_MAX];
  if (snprintf(rw_gpt_path, sizeof(rw_gpt_path), "%s/rw_gpt", temp_dir) < 0) {
    goto cleanup;
  }
  original_hash = DigestFile(rw_gpt_path, SHA1_DIGEST_ALGORITHM);

  // Obtain the MTD size.
  ret++;
  uint64_t drive_size = 0;
  if (GetMtdSize(mtd_device, &drive_size) != 0) {
    Error("Cannot get the size of %s.\n", mtd_device);
    goto cleanup;
  }

  // Launch cgpt on "rw_gpt" with -D size.
  ret++;
  const char** my_argv = calloc(argc + 2 + 1, sizeof(char *));
  if (my_argv == NULL) {
    errno = ENOMEM;
    goto cleanup;
  }
  memcpy(my_argv, argv, sizeof(char *) * argc);
  char *real_cgpt;
  if (asprintf(&real_cgpt, "%s.bin", argv[0]) == -1) {
    free(my_argv);
    goto cleanup;
  }
  my_argv[0] = real_cgpt;

  int i;
  for (i = 2; i < argc; ++i) {
    if (strcmp(my_argv[i], mtd_device) == 0) {
      my_argv[i] = rw_gpt_path;
    }
  }
  my_argv[argc] = "-D";
  char size[32];
  snprintf(size, sizeof(size), "%" PRIu64, drive_size);
  my_argv[argc + 1] = size;
  i = ForkExecV(NULL, my_argv);
  free(real_cgpt);
  free(my_argv);
  if (i != 0) {
    Error("Cannot exec cgpt to modify rw_gpt.\n");
    goto cleanup;
  }

  // Write back "rw_gpt" to NOR flash in two chunks.
  ret++;
  modified_hash = DigestFile(rw_gpt_path, SHA1_DIGEST_ALGORITHM);
  if (original_hash != NULL && modified_hash != NULL) {
    if (memcmp(original_hash, modified_hash, SHA1_DIGEST_SIZE) != 0) {
      ret = WriteNorFlash(temp_dir);
    } else {
      ret = 0;
    }
  }

cleanup:
  free(original_hash);
  free(modified_hash);
  RemoveDir(temp_dir);
  return ret;
}

int main(int argc, const char *argv[]) {
  char resolved_cgpt[PATH_MAX];
  pid_t pid = getpid();
  char exe_link[40];

  if (argc < 1) {
    return -1;
  }

  snprintf(exe_link, sizeof(exe_link), "/proc/%d/exe", pid);
  memset(resolved_cgpt, 0, sizeof(resolved_cgpt));
  if (readlink(exe_link, resolved_cgpt, sizeof(resolved_cgpt) - 1) == -1) {
    perror("readlink");
    return -1;
  }

  argv[0] = resolved_cgpt;

  if (argc > 2 && !has_dash_D(argc, argv)) {
    const char *mtd_device = find_mtd_device(argc, argv);
    if (mtd_device) {
      return wrap_cgpt(argc, argv, mtd_device);
    }
  }

  // Forward to cgpt as-is. Real cgpt has been renamed cgpt.bin.
  char *real_cgpt;
  if (asprintf(&real_cgpt, "%s.bin", argv[0]) == -1) {
    return -1;
  }
  argv[0] = real_cgpt;
  if (execv(argv[0], (char * const *)argv) == -1) {
    err(-2, "execv(%s) failed", real_cgpt);
  }
  return -2;
}
