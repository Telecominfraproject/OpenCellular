/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "futility.h"

#define MYNAME "futility"
#define MYNAME_S MYNAME "_s"
#ifdef OLDDIR
#define XSTR(A) STR(A)
#define STR(A) #A
#define SUBDIR XSTR(OLDDIR)
#else
#define SUBDIR "old_bins"
#endif

/* File to use for logging, if present */
#define LOGFILE "/tmp/futility.log"

/* Normally logging will only happen if the logfile already exists. Uncomment
 * this to force log file creation (and thus logging) always. */
/* #define FORCE_LOGGING_ON */

/******************************************************************************/

static const char * const usage= "\n\
Usage: " MYNAME " PROGRAM|COMMAND [args...]\n\
\n\
This is the unified firmware utility, which will eventually replace\n\
all the distinct userspace tools formerly produced by the\n\
vboot_reference package.\n\
\n\
When symlinked under the name of one of those previous tools, it can\n\
do one of two things: either it will fully implement the original\n\
behavior, or (until that functionality is complete) it will just exec\n\
the original binary.\n\
\n\
In either case it can append some usage information to " LOGFILE "\n\
to help improve coverage and correctness.\n\
\n\
If you invoke it directly instead of via a symlink, it requires one\n\
argument, which is the name of the old binary to exec. That binary\n\
must be located in a directory named \"" SUBDIR "\" underneath\n\
the " MYNAME " executable.\n\
\n";

static int do_help(int argc, char *argv[])
{
  struct futil_cmd_t **cmd;
  int i;

  fputs(usage, stdout);

  printf("The following commands are built-in:\n");

  for (cmd = futil_cmds; *cmd; cmd++)
    printf("  %-20s %s\n", (*cmd)->name, (*cmd)->shorthelp);
  printf("\n");

  if (argc) {
    printf("FYI, you added these args that I'm ignoring:\n");
    for (i = 0; i < argc; i++)
      printf("argv[%d] = %s\n", i, argv[i]);
  }

  return 0;
}
DECLARE_FUTIL_COMMAND(help, do_help, "show a bit of help");


/******************************************************************************/
/* Logging stuff */

static int log_fd = -1;

/* Write the string and a newline. Silently give up on errors */
static void log_str(char *str)
{
  int len, done, n;

  if (log_fd < 0)
    return;

  if (!str)
    str = "(NULL)";

  len = strlen(str);
  if (len == 0) {
    str = "(EMPTY)";
    len = strlen(str);
  }

  for (done = 0; done < len; done += n) {
    n = write(log_fd, str + done, len - done);
    if (n < 0)
      return;
  }

  if (write(log_fd, "\n", 1) < 0)
    return;
}

static void log_close(void)
{
  struct flock lock;

  if (log_fd >= 0) {
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    if (fcntl(log_fd, F_SETLKW, &lock))
      perror("Unable to unlock log file");

    close(log_fd);
    log_fd = -1;
  }
}

static void log_open(void)
{
  struct flock lock;
  int ret;

#ifdef FORCE_LOGGING_ON
  log_fd = open(LOGFILE, O_WRONLY|O_APPEND|O_CREAT, 0666);
#else
  log_fd = open(LOGFILE, O_WRONLY|O_APPEND);
#endif
  if (log_fd < 0) {

    if (errno != EACCES)
      return;

    /* Permission problems should improve shortly ... */
    sleep(1);
    log_fd = open(LOGFILE, O_WRONLY|O_APPEND|O_CREAT, 0666);
    if (log_fd < 0)                     /* Nope, they didn't */
      return;
  }

  /* Let anyone have a turn */
  fchmod(log_fd, 0666);

  /* But only one at a time */
  memset(&lock, 0, sizeof(lock));
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_END;

  ret = fcntl(log_fd, F_SETLKW, &lock); /* this blocks */
  if (ret < 0)
    log_close();
}

#define CALLER_PREFIX "CALLER:"
static void log_args(int argc, char *argv[])
{
  int i;
  ssize_t r;
  pid_t parent;
  char buf[80];
  char str_caller[PATH_MAX + sizeof(CALLER_PREFIX)] = CALLER_PREFIX;
  char *truename = str_caller + sizeof(CALLER_PREFIX) - 1;
  /* Note: truename starts on the \0 from CALLER_PREFIX, so we can write
   * PATH_MAX chars into truename and still append a \0 at the end. */

  log_open();

  /* delimiter */
  log_str("##### HEY #####");

  /* Can we tell who called us? */
  parent = getppid();
  snprintf(buf, sizeof(buf), "/proc/%d/exe", parent);
  r = readlink(buf, truename, PATH_MAX);
  if (r >= 0) {
    truename[r] = '\0';
    log_str(str_caller);
  }

  /* Now log the stuff about ourselves */
  for (i = 0; i < argc; i++)
    log_str(argv[i]);

  log_close();
}


/******************************************************************************/
/* Here we go */

#ifdef COVERAGE
void __gcov_flush(void);
#endif

int main(int argc, char *argv[], char *envp[])
{
  char *progname;
  char truename[PATH_MAX];
  char oldname[PATH_MAX];
  char buf[80];
  pid_t myproc;
  ssize_t r;
  char *s;
  struct futil_cmd_t **cmd;

  log_args(argc, argv);

  /* How were we invoked? */
  progname = strrchr(argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  /* Invoked directly by name */
  if (0 == strcmp(progname, MYNAME) || 0 == strcmp(progname, MYNAME_S)) {
    if (argc < 2) {                     /* must have an argument */
      do_help(0, 0);
      exit(1);
    }

    /* We can just pass the rest along, then */
    argc--;
    argv++;

    /* So now what name do we want to invoke? */
    progname = strrchr(argv[0], '/');
    if (progname)
      progname++;
    else
      progname = argv[0];
  }

  /* See if it's asking for something we know how to do ourselves */
  for (cmd = futil_cmds; *cmd; cmd++)
    if (0 == strcmp((*cmd)->name, progname))
      return (*cmd)->handler(argc, argv);

  /* Nope, it must be wrapped */

  /* The old binaries live under the true executable. Find out where that is. */
  myproc = getpid();
  snprintf(buf, sizeof(buf), "/proc/%d/exe", myproc);
  r = readlink(buf, truename, PATH_MAX - 1);
  if (r < 0) {
    fprintf(stderr, "%s is lost: %s => %s: %s\n", MYNAME, argv[0],
            buf, strerror(errno));
    exit(1);
  } else if (r == PATH_MAX - 1) {
    /* Yes, it might _just_ fit, but we'll count that as wrong anyway. We can't
     * determine the right size using the example in the readlink manpage,
     * because the /proc symlink returns an st_size of 0. */
    fprintf(stderr, "%s is too long: %s => %s\n", MYNAME, argv[0], buf);
    exit(1);
  }

  truename[r] = '\0';
  s = strrchr(truename, '/');           /* Find the true directory */
  if (s) {
    *s = '\0';
  } else {                              /* I don't think this can happen */
    fprintf(stderr, "%s says %s doesn't make sense\n", MYNAME, truename);
    exit(1);
  }
  /* If the old binary path doesn't fit, just give up. */
  r = snprintf(oldname, PATH_MAX, "%s/%s/%s", truename, SUBDIR, progname);
  if (r >= PATH_MAX) {
    fprintf(stderr, "%s/%s/%s is too long\n", truename, SUBDIR, progname);
    exit(1);
  }

  fflush(0);
#ifdef COVERAGE
  /* Write gcov data prior to exec. */
  __gcov_flush();
#endif
  execve(oldname, argv, envp);

  fprintf(stderr, "%s failed to exec %s: %s\n", MYNAME,
          oldname, strerror(errno));
  return 1;
}
