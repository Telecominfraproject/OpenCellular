/* OCXO/TCXO based calibration utility			*/

/*
 * (C) 2012-2013 Holger Hans Peter Freyther
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>

#define _GNU_SOURCE
#include <getopt.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1types.h>

#include <osmocom/gsm/gsm_utils.h>

#include <osmocom/core/utils.h>

#include "sysmobts-layer1.h"

enum actions {
	ACTION_SCAN,
	ACTION_CALIB,
	ACTION_BCCH,
	ACTION_BCCH_CCCH,
};

static const char *modes[] = {
	[ACTION_SCAN]		= "scan",
	[ACTION_CALIB]		= "calibrate",
	[ACTION_BCCH]		= "bcch",
	[ACTION_BCCH_CCCH]	= "bcch_ccch",
};

static const char *bands[] = {
	[GsmL1_FreqBand_850]	= "850",
	[GsmL1_FreqBand_900]	= "900",
	[GsmL1_FreqBand_1800]	= "1800",
	[GsmL1_FreqBand_1900]	= "1900",	
};

struct channel_pair {
	int min;
	int max;
};

static const struct channel_pair arfcns[] = {
	[GsmL1_FreqBand_850]	= { .min = 128,	.max = 251 },
	[GsmL1_FreqBand_900]	= { .min = 1,	.max = 124 },
	[GsmL1_FreqBand_1800]	= { .min = 512,	.max = 885 },
	[GsmL1_FreqBand_1900]	= { .min = 512,	.max = 810 },

};

static const char *clk_source[] = {
	[SuperFemto_ClkSrcId_Ocxo]	= "ocxo",
	[SuperFemto_ClkSrcId_Tcxo]	= "tcxo",
	[SuperFemto_ClkSrcId_External]	= "external",
	[SuperFemto_ClkSrcId_GpsPps]	= "gps",
	[SuperFemto_ClkSrcId_Trx]	= "trx",
	[SuperFemto_ClkSrcId_Rx]	= "rx",
	[SuperFemto_ClkSrcId_Edge]	= "edge",
	[SuperFemto_ClkSrcId_NetList]	= "netlisten",
};

static const struct value_string sapi_names[GsmL1_Sapi_NUM+1] = {
	{ GsmL1_Sapi_Fcch,	"FCCH" },
	{ GsmL1_Sapi_Sch,	"SCH" },
	{ GsmL1_Sapi_Sacch,	"SACCH" },
	{ GsmL1_Sapi_Sdcch,	"SDCCH" },
	{ GsmL1_Sapi_Bcch,	"BCCH" },
	{ GsmL1_Sapi_Pch,	"PCH" },
	{ GsmL1_Sapi_Agch,	"AGCH" },
	{ GsmL1_Sapi_Cbch,	"CBCH" },
	{ GsmL1_Sapi_Rach,	"RACH" },
	{ GsmL1_Sapi_TchF,	"TCH/F" },
	{ GsmL1_Sapi_FacchF,	"FACCH/F" },
	{ GsmL1_Sapi_TchH,	"TCH/H" },
	{ GsmL1_Sapi_FacchH,	"FACCH/H" },
	{ GsmL1_Sapi_Nch,	"NCH" },
	{ GsmL1_Sapi_Pdtch,	"PDTCH" },
	{ GsmL1_Sapi_Pacch,	"PACCH" },
	{ GsmL1_Sapi_Pbcch,	"PBCCH" },
	{ GsmL1_Sapi_Pagch,	"PAGCH" },
	{ GsmL1_Sapi_Ppch,	"PPCH" },
	{ GsmL1_Sapi_Pnch,	"PNCH" },
	{ GsmL1_Sapi_Ptcch,	"PTCCH" },
	{ GsmL1_Sapi_Prach,	"PRACH" },
	{ 0, NULL }
};

static int action = ACTION_SCAN;
static int band = GsmL1_FreqBand_900;
static int calib = SuperFemto_ClkSrcId_Ocxo;
static int source = SuperFemto_ClkSrcId_NetList;
static int dsp_flags = 0x0;
static int cal_arfcn = 0;
static int initial_cor = 0;
static int steps = -1;

static void print_usage(void)
{
	printf("Usage: sysmobts-calib ARGS\n");
}

static void print_help(void)
{
	printf("  -h --help this text\n");
	printf("  -c --clock "
		"ocxo|tcxo|external|gps|trx|rx|edge\n");
	printf("  -s --calibration-source "
		"ocxo|tcxo|external|gps|trx|rx|edge|netlisten\n");
	printf("  -b --band 850|900|1800|1900\n");
	printf("  -m --mode scan|calibrate|bcch|bcch_ccch\n");
	printf("  -a --arfcn NR arfcn for calibration\n");
	printf("  -d --dsp-flags NR dsp mask for debug log\n");
	printf("  -t --threshold level\n");
	printf("  -i --initial-clock-correction COR.\n");
	printf("  -t --steps STEPS\n");
}

static int find_value(const char **array, int size, char *value)
{
	int i = 0;
	for (i = 0; i < size; ++i) {
		if (array[i] == NULL)
			continue;
		if (strcmp(value, array[i]) == 0)
			return i;
	}

	printf("Failed to find: '%s'\n", value);
	exit(-2);
}

static void handle_options(int argc, char **argv)
{
	while (1) {
		int option_index = 0, c;
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"calibration-source", 1, 0, 's'},
			{"clock", 1, 0, 'c'},
			{"mode", 1, 0, 'm'},
			{"band", 1, 0, 'b'},
			{"dsp-flags", 1, 0, 'd'},
			{"arfcn", 1, 0, 'a'},
			{"initial-clock-correction", 1, 0, 'i'},
			{"steps", 1, 0, 't'},
			{0, 0, 0, 0},
		};

		c = getopt_long(argc, argv, "hs:c:m:b:d:a:i:t:",
			long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			print_usage();
			print_help();
			exit(0);
		case 's':
			source = find_value(clk_source,
					ARRAY_SIZE(clk_source), optarg);
			break;
		case 'c':
			calib = find_value(clk_source,
					ARRAY_SIZE(clk_source), optarg);
			break;
		case 'm':
			action = find_value(modes,
					ARRAY_SIZE(modes), optarg);
			break;
		case 'b':
			band = find_value(bands,
					ARRAY_SIZE(bands), optarg);
			break;
		case 'd':
			dsp_flags = strtol(optarg, NULL, 16);
			break;
		case 'a':
			cal_arfcn = atoi(optarg);
			break;
		case 'i':
			initial_cor = atoi(optarg);
			break;
		case 't':
			steps = atoi(optarg);
			break;
		default:
			printf("Unhandled option, terminating.\n");
			exit(-1);
		}
	}

	if (source == calib) {
		printf("Clock source and reference clock may not be the same.\n");
		exit(-3);
	}

	if (calib == SuperFemto_ClkSrcId_NetList) {
		printf("Clock may not be network listen.\n");
		exit(-4);
	}

	if (action == ACTION_CALIB && source == SuperFemto_ClkSrcId_NetList) {
		if (cal_arfcn == 0) {
			printf("Please specify the reference ARFCN.\n");
			exit(-5);
		}

		if (cal_arfcn < arfcns[band].min || cal_arfcn > arfcns[band].max) {
			printf("ARFCN(%d) is not in the given band.\n", cal_arfcn);
			exit(-6);
		}
	}
}

#define CHECK_RC(rc) \
	if (rc != 0) \
		return EXIT_FAILURE;

#define CHECK_RC_MSG(rc, msg) \
	if (rc != 0) { \
		printf("%s: %d\n", msg, rc);	\
		return EXIT_FAILURE; 		\
	}
#define CHECK_COND_MSG(cond, rc, msg) \
	if (cond) { \
		printf("%s: %d\n", msg, rc);	\
		return EXIT_FAILURE; 		\
	}

struct scan_result 
{
    uint16_t    arfcn;
    float       rssi;
};

static int scan_cmp(const void *arg1, const void *arg2)
{
	struct scan_result *elem1 = (struct scan_result *) arg1;
	struct scan_result *elem2 = (struct scan_result * )arg2;
   
	float diff = elem1->rssi - elem2->rssi;
	if (diff > 0.0)
		return 1;
	else if (diff < 0.0)
		return -1;
	else
        	return 0;
}

static int scan_band()
{
	int arfcn, rc, i;

	/* Scan results.. at most 400 items */
	struct scan_result results[400];
	memset(&results, 0, sizeof(results));
	int num_scan_results = 0;

	printf("Going to scan bands.\n");

	for (arfcn = arfcns[band].min; arfcn <= arfcns[band].max; ++arfcn) {
		float mean_rssi;

		printf(".");
		fflush(stdout);
		rc = power_scan(band, arfcn, 10, &mean_rssi);
		CHECK_RC_MSG(rc, "Power Measurement failed");

                results[num_scan_results].arfcn = arfcn;
                results[num_scan_results].rssi = mean_rssi;
                num_scan_results++;
	}

    	qsort(results, num_scan_results, sizeof(struct scan_result), scan_cmp);
    	printf("\nSorted scan results (weakest first):\n");
	for (i = 0; i < num_scan_results; ++i)
		printf("ARFCN %3d: %.4f\n", results[i].arfcn, results[i].rssi);

	return 0;
}

static int calib_get_clock_error(void)
{
	int rc, clkErr, clkErrRes;

	printf("Going to determine the clock offset.\n");

	rc = rf_clock_info(&clkErr, &clkErrRes);
	CHECK_RC_MSG(rc, "Clock info failed.\n");

	if (clkErr == 0 && clkErrRes == 0) {
		printf("Failed to get the clock info. Are both clocks present?\n");
		return -1;
	}

	/*
	 * Empiric gps error determination. With revE and firmware v3.3
	 * the clock error for TCXO to GPS appears to have a different
	 * sign. The device in question doesn't have a networklisten mode
	 * so it is impossible to verify that this only applies to GPS.
	 */
	if (source == SuperFemto_ClkSrcId_GpsPps)
		clkErr *= -1;


	/* this is an absolute clock error */
	printf("The calibration value is: %d\n", clkErr);
	return 0;
}

static int calib_clock_after_sync(void)
{
	int rc, clkErr, clkErrRes, iteration, cor;

	iteration = 0;
	cor = initial_cor;

	printf("Trying to calibrate now and reducing clock error.\n");

	for (iteration = 0; iteration < steps || steps <= 0; ++iteration) {
		if (steps > 0)
			printf("Iteration %d/%d with correction: %d\n", iteration, steps, cor);
		else
			printf("Iteration %d with correction: %d\n", iteration, cor);
	
		rc = rf_clock_info(&clkErr, &clkErrRes);
		CHECK_RC_MSG(rc, "Clock info failed.\n");

		/*
		 * TODO: use the clock error resolution here, implement it as a
                 * a PID controller..
		 */

		/* Picocell class requires 0.1ppm.. but that is 'too easy' */
		if (fabs(clkErr / 1000.0f) <= 0.05f) {
			printf("The calibration value is: %d\n", cor);
			return 1;
		}

		cor -= clkErr / 2;
		rc = set_clock_cor(cor, calib, source);
		CHECK_RC_MSG(rc, "Clock correction failed.\n");
	}

	return -1;
}

static int find_initial_clock(HANDLE layer1, int *clock)
{
	int i;

	printf("Trying to find an initial clock value.\n");

	for (i = 0; i < 1000; ++i) {
		int rc;
		int cor = i * 150;
		rc = wait_for_sync(layer1, cor, calib, source);
		if (rc == 1) {
			printf("Found initial clock offset: %d\n", cor);
			*clock = cor;
			break;
		} else {
			CHECK_RC_MSG(rc, "Failed to set new clock value.\n");
		}

		cor = i * -150;
		rc = wait_for_sync(layer1, cor, calib, source);
		if (rc == 1) {
			printf("Found initial clock offset: %d\n", cor);
			*clock = cor;
			break;
		} else {
			CHECK_RC_MSG(rc, "Failed to set new clock value.\n");
		}
	}

	return 0;
}

static int calib_clock_netlisten(void)
{
	int rc, cor = initial_cor;
	float mean_rssi;
	HANDLE layer1;

	rc = power_scan(band, cal_arfcn, 10, &mean_rssi);
	CHECK_RC_MSG(rc, "ARFCN measurement scan failed");
	if (mean_rssi < -118.0f)
		printf("ARFCN has weak signal for calibration: %f\n", mean_rssi);

	/* initial lock */
	rc = follow_sch(band, cal_arfcn, calib, source, &layer1);
	if (rc == -23)
		rc = find_initial_clock(layer1, &cor);
	CHECK_RC_MSG(rc, "Following SCH failed");

	/* now try to calibrate it */
	rc = set_clock_cor(cor, calib, source);
	CHECK_RC_MSG(rc, "Clock setup failed.");

	calib_clock_after_sync();

	rc = mph_close(layer1);
	CHECK_RC_MSG(rc, "MPH-Close");

	return EXIT_SUCCESS;
}

static int calib_clock(void)
{
	int rc;

	/* now try to calibrate it */
	rc = set_clock_cor(initial_cor, calib, source);
	CHECK_RC_MSG(rc, "Clock setup failed.");

	calib_get_clock_error();

	return EXIT_SUCCESS;
}

static int bcch_follow(void)
{
	int rc, cor = initial_cor;
	float mean_rssi;
	HANDLE layer1;

	rc = power_scan(band, cal_arfcn, 10, &mean_rssi);
	CHECK_RC_MSG(rc, "ARFCN measurement scan failed");
	if (mean_rssi < -118.0f)
		printf("ARFCN has weak signal for calibration: %f\n", mean_rssi);

	/* initial lock */
	rc = follow_sch(band, cal_arfcn, calib, source, &layer1);
	if (rc == -23)
		rc = find_initial_clock(layer1, &cor);
	CHECK_RC_MSG(rc, "Following SCH failed");

	/* identify the BSIC and set it as TSC */
	rc = find_bsic();
	CHECK_COND_MSG(rc < 0, rc, "Identifying the BSIC failed");
	rc = set_tsc_from_bsic(layer1, rc);
	CHECK_RC_MSG(rc, "Setting the TSC failed");


	/* follow the bcch */
	rc = follow_bcch(layer1);
	CHECK_RC_MSG(rc, "Follow BCCH");

	/* follow the pch */
	if (action == ACTION_BCCH_CCCH) {
		rc = follow_pch(layer1);
		CHECK_RC_MSG(rc, "Follow BCCH/CCCH");
	}

	/* now wait for the PhDataInd */
	for (;;) {
		uint32_t fn;
		uint8_t block;
		uint8_t data[23];
		size_t size;
		struct gsm_time gsmtime;
		GsmL1_Sapi_t sapi;

		rc = wait_for_data(data, &size, &fn, &block, &sapi);
		if (rc == 1)
			continue;
		CHECK_RC_MSG(rc, "No Data Indication");

		gsm_fn2gsmtime(&gsmtime, fn);
		printf("%02u/%02u/%02u %6s %s\n",
			gsmtime.t1, gsmtime.t2, gsmtime.t3,
			get_value_string(sapi_names, sapi),
			osmo_hexdump(data, size));
	}

	rc = mph_close(layer1);
	CHECK_RC_MSG(rc, "MPH-Close");

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int rc;

	handle_options(argc, argv);
	printf("Initializing the Layer1\n");
	rc = initialize_layer1(dsp_flags);
	CHECK_RC(rc);

	printf("Fetching system info.\n");
	rc = print_system_info();
	CHECK_RC(rc);

	printf("Opening RF frontend with clock(%d) and correction(%d)\n",
		calib, initial_cor);
	rc = activate_rf_frontend(calib, initial_cor);
	CHECK_RC(rc);

	if (action == ACTION_SCAN)
		return scan_band();
	else if (action == ACTION_BCCH || action == ACTION_BCCH_CCCH)
		return bcch_follow();
	else {
		if (source == SuperFemto_ClkSrcId_NetList) 
			return calib_clock_netlisten();
		return calib_clock();
	}

	return EXIT_SUCCESS;
}
