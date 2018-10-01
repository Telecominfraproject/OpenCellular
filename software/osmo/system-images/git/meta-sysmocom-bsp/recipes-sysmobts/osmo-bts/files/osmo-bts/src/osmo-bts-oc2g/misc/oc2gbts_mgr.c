/* Main program for NuRAN Wireless OC-2G BTS management daemon */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_mgr.c
 *     (C) 2012 by Harald Welte <laforge@gnumonks.org>
 *     (C) 2014 by Holger Hans Peter Freyther
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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/signal.h>
#include <sys/stat.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/msgb.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>
#include <osmocom/vty/ports.h>

#include "misc/oc2gbts_misc.h"
#include "misc/oc2gbts_mgr.h"
#include "misc/oc2gbts_par.h"
#include "misc/oc2gbts_bid.h"
#include "misc/oc2gbts_power.h"
#include "misc/oc2gbts_swd.h"

#include "oc2gbts_led.h"

static int no_rom_write = 0;
static int daemonize = 0;
void *tall_mgr_ctx;

/* every 6 hours means 365*4 = 1460 rom writes per year (max) */
#define SENSOR_TIMER_SECS		(6 * 3600)

/* every 1 hours means 365*24 = 8760 rom writes per year (max) */
#define HOURS_TIMER_SECS	(1 * 3600)

/* the initial state */
static struct oc2gbts_mgr_instance manager = {
	.config_file	= "oc2gbts-mgr.cfg",
	.temp = {
		.supply_temp_limit = {
			.thresh_warn_max	= 80,
			.thresh_crit_max	= 85,
			.thresh_warn_min	= -40,
		},
		.soc_temp_limit = {
			.thresh_warn_max	= 95,
			.thresh_crit_max	= 100,
			.thresh_warn_min	= -40,
		},
		.fpga_temp_limit = {
			.thresh_warn_max	= 95,
			.thresh_crit_max	= 100,
			.thresh_warn_min	= -40,
		},
		.rmsdet_temp_limit = {
			.thresh_warn_max	= 80,
			.thresh_crit_max	= 85,
			.thresh_warn_min	= -40,
		},
		.ocxo_temp_limit = {
			.thresh_warn_max	= 80,
			.thresh_crit_max	= 85,
			.thresh_warn_min	= -40,
		},
		.tx_temp_limit = {
			.thresh_warn_max	= 80,
			.thresh_crit_max	= 85,
			.thresh_warn_min	= -20,
		},
		.pa_temp_limit = {
			.thresh_warn_max	= 80,
			.thresh_crit_max	= 85,
			.thresh_warn_min	= -40,
		}
	},
	.volt = {
		.supply_volt_limit = {
			.thresh_warn_max	= 30000,
			.thresh_crit_max	= 30500,
			.thresh_warn_min	= 19000,
			.thresh_crit_min	= 17500,
		}
	},
	.pwr = {
		.supply_pwr_limit = {
			.thresh_warn_max	= 30,
			.thresh_crit_max	= 40,
		},
		.pa_pwr_limit = {
			.thresh_warn_max	= 20,
			.thresh_crit_max	= 30,
		}
	},
	.vswr = {
		.vswr_limit = {
			.thresh_warn_max	= 3000,
			.thresh_crit_max	= 5000,
		}
	},
	.gps = {
		.gps_fix_limit = {
			.thresh_warn_max	= 7,
		}
	},
	.state = {
		.action_norm	= SENSOR_ACT_NORM_PA_ON,
		.action_warn	= 0,
		.action_crit	= 0,
		.action_comb	= 0,
		.state			= STATE_NORMAL,
	}
};

static struct osmo_timer_list sensor_timer;
static void check_sensor_timer_cb(void *unused)
{
	oc2gbts_check_temp(no_rom_write);
	oc2gbts_check_power(no_rom_write);
	oc2gbts_check_vswr(no_rom_write);
	osmo_timer_schedule(&sensor_timer, SENSOR_TIMER_SECS, 0);
	/* TODO checks if oc2gbts_check_temp/oc2gbts_check_power/oc2gbts_check_vswr went ok */
	oc2gbts_swd_event(&manager, SWD_CHECK_SENSOR);
}

static struct osmo_timer_list hours_timer;
static void hours_timer_cb(void *unused)
{
	oc2gbts_update_hours(no_rom_write);

	osmo_timer_schedule(&hours_timer, HOURS_TIMER_SECS, 0);
	/* TODO: validates if oc2gbts_update_hours went correctly */
	oc2gbts_swd_event(&manager, SWD_UPDATE_HOURS);
}

static void print_help(void)
{
	printf("oc2gbts-mgr [-nsD] [-d cat]\n");
	printf(" -n Do not write to ROM\n");
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
			no_rom_write = 1;
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
		oc2gbts_check_temp(no_rom_write);
		oc2gbts_check_power(no_rom_write);
		oc2gbts_check_vswr(no_rom_write);
		oc2gbts_update_hours(no_rom_write);
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
		.description = "Firmware management",
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
	[DSWD] = {
		.name = "DSWD",
		.description = "Software Watchdog",
		.color = "\033[1;37m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
};

static const struct log_info mgr_log_info = {
	.cat = mgr_log_info_cat,
	.num_cat = ARRAY_SIZE(mgr_log_info_cat),
};

static int mgr_log_init(void)
{
	osmo_init_logging(&mgr_log_info);
	return 0;
}

int main(int argc, char **argv)
{
	void *tall_msgb_ctx;
	int rc;
	pthread_t tid;

	tall_mgr_ctx = talloc_named_const(NULL, 1, "bts manager");
	tall_msgb_ctx = talloc_named_const(tall_mgr_ctx, 1, "msgb");
	msgb_set_talloc_ctx(tall_msgb_ctx);

	mgr_log_init();

	osmo_init_ignore_signals();
	signal(SIGINT, &signal_handler);
	signal(SIGUSR1, &signal_handler);
	signal(SIGUSR2, &signal_handler);

	rc = parse_options(argc, argv);
	if (rc < 0)
		exit(2);

	oc2gbts_mgr_vty_init();
	logging_vty_add_cmds(&mgr_log_info);
	rc = oc2gbts_mgr_parse_config(&manager);
	if (rc < 0) {
		LOGP(DFIND, LOGL_FATAL, "Cannot parse config file\n");
		exit(1);
	}

	rc = telnet_init(tall_mgr_ctx, NULL, OSMO_VTY_PORT_BTSMGR);
	if (rc < 0) {
		fprintf(stderr, "Error initializing telnet\n");
		exit(1);
	}

	INIT_LLIST_HEAD(&manager.oc2gbts_leds.list);
	INIT_LLIST_HEAD(&manager.alarms.list);

	/* Initialize the service watchdog notification for SWD_LAST event(s) */
	if (oc2gbts_swd_init(&manager, (int)(SWD_LAST)) != 0)
		exit(3);

	/* start temperature check timer */
	sensor_timer.cb = check_sensor_timer_cb;
	check_sensor_timer_cb(NULL);

	/* start operational hours timer */
	hours_timer.cb = hours_timer_cb;
	hours_timer_cb(NULL);

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		/* Enable the PAs */
		rc = oc2gbts_power_set(OC2GBTS_POWER_PA, 1);
		if (rc < 0) {
			exit(3);
		}
	}
	/* handle broadcast messages for ipaccess-find */
	if (oc2gbts_mgr_nl_init() != 0)
		exit(3);

	/* Initialize the sensor control */
	oc2gbts_mgr_sensor_init(&manager);

	if (oc2gbts_mgr_calib_init(&manager) != 0)
		exit(3);

	if (oc2gbts_mgr_control_init(&manager))
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
		oc2gbts_swd_event(&manager, SWD_MAINLOOP);
	}
}
