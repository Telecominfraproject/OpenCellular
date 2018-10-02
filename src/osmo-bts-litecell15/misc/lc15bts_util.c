/* lc15bts-util - access to hardware related parameters */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_misc.c
 *     (C) 2012-2013 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/msgb.h>

#include "lc15bts_par.h"

void *tall_util_ctx;

enum act {
	ACT_GET,
	ACT_SET,
};

static enum act action;
static char *write_arg;
static int void_warranty;

static void print_help()
{
	const struct value_string *par = lc15bts_par_names;

	printf("lc15bts-util [--void-warranty -r | -w value] param_name\n");
	printf("Possible param names:\n");

	for (; par->str != NULL; par += 1) {
		if (!lc15bts_par_is_int(par->value))
			continue;
		printf(" %s\n", par->str);
	}
}

static int parse_options(int argc, char **argv)
{
	while (1) {
		int option_idx = 0, c;
		static const struct option long_options[] = {
			{ "help", 0, 0, 'h' },
			{ "read", 0, 0, 'r' },
			{ "void-warranty", 0, 0, 1000},
			{ "write", 1, 0, 'w' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "rw:h",
				long_options, &option_idx);
		if (c == -1)
			break;
		switch (c) {
		case 'r':
			action = ACT_GET;
			break;
		case 'w':
			action = ACT_SET;
			write_arg = optarg;
			break;
		case 'h':
			print_help();
			return -1;
			break;
		case 1000:
			printf("Will void warranty on write.\n");
			void_warranty = 1;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	const char *parname;
	enum lc15bts_par par;
	int rc, val;

	tall_util_ctx = talloc_named_const(NULL, 1, "lc15 utils");
	msgb_talloc_ctx_init(tall_util_ctx, 0);

	rc = parse_options(argc, argv);
	if (rc < 0)
		exit(2);

	if (optind >= argc) {
		fprintf(stderr, "You must specify the parameter name\n");
		exit(2);
	}
	parname = argv[optind];

	rc = get_string_value(lc15bts_par_names, parname);
	if (rc < 0) {
		fprintf(stderr, "`%s' is not a valid parameter\n", parname);
		exit(2);
	} else
		par = rc;

	switch (action) {
	case ACT_GET:
		rc = lc15bts_par_get_int(tall_util_ctx, par, &val);
		if (rc < 0) {
			fprintf(stderr, "Error %d\n", rc);
			goto err;
		}
		printf("%d\n", val);
		break;
	case ACT_SET:
		rc = lc15bts_par_get_int(tall_util_ctx, par, &val);
		if (rc < 0) {
			fprintf(stderr, "Error %d\n", rc);
			goto err;
		}
		if (val != 0xFFFF && val != 0xFF && val != 0xFFFFFFFF && !void_warranty) {
			fprintf(stderr, "Parameter is already set!\r\n");
			goto err;
		}
		rc = lc15bts_par_set_int(tall_util_ctx, par, atoi(write_arg));
		if (rc < 0) {
			fprintf(stderr, "Error %d\n", rc);
			goto err;
		}
		printf("Success setting %s=%d\n", parname,
			atoi(write_arg));
		break;
	default:
		fprintf(stderr, "Unsupported action\n");
		goto err;
	}

	exit(0);

err:
	exit(1);
}

