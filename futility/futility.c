/*
 * Copyright 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
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

/* Default is to support everything we can */
enum vboot_version vboot_version = VBOOT_VERSION_ALL;

static const char *const usage = "\n"
"Usage: " MYNAME " [options] COMMAND [args...]\n"
"\n"
"This is the unified firmware utility, which will eventually replace\n"
"most of the distinct verified boot tools formerly produced by the\n"
"vboot_reference package.\n"
"\n"
"When symlinked under the name of one of those previous tools, it should\n"
"fully implement the original behavior. It can also be invoked directly\n"
"as " MYNAME ", followed by the original name as the first argument.\n"
"\n";

static const char *const options =
"Global options:\n"
"\n"
"  --vb1        Use only vboot v1.0 binary formats\n"
"  --vb21       Use only vboot v2.1 binary formats\n"
"\n";

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
		if (vboot_version & (*cmd)->version)
			printf("  %-20s %s\n",
			       (*cmd)->name, (*cmd)->shorthelp);
}

static int do_help(int argc, char *argv[])
{
	const struct futil_cmd_t *cmd;
	const char *vstr;

	if (argc >= 2) {
		cmd = find_command(argv[1]);
		if (cmd) {
			printf("\n%s - %s\n", argv[1], cmd->shorthelp);
			if (cmd->longhelp)
				cmd->longhelp(argv[1]);
			return 0;
		}
	}

	fputs(usage, stdout);

	if (vboot_version == VBOOT_VERSION_ALL)
		fputs(options, stdout);

	switch (vboot_version) {
	case VBOOT_VERSION_1_0:
		vstr = "version 1.0 ";
		break;
	case VBOOT_VERSION_2_1:
		vstr = "version 2.1 ";
		break;
	case VBOOT_VERSION_ALL:
		vstr = "";
		break;
	}
	printf("The following %scommands are built-in:\n\n", vstr);
	list_commands();
	printf("\nUse \"" MYNAME " help COMMAND\" for more information.\n\n");

	return 0;
}

DECLARE_FUTIL_COMMAND(help, do_help, VBOOT_VERSION_ALL,
		      "Show a bit of help (you're looking at it)",
		      NULL);

static int do_version(int argc, char *argv[])
{
	printf("%s\n", futility_version);
	return 0;
}

DECLARE_FUTIL_COMMAND(version, do_version, VBOOT_VERSION_ALL,
		      "Show the futility source revision and build date",
		      NULL);

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
	int i, errorcnt = 0;
	int vb_ver = VBOOT_VERSION_ALL;
	struct option long_opts[] = {
		{"vb1" , 0,  &vb_ver,  VBOOT_VERSION_1_0},
		{"vb21", 0,  &vb_ver,  VBOOT_VERSION_2_1},
		{ 0, 0, 0, 0},
	};

	log_args(argc, argv);

	/* How were we invoked? */
	progname = simple_basename(argv[0]);

	/* See if the program name is a command we recognize */
	cmd = find_command(progname);
	if (cmd)
		/* Yep, just do that */
		return run_command(cmd, argc, argv);

	/* Parse the global options, stopping at the first non-option. */
	opterr = 0;				/* quiet, you. */
	while ((i = getopt_long(argc, argv, "+:", long_opts, NULL)) != -1) {
		switch (i) {
		case '?':
			if (optopt)
				fprintf(stderr, "Unrecognized option: -%c\n",
					optopt);
			else
				fprintf(stderr, "Unrecognized option: %s\n",
					argv[optind - 1]);
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "Missing argument to -%c\n", optopt);
			errorcnt++;
			break;
		case 0:				/* handled option */
			break;
		default:
			Debug("i=%d\n", i);
			DIE;
		}
	}
	vboot_version = vb_ver;

	/* Reset the getopt state so commands can parse their own options. */
	argc -= optind;
	argv += optind;
	optind = 0;

	/* We require a command name. */
	if (errorcnt || argc < 1) {
		do_help(0, 0);
		return 1;
	}

	/* For reasons I've forgotten, treat /blah/blah/CMD the same as CMD */
	progname = simple_basename(argv[0]);

	/* Do we recognize the command? */
	cmd = find_command(progname);
	if (cmd)
		return run_command(cmd, argc, argv);

	/* Nope. We've no clue what we're being asked to do. */
	do_help(0, 0);
	return 1;
}
