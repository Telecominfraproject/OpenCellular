/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <inttypes.h>
#include <linux/major.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cgpt.h"
#include "cgpt_nor.h"

static const char FLASHROM_PATH[] = "/usr/sbin/flashrom";

// Obtain the MTD size from its sysfs node.
int GetMtdSize(const char *mtd_device, uint64_t *size) {
  mtd_device = strrchr(mtd_device, '/');
  if (mtd_device == NULL) {
    errno = EINVAL;
    return 1;
  }
  char *sysfs_name;
  if (asprintf(&sysfs_name, "/sys/class/mtd%s/size", mtd_device) == -1) {
    return 1;
  }
  FILE *fp = fopen(sysfs_name, "r");
  free(sysfs_name);
  if (fp == NULL) {
    return 1;
  }
  int ret = (fscanf(fp, "%" PRIu64 "\n", size) != 1);
  fclose(fp);
  return ret;
}

int ForkExecV(const char *cwd, const char *const argv[]) {
  pid_t pid = fork();
  if (pid == -1) {
    return -1;
  }
  int status = -1;
  if (pid == 0) {
    if (cwd && chdir(cwd) != 0) {
      return -1;
    }
    execv(argv[0], (char *const *)argv);
    // If this is reached, execv fails.
    err(-1, "Cannot exec %s in %s.", argv[0], cwd);
  } else {
    if (waitpid(pid, &status, 0) != -1 && WIFEXITED(status))
      return WEXITSTATUS(status);
  }
  return status;
}

int ForkExecL(const char *cwd, const char *cmd, ...) {
  int argc;
  va_list ap;
  va_start(ap, cmd);
  for (argc = 1; va_arg(ap, char *) != NULL; ++argc);
  va_end(ap);

  va_start(ap, cmd);
  const char **argv = calloc(argc + 1, sizeof(char *));
  if (argv == NULL) {
    errno = ENOMEM;
    return -1;
  }
  argv[0] = cmd;
  int i;
  for (i = 1; i < argc; ++i) {
    argv[i] = va_arg(ap, char *);
  }
  va_end(ap);

  int ret = ForkExecV(cwd, argv);
  free(argv);
  return ret;
}

static int read_write(int source_fd,
                      uint64_t size,
                      const char *src_name,
                      int idx) {
  int ret = 1;
  const int bufsize = 4096;
  char *buf = malloc(bufsize);
  if (buf == NULL) {
    goto clean_exit;
  }

  ret++;
  char *dest;
  if (asprintf(&dest, "%s_%d", src_name, idx) == -1) {
    goto free_buf;
  }

  ret++;
  int dest_fd = open(dest, O_WRONLY | O_CLOEXEC | O_CREAT, 0600);
  if (dest_fd < 0) {
    goto free_dest;
  }

  ret++;
  uint64_t copied = 0;
  ssize_t nr_read;
  ssize_t nr_write;
  while (copied < size) {
    size_t to_read = size - copied;
    if (to_read > bufsize) {
      to_read = bufsize;
    }
    nr_read = read(source_fd, buf, to_read);
    if (nr_read < 0) {
      goto close_dest_fd;
    }
    nr_write = 0;
    while (nr_write < nr_read) {
      ssize_t s = write(dest_fd, buf + nr_write, nr_read - nr_write);
      if (s < 0) {
        goto close_dest_fd;
      }
      nr_write += s;
    }
    copied += nr_read;
  }

  ret = 0;

close_dest_fd:
  close(dest_fd);
free_dest:
  free(dest);
free_buf:
  free(buf);
clean_exit:
  return ret;
}

static int split_gpt(const char *dir_name, const char *file_name) {
  int ret = 1;
  char *source;
  if (asprintf(&source, "%s/%s", dir_name, file_name) == -1) {
    goto clean_exit;
  }

  ret++;
  int fd = open(source, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    goto free_source;
  }

  ret++;
  struct stat stat;
  if (fstat(fd, &stat) != 0 || (stat.st_size & 1) != 0) {
    goto close_fd;
  }
  uint64_t half_size = stat.st_size / 2;

  ret++;
  if (read_write(fd, half_size, source, 1) != 0 ||
      read_write(fd, half_size, source, 2) != 0) {
    goto close_fd;
  }

  ret = 0;
close_fd:
  close(fd);
free_source:
  free(source);
clean_exit:
  return ret;
}

static int remove_file_or_dir(const char *fpath, const struct stat *sb,
                              int typeflag, struct FTW *ftwbuf) {
  return remove(fpath);
}

int RemoveDir(const char *dir) {
  return nftw(dir, remove_file_or_dir, 20, FTW_DEPTH | FTW_PHYS);
}

// Read RW_GPT from NOR flash to "rw_gpt" in a temp dir |temp_dir_template|.
// |temp_dir_template| is passed to mkdtemp() so it must satisfy all
// requirements by mkdtemp.
int ReadNorFlash(char *temp_dir_template) {
  int ret = 0;

  // Create a temp dir to work in.
  ret++;
  if (mkdtemp(temp_dir_template) == NULL) {
    Error("Cannot create a temporary directory.\n");
    return ret;
  }

  // Read RW_GPT section from NOR flash to "rw_gpt".
  ret++;
  int fd_flags = fcntl(1, F_GETFD);
  // Close stdout on exec so that flashrom does not muck up cgpt's output.
  fcntl(1, F_SETFD, FD_CLOEXEC);
  if (ForkExecL(temp_dir_template, FLASHROM_PATH, "-i", "RW_GPT:rw_gpt", "-r",
                NULL) != 0) {
    Error("Cannot exec flashrom to read from RW_GPT section.\n");
    RemoveDir(temp_dir_template);
  } else {
    ret = 0;
  }

  fcntl(1, F_SETFD, fd_flags);
  return ret;
}

// Write "rw_gpt" back to NOR flash. We write the file in two parts for safety.
int WriteNorFlash(const char *dir) {
  int ret = 0;
  ret++;
  if (split_gpt(dir, "rw_gpt") != 0) {
    Error("Cannot split rw_gpt in two.\n");
    return ret;
  }
  ret++;
  int nr_fails = 0;
  int fd_flags = fcntl(1, F_GETFD);
  // Close stdout on exec so that flashrom does not muck up cgpt's output.
  fcntl(1, F_SETFD, FD_CLOEXEC);
  if (ForkExecL(dir, FLASHROM_PATH, "-i", "RW_GPT_PRIMARY:rw_gpt_1",
                "-w", "--fast-verify", NULL) != 0) {
    Warning("Cannot write the 1st half of rw_gpt back with flashrom.\n");
    nr_fails++;
  }
  if (ForkExecL(dir, FLASHROM_PATH, "-i", "RW_GPT_SECONDARY:rw_gpt_2",
                "-w", "--fast-verify", NULL) != 0) {
    Warning("Cannot write the 2nd half of rw_gpt back with flashrom.\n");
    nr_fails++;
  }
  fcntl(1, F_SETFD, fd_flags);
  switch (nr_fails) {
    case 0: ret = 0; break;
    case 1: Warning("It might still be okay.\n"); break;
    case 2: Error("Cannot write both parts back with flashrom.\n"); break;
  }
  return ret;
}
