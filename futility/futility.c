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


/******************************************************************************/
/* Logging stuff */

/* File to use for logging, if present */
#define LOGFILE "/tmp/futility.log"

/* Normally logging will only happen if the logfile already exists. Uncomment
 * this to force log file creation (and thus logging) always. */

/* #define FORCE_LOGGING_ON */

static int log_fd = -1;

/* Write the string and a newline. Silently give up on errors */
static void log_str(char *prefix, char *str)
{
	int len, done, n;

	if (log_fd < 0)
		return;

	if (!str)
		str = "(NULL)";

	if (prefix && *prefix) {
		len = strlen(prefix);
		for (done = 0; done < len; done += n) {
			n = write(log_fd, prefix + done, len - done);
			if (n < 0)
				return;
		}
	}

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
	log_fd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0666);
#else
	log_fd = open(LOGFILE, O_WRONLY | O_APPEND);
#endif
	if (log_fd < 0) {

		if (errno != EACCES)
			return;

		/* Permission problems should improve shortly ... */
		sleep(1);
		log_fd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (log_fd < 0)	/* Nope, they didn't */
			return;
	}

	/* Let anyone have a turn */
	fchmod(log_fd, 0666);

	/* But only one at a time */
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_END;

	ret = fcntl(log_fd, F_SETLKW, &lock);	/* this blocks */
	if (ret < 0)
		log_close();
}

static void log_args(int argc, char *argv[])
{
	int i;
	ssize_t r;
	pid_t parent;
	char buf[80];
	FILE *fp;
	char caller_buf[PATH_MAX];

	log_open();

	/* delimiter */
	log_str(NULL, "##### LOG #####");

	/* Can we tell who called us? */
	parent = getppid();
	snprintf(buf, sizeof(buf), "/proc/%d/exe", parent);
	r = readlink(buf, caller_buf, sizeof(caller_buf) - 1);
	if (r >= 0) {
		caller_buf[r] = '\0';
		log_str("CALLER:", caller_buf);
	}

	/* From where? */
	snprintf(buf, sizeof(buf), "/proc/%d/cwd", parent);
	r = readlink(buf, caller_buf, sizeof(caller_buf) - 1);
	if (r >= 0) {
		caller_buf[r] = '\0';
		log_str("DIR:", caller_buf);
	}

	/* And maybe the args? */
	snprintf(buf, sizeof(buf), "/proc/%d/cmdline", parent);
	fp = fopen(buf, "r");
	if (fp) {
		memset(caller_buf, 0, sizeof(caller_buf));
		r = fread(caller_buf, 1, sizeof(caller_buf) - 1, fp);
		if (r > 0) {
			char *s = caller_buf;
			for (i = 0; i < r && *s; ) {
				log_str("CMDLINE:", s);
				while (i < r && *s)
					i++, s++;
				i++, s++;
			}
		}
		fclose(fp);
	}

	/* Now log the stuff about ourselves */
	for (i = 0; i < argc; i++)
		log_str(NULL, argv[i]);

	log_close();
}

/******************************************************************************/

static const char *const usage = "\n"
"Usage: " MYNAME " COMMAND [args...]\n"
"\n"
"This is the unified firmware utility, which will eventually replace\n"
"most of the distinct verified boot tools formerly produced by the\n"
"vboot_reference package.\n"
"\n"
"When symlinked under the name of one of those previous tools, it should\n"
"fully implement the original behavior. It can also be invoked directly\n"
"as " MYNAME ", followed by the original name as the first argument.\n"
"\n";

static void print_help(const char *cmd)
{
	puts(usage);
}

static const struct futil_cmd_t *find_command(const char *name)
{
	const struct futil_cmd_t *const *cmd;

	for (cmd = futil_cmds; *cmd; cmd++)
		if (0 == strcmp((*cmd)->name, name))
			return *cmd;

	return NULL;
}

static void list_commands(void)
{
	const struct futil_cmd_t *const *cmd;

	for (cmd = futil_cmds; *cmd; cmd++)
		printf("  %-20s %s\n", (*cmd)->name, (*cmd)->shorthelp);
}

static int do_help(int argc, char *argv[])
{
	const struct futil_cmd_t *cmd;

	if (argc >= 2) {
		cmd = find_command(argv[1]);
		if (cmd) {
			printf("\n%s - %s\n", argv[1], cmd->shorthelp);
			cmd->longhelp(argv[1]);
			return 0;
		}
	}

	fputs(usage, stdout);

	printf("The following commands are built-in:\n\n");
	list_commands();
	printf("\nUse \"" MYNAME " help COMMAND\" for more information.\n\n");

	return 0;
}

DECLARE_FUTIL_COMMAND(help, do_help,
		      "Show a bit of help (you're looking at it)",
		      print_help);

static int do_version(int argc, char *argv[])
{
	printf("%s\n", futility_version);
	return 0;
}

DECLARE_FUTIL_COMMAND(version, do_version,
		      "Show the futility source revision and build date",
		      NULL);

/*
 * These are built-in functions that we'd like to abandon completely someday.
 * TODO: If no one complains, get rid of them.
 */
static const char *const dep_cmds[] = {
	"dev_sign_file",
};

static const char *const dep_usage = "\n"
"The program \"%s\" is deprecated and may go away soon.\n"
"\n"
"If you feel this is in error, please open a bug at\n"
"\n"
"  http://dev.chromium.org/for-testers/bug-reporting-guidelines\n"
"\n"
"In the meantime, you may continue to use the program by invoking it as\n"
"\n" MYNAME " %s [...]\n"
"\n";

static int deprecated(const char *depname)
{
	fprintf(stderr, dep_usage, depname, depname);
	return 1;
}

int run_command(const struct futil_cmd_t *cmd, int argc, char *argv[])
{
	/* Handle the "CMD --help" case ourselves */
	if (2 == argc && 0 == strcmp(argv[1], "--help")) {
		char *fake_argv[] = {"help",
				     (char *)cmd->name,
				     NULL};
		return do_help(2, fake_argv);
	}

	return cmd->handler(argc, argv);
}

static char *simple_basename(char *str)
{
	char *s = strrchr(str, '/');
	if (s)
		s++;
	else
		s = str;
	return s;
}

/* Here we go */
int main(int argc, char *argv[], char *envp[])
{
	char *progname;
	const struct futil_cmd_t *cmd;
	int i;

	log_args(argc, argv);

	/* How were we invoked? */
	progname = simple_basename(argv[0]);

	/* See if the program name is a command we recognize */
	cmd = find_command(progname);
	if (cmd) {
		/* Block any deprecated functions invoked directly. */
		for (i = 0; i < ARRAY_SIZE(dep_cmds); i++)
			if (0 == strcmp(dep_cmds[i], progname))
				return deprecated(progname);

		return run_command(cmd, argc, argv);
	}

	/* The program name means nothing, so we require an argument. */
	if (argc < 2) {
		do_help(0, 0);
		return 1;
	}

	/* The first arg should be a command we recognize */
	argc--;
	argv++;

	/* For reasons I've forgotten, treat /blah/blah/CMD the same as CMD */
	progname = simple_basename(argv[0]);
	/* Oh, and treat "--foo" the same as "foo" */
	while (*progname == '-')
		progname++;

	/* Do we recognize the command? */
	cmd = find_command(progname);
	if (cmd)
		return run_command(cmd, argc, argv);

	/* Nope. We've no clue what we're being asked to do. */
	do_help(0, 0);
	return 1;
}
