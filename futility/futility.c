/*
 * Copyright 2013 The Chromium OS Authors. All rights reserved.
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
most of the distinct verified boot tools formerly produced by the\n\
vboot_reference package.\n\
\n\
When symlinked under the name of one of those previous tools, should\n\
fully implement the original behavior. It can also be invoked directly\n\
as " MYNAME ", followed by the original name as the first argument.\n\
\n\
In either case it will append some usage information to " LOGFILE "\n\
(iff that file exists), to help improve coverage and correctness.\n\
\n";

static int do_help(int argc, char *argv[])
{
  struct futil_cmd_t **cmd;
  int i;

  fputs(usage, stdout);

  printf("The following commands are built-in:\n\n");

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
DECLARE_FUTIL_COMMAND(help, do_help,
		      "Show a bit of help (you're looking at it)");

/*
 * These are built-in functions that we'd like to abandon completely someday.
 * TODO: If no one complains, get rid of them.
 */
static char *dep_cmds[] = {
  "dev_sign_file",
};

static const char * const dep_usage= "\n\
The program \"%s\" is deprecated and may go away soon.\n\
\n\
If you feel this is in error, please open a bug at\n\
\n\
  http://dev.chromium.org/for-testers/bug-reporting-guidelines\n\
\n\
In the meantime, you may continue to use the program by invoking it as\n\
\n\
  " MYNAME " %s [...]\n\
\n";

static void deprecated(const char *depname)
{
  fprintf(stderr, dep_usage, depname, depname);
  exit(1);
}

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

int main(int argc, char *argv[], char *envp[])
{
  char *fullname, *progname;
  char truename[PATH_MAX];
  char buf[80];
  pid_t myproc;
  ssize_t r;
  struct futil_cmd_t **cmd;
  int i;
  int via_symlink = 0;

  log_args(argc, argv);

  /* How were we invoked? */
  fullname = strdup(argv[0]);
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

    /* So now what function do we want to invoke? */
    progname = strrchr(argv[0], '/');
    if (progname)
      progname++;
    else
      progname = argv[0];
  } else {	      /* Invoked by symlink */
    via_symlink = 1;
    /* Block any deprecated functions. */
    for (i = 0; i < ARRAY_SIZE(dep_cmds); i++)
      if (0 == strcmp(dep_cmds[i], progname))
	deprecated(progname);
  }

  /* See if it's asking for something we know how to do ourselves */
  for (cmd = futil_cmds; *cmd; cmd++)
    if (0 == strcmp((*cmd)->name, progname))
      return (*cmd)->handler(argc, argv);

  /* Nope */
  if (!via_symlink) {
    do_help(0, 0);
    exit(1);
  }

  /* Complain about bogus symlink */

  myproc = getpid();
  snprintf(buf, sizeof(buf), "/proc/%d/exe", myproc);
  r = readlink(buf, truename, PATH_MAX - 1);
  if (r < 0) {
    fprintf(stderr, "%s is lost: %s => %s: %s\n", MYNAME, argv[0],
            buf, strerror(errno));
    exit(1);
  }
  truename[r] = '\0';

  fprintf(stderr, "\n\
The program\n\n  %s\n\nis a symlink to\n\n  %s\n\
\n\
However, " MYNAME " doesn't know how to implement that function.\n\
\n\
This is probably an error in your installation. If the problem persists\n\
after a fresh checkout/build/install, please open a bug at\n\
\n\
  http://dev.chromium.org/for-testers/bug-reporting-guidelines\n\
\n", fullname, truename);
  return 1;
}
