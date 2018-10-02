/* Main program for SysmoBTS management daemon */

/* (C) 2012 by Harald Welte <laforge@gnumonks.org>
 * (C) 2014 by Holger Hans Peter Freyther
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

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/msgb.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>
#include <osmocom/vty/ports.h>
#include <osmocom/ctrl/control_if.h>
#include <osmocom/ctrl/ports.h>

#include "misc/sysmobts_misc.h"
#include "misc/sysmobts_mgr.h"
#include "misc/sysmobts_par.h"

static int bts_type;
static int trx_number;

static int no_eeprom_write = 0;
static int daemonize = 0;
void *tall_mgr_ctx;

/* every 6 hours means 365*4 = 1460 EEprom writes per year (max) */
#define TEMP_TIMER_SECS		(6 * 3600)

/* every 1 hours means 365*24 = 8760 EEprom writes per year (max) */
#define HOURS_TIMER_SECS	(1 * 3600)

/* the initial state */
static struct sysmobts_mgr_instance manager = {
	.config_file	= "sysmobts-mgr.cfg",
	.rf_limit	= {
		.thresh_warn	= 60,
		.thresh_crit	= 78,
	},
	.digital_limit = {
		.thresh_warn	= 60,
		.thresh_crit	= 78,
	},
	.board_limit	= {
		.thresh_warn	= 60,
		.thresh_crit	= 78,
	},
	.pa_limit	= {
		.thresh_warn	= 60,
		.thresh_crit	= 100,
	},
	.action_warn		= 0,
	.action_crit		= TEMP_ACT_PA_OFF,
	.state			= STATE_NORMAL,
};


static int classify_bts(void)
{
	int rc;

	rc = sysmobts_get_type(&bts_type);
	if (rc < 0) {
		fprintf(stderr, "Failed to get model number.\n");
		return -1;
	}

	rc = sysmobts_get_trx(&trx_number);
	if (rc < 0) {
		fprintf(stderr, "Failed to get the trx number.\n");
		return -1;
	}

	return 0;
}

int sysmobts_bts_type(void)
{
	return bts_type;
}

int sysmobts_trx_number(void)
{
	return trx_number;
}

int is_sbts2050(void)
{
	return bts_type == 2050;
}

int is_sbts2050_trx(int trx)
{
	return trx_number == trx;
}

int is_sbts2050_master(void)
{
	if (!is_sbts2050())
		return 0;
	if (!is_sbts2050_trx(0))
		return 0;
	return 1;
}

static struct osmo_timer_list temp_timer;
static void check_temp_timer_cb(void *unused)
{
	sysmobts_check_temp(no_eeprom_write);

	osmo_timer_schedule(&temp_timer, TEMP_TIMER_SECS, 0);
}

static struct osmo_timer_list hours_timer;
static void hours_timer_cb(void *unused)
{
	sysmobts_update_hours(no_eeprom_write);

	osmo_timer_schedule(&hours_timer, HOURS_TIMER_SECS, 0);
}

static void print_help(void)
{
	printf("sysmobts-mgr [-nsD] [-d cat]\n");
	printf(" -n Do not write to EEPROM\n");
	printf(" -s Disable color\n");
	printf(" -d CAT enable debugging\n");
	printf(" -D daemonize\n");
	printf(" -c Specify the filename of the config file\n");
}

static int parse_options(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "nhsd:c:")) != -1) {
		switch (opt) {
		case 'n':
			no_eeprom_write = 1;
			break;
		case 'h':
			print_help();
			return -1;
		case 's':
			log_set_use_color(osmo_stderr_target, 0);
			break;
		case 'd':
			log_parse_category_mask(osmo_stderr_target, optarg);
			break;
		case 'D':
			daemonize = 1;
			break;
		case 'c':
			manager.config_file = optarg;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

static void signal_handler(int signal)
{
	fprintf(stderr, "signal %u received\n", signal);

	switch (signal) {
	case SIGINT:
	case SIGTERM:
		sysmobts_check_temp(no_eeprom_write);
		sysmobts_update_hours(no_eeprom_write);
		exit(0);
		break;
	case SIGABRT:
	case SIGUSR1:
	case SIGUSR2:
		talloc_report_full(tall_mgr_ctx, stderr);
		break;
	default:
		break;
	}
}

static struct log_info_cat mgr_log_info_cat[] = {
	[DTEMP] = {
		.name = "DTEMP",
		.description = "Temperature monitoring",
		.color = "\033[1;35m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DFW] =	{
		.name = "DFW",
		.description = "DSP/FPGA firmware management",
		.color = "\033[1;36m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DFIND] = {
		.name = "DFIND",
		.description = "ipaccess-find handling",
		.color = "\033[1;37m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DCALIB] = {
		.name = "DCALIB",
		.description = "Calibration handling",
		.color = "\033[1;37m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
};

static const struct log_info mgr_log_info = {
	.cat = mgr_log_info_cat,
	.num_cat = ARRAY_SIZE(mgr_log_info_cat),
};

int main(int argc, char **argv)
{
	int rc;
	struct ctrl_connection *ccon;

	tall_mgr_ctx = talloc_named_const(NULL, 1, "bts manager");
	msgb_talloc_ctx_init(tall_mgr_ctx, 0);

	srand(time(NULL));

	osmo_init_logging2(tall_mgr_ctx, &mgr_log_info);
	if (classify_bts() != 0)
		exit(2);

	osmo_init_ignore_signals();
	signal(SIGINT, &signal_handler);
	signal(SIGTERM, &signal_handler);
	signal(SIGUSR1, &signal_handler);
	signal(SIGUSR2, &signal_handler);

	rc = parse_options(argc, argv);
	if (rc < 0)
		exit(2);

	sysmobts_mgr_vty_init();
	logging_vty_add_cmds(&mgr_log_info);
	rc = sysmobts_mgr_parse_config(&manager);
	if (rc < 0) {
		LOGP(DFIND, LOGL_FATAL, "Cannot parse config file\n");
		exit(1);
	}

	rc = telnet_init(tall_mgr_ctx, NULL, OSMO_VTY_PORT_BTSMGR);
	if (rc < 0) {
		fprintf(stderr, "Error initializing telnet\n");
		exit(1);
	}

	/* start temperature check timer */
	temp_timer.cb = check_temp_timer_cb;
	check_temp_timer_cb(NULL);

	/* start operational hours timer */
	hours_timer.cb = hours_timer_cb;
	hours_timer_cb(NULL);

	/* start uc temperature check timer */
	sbts2050_uc_initialize();

	/* handle broadcast messages for ipaccess-find */
	if (sysmobts_mgr_nl_init() != 0)
		exit(3);

	/* Initialize the temperature control */
	ccon = osmo_ctrl_conn_alloc(tall_mgr_ctx, NULL);
	rc = -1;
	if (ccon) {
		ccon->write_queue.bfd.data = ccon;
		rc = osmo_sock_init_ofd(&ccon->write_queue.bfd, AF_INET,
					SOCK_STREAM, IPPROTO_TCP,
					"localhost", OSMO_CTRL_PORT_BTS,
					OSMO_SOCK_F_CONNECT);
	}
	if (rc < 0)
		LOGP(DLCTRL, LOGL_ERROR, "Can't connect to CTRL @ localhost:%u\n",
		     OSMO_CTRL_PORT_BTS);
	else
		LOGP(DLCTRL, LOGL_NOTICE, "CTRL connected to locahost:%u\n",
		     OSMO_CTRL_PORT_BTS);

        sysmobts_mgr_temp_init(&manager, ccon);

	if (sysmobts_mgr_calib_init(&manager) != 0)
		exit(3);

	if (daemonize) {
		rc = osmo_daemonize();
		if (rc < 0) {
			perror("Error during daemonize");
			exit(1);
		}
	}


	while (1) {
		log_reset_context();
		osmo_select_main(0);
	}
}
