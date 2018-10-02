/* Main program for Osmocom BTS */

/* (C) 2011-2016 by Harald Welte <laforge@gnumonks.org>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/gsmtap.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/control_if.h>
#include <osmocom/ctrl/control_if.h>
#include <osmocom/ctrl/ports.h>
#include <osmocom/ctrl/control_vty.h>
#include <osmo-bts/oml.h>

int quit = 0;
static const char *config_file = "osmo-bts.cfg";
static int daemonize = 0;
static int rt_prio = -1;
static int trx_num = 1;
static char *gsmtap_ip = 0;
extern int g_vty_port_num;

static void print_help()
{
	printf( "Some useful options:\n"
		"  -h	--help		this text\n"
		"  -d	--debug MASK	Enable debugging (e.g. -d DRSL:DOML:DLAPDM)\n"
		"  -D	--daemonize	For the process into a background daemon\n"
		"  -c	--config-file 	Specify the filename of the config file\n"
		"  -s	--disable-color	Don't use colors in stderr log output\n"
		"  -T	--timestamp	Prefix every log line with a timestamp\n"
		"  -V	--version	Print version information and exit\n"
		"  -e 	--log-level	Set a global log-level\n"
		"  -r	--realtime PRIO	Use SCHED_RR with the specified priority\n"
		"  -i	--gsmtap-ip	The destination IP used for GSMTAP.\n"
		"  -t	--trx-num	Set number of TRX (default=%d)\n",
		trx_num
		);
	bts_model_print_help();
}

/* FIXME: finally get some option parsing code into libosmocore */
static void handle_options(int argc, char **argv)
{
	char *argv_out[argc];
	int argc_out = 0;

	argv_out[argc_out++] = argv[0];

	/* disable generation of error messages on encountering unknown
	 * options */
	opterr = 0;

	while (1) {
		int option_idx = 0, c;
		static const struct option long_options[] = {
			/* FIXME: all those are generic Osmocom app options */
			{ "help", 0, 0, 'h' },
			{ "debug", 1, 0, 'd' },
			{ "daemonize", 0, 0, 'D' },
			{ "config-file", 1, 0, 'c' },
			{ "disable-color", 0, 0, 's' },
			{ "timestamp", 0, 0, 'T' },
			{ "version", 0, 0, 'V' },
			{ "log-level", 1, 0, 'e' },
			/* FIXME: generic BTS app options */
			{ "gsmtap-ip", 1, 0, 'i' },
			{ "trx-num", 1, 0, 't' },
			{ "realtime", 1, 0, 'r' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "-hc:d:Dc:sTVe:i:t:r:",
				long_options, &option_idx);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_help();
			exit(0);
			break;
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
			config_file = optarg;
			break;
		case 'T':
			log_set_print_timestamp(osmo_stderr_target, 1);
			break;
		case 'V':
			print_version(1);
			exit(0);
			break;
		case 'e':
			log_set_log_level(osmo_stderr_target, atoi(optarg));
			break;
		case 'r':
			rt_prio = atoi(optarg);
			break;
		case 'i':
			gsmtap_ip = optarg;
			break;
		case 't':
			trx_num = atoi(optarg);
			if (trx_num < 1)
				trx_num = 1;
			break;
		case '?':
		case 1:
			/* prepare argv[] for bts_model */
			argv_out[argc_out++] = argv[optind-1];
			break;
		default:
			break;
		}
	}

	/* re-set opt-ind for new parsig round */
	optind = 1;
	/* enable error-checking for the following getopt call */
	opterr = 1;
	if (bts_model_handle_options(argc_out, argv_out)) {
		print_help();
		exit(1);
	}
}

static struct gsm_bts *bts;

static void signal_handler(int signal)
{
	fprintf(stderr, "signal %u received\n", signal);

	switch (signal) {
	case SIGINT:
	case SIGTERM:
		if (!quit) {
			oml_fail_rep(OSMO_EVT_CRIT_PROC_STOP,
				     "BTS: SIGINT received -> shutdown");
			bts_shutdown(bts, "SIGINT");
		}
		quit++;
		break;
	case SIGABRT:
	case SIGUSR1:
	case SIGUSR2:
		oml_fail_rep(OSMO_EVT_CRIT_PROC_STOP,
			     "BTS: signal %d (%s) received", signal,
			     strsignal(signal));
		talloc_report_full(tall_bts_ctx, stderr);
		break;
	default:
		break;
	}
}

static int write_pid_file(char *procname)
{
	FILE *outf;
	char tmp[PATH_MAX+1];

	snprintf(tmp, sizeof(tmp)-1, "/var/run/%s.pid", procname);
	tmp[PATH_MAX-1] = '\0';

	outf = fopen(tmp, "w");
	if (!outf)
		return -1;

	fprintf(outf, "%d\n", getpid());

	fclose(outf);

	return 0;
}

int bts_main(int argc, char **argv)
{
	struct gsm_bts_trx *trx;
	struct e1inp_line *line;
	int rc, i;

	printf("((*))\n  |\n / \\ OsmoBTS\n");

	/* Track the use of talloc NULL memory contexts */
	talloc_enable_null_tracking();

	tall_bts_ctx = talloc_named_const(NULL, 1, "OsmoBTS context");
	msgb_talloc_ctx_init(tall_bts_ctx, 100*1024);
	bts_vty_info.tall_ctx = tall_bts_ctx;

	osmo_init_logging2(tall_bts_ctx, &bts_log_info);
	vty_init(&bts_vty_info);
	ctrl_vty_init(tall_bts_ctx);
	rate_ctr_init(tall_bts_ctx);

	handle_options(argc, argv);

	bts = gsm_bts_alloc(tall_bts_ctx, 0);
	if (!bts) {
		fprintf(stderr, "Failed to create BTS structure\n");
		exit(1);
	}
	for (i = 1; i < trx_num; i++) {
		trx = gsm_bts_trx_alloc(bts);
		if (!trx) {
			fprintf(stderr, "Failed to create TRX structure\n");
			exit(1);
		}
	}
	e1inp_vty_init();
	bts_vty_init(bts, &bts_log_info);

	/* enable realtime priority for us */
	if (rt_prio != -1) {
		struct sched_param param;

		memset(&param, 0, sizeof(param));
		param.sched_priority = rt_prio;
		rc = sched_setscheduler(getpid(), SCHED_RR, &param);
		if (rc != 0) {
			fprintf(stderr, "Setting SCHED_RR priority(%d) failed: %s\n",
				param.sched_priority, strerror(errno));
			exit(1);
		}
	}

        if (gsmtap_ip) {
		gsmtap = gsmtap_source_init(gsmtap_ip, GSMTAP_UDP_PORT, 1);
		if (!gsmtap) {
			fprintf(stderr, "Failed during gsmtap_init()\n");
			exit(1);
		}
		gsmtap_source_add_sink(gsmtap);
	}

	if (bts_init(bts) < 0) {
		fprintf(stderr, "unable to open bts\n");
		exit(1);
	}

	abis_init(bts);

	rc = vty_read_config_file(config_file, NULL);
	if (rc < 0) {
		fprintf(stderr, "Failed to parse the config file: '%s'\n",
			config_file);
		exit(1);
	}

	if (!phy_link_by_num(0)) {
		fprintf(stderr, "You need to configure at least phy0\n");
		exit(1);
	}

	llist_for_each_entry(trx, &bts->trx_list, list) {
		if (!trx->role_bts.l1h) {
			fprintf(stderr, "TRX %u has no associated PHY instance\n",
				trx->nr);
			exit(1);
		}
	}

	write_pid_file("osmo-bts");

	bts_controlif_setup(bts, ctrl_vty_get_bind_addr(), OSMO_CTRL_PORT_BTS);

	rc = telnet_init_dynif(tall_bts_ctx, NULL, vty_get_bind_addr(),
			       g_vty_port_num);
	if (rc < 0) {
		fprintf(stderr, "Error initializing telnet\n");
		exit(1);
	}

	if (pcu_sock_init(bts->pcu.sock_path)) {
		fprintf(stderr, "PCU L1 socket failed\n");
		exit(1);
	}

	signal(SIGINT, &signal_handler);
	signal(SIGTERM, &signal_handler);
	//signal(SIGABRT, &signal_handler);
	signal(SIGUSR1, &signal_handler);
	signal(SIGUSR2, &signal_handler);
	osmo_init_ignore_signals();

	if (!bts->bsc_oml_host) {
		fprintf(stderr, "Cannot start BTS without knowing BSC OML IP\n");
		exit(1);
	}

	line = abis_open(bts, bts->bsc_oml_host, "sysmoBTS");
	if (!line) {
		fprintf(stderr, "unable to connect to BSC\n");
		exit(2);
	}

	rc = phy_links_open();
	if (rc < 0) {
		fprintf(stderr, "unable to open PHY link(s)\n");
		exit(2);
	}

	if (daemonize) {
		rc = osmo_daemonize();
		if (rc < 0) {
			perror("Error during daemonize");
			exit(1);
		}
	}

	while (quit < 2) {
		log_reset_context();
		osmo_select_main(0);
	}

	return EXIT_SUCCESS;
}
