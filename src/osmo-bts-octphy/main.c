/* Main program of osmo-bts for OCTPHY-2G */

/* Copyright (c) 2014 Octasic Inc. All rights reserved.
 * Copyright (c) 2015 Harald Welte <laforge@gnumonks.org>
 *
 * based on a copy of osmo-bts-sysmo/main.c, which is
 * Copyright (C) 2011-2013 by Harald Welte <laforge@gnumonks.org>
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
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/l1sap.h>

#include "l1_if.h"

#define RF_LOCK_PATH	"/var/lock/bts_rf_lock"

extern int pcu_direct;
extern bool no_fw_check;

int bts_model_print_help()
{
	printf("  -I	--no-fw-check	Override firmware version check\n");
	return 0;
}

int bts_model_handle_options(int argc, char **argv)
{
	int num_errors = 0;

	while (1) {
		int option_idx = 0, c;
		static const struct option long_options[] = {
			/* specific to this hardware */
			{ "no-fw-check", 0, 0, 'I' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "I",
				long_options, &option_idx);
		if (c == -1)
			break;

		switch (c) {
		case 'I':
			no_fw_check = true;
			break;
		default:
			num_errors++;
			break;
		}
	}

	return num_errors;
}

void bts_model_abis_close(struct gsm_bts *bts)
{
	/* for now, we simply terminate the program and re-spawn */
	bts_shutdown(bts, "Abis close");
}

int main(int argc, char **argv)
{
	return bts_main(argc, argv);
}
