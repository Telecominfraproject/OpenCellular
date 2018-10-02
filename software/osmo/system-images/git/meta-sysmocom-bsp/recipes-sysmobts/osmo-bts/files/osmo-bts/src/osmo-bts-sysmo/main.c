/* Main program for Sysmocom BTS */

/* (C) 2011-2015 by Harald Welte <laforge@gnumonks.org>
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
#include <osmocom/vty/ports.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/l1sap.h>

#define SYSMOBTS_RF_LOCK_PATH	"/var/lock/bts_rf_lock"

#include "utils.h"
#include "eeprom.h"
#include "l1_if.h"
#include "hw_misc.h"
#include "oml_router.h"

int bts_model_init(struct gsm_bts *bts)
{
	struct stat st;
	static struct osmo_fd accept_fd, read_fd;
	int rc;

	bts->variant = BTS_OSMO_SYSMO;
	bts->support.ciphers = CIPHER_A5(1) | CIPHER_A5(2) | CIPHER_A5(3);

	rc = oml_router_init(bts, OML_ROUTER_PATH, &accept_fd, &read_fd);
	if (rc < 0) {
		fprintf(stderr, "Error creating the OML router: %s rc=%d\n",
			OML_ROUTER_PATH, rc);
		exit(1);
	}

	if (stat(SYSMOBTS_RF_LOCK_PATH, &st) == 0) {
		LOGP(DL1C, LOGL_NOTICE, "Not starting BTS due to RF_LOCK file present\n");
		exit(23);
	}

	gsm_bts_set_feature(bts, BTS_FEAT_GPRS);
	gsm_bts_set_feature(bts, BTS_FEAT_EGPRS);
	gsm_bts_set_feature(bts, BTS_FEAT_OML_ALERTS);
	gsm_bts_set_feature(bts, BTS_FEAT_AGCH_PCH_PROP);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_V1);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_H_V1);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_EFR);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_AMR);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_H_AMR);

	bts_model_vty_init(bts);

	return 0;
}

int bts_model_oml_estab(struct gsm_bts *bts)
{
	return 0;
}

void bts_update_status(enum bts_global_status which, int on)
{
	static uint64_t states = 0;
	uint64_t old_states = states;
	int led_rf_active_on;

	if (on)
		states |= (1ULL << which);
	else
		states &= ~(1ULL << which);

	led_rf_active_on =
		(states & (1ULL << BTS_STATUS_RF_ACTIVE)) &&
		!(states & (1ULL << BTS_STATUS_RF_MUTE));

	LOGP(DL1C, LOGL_INFO,
	     "Set global status #%d to %d (%04llx -> %04llx), LEDs: ACT %d\n",
	     which, on,
	     (long long)old_states, (long long)states,
	     led_rf_active_on);

	sysmobts_led_set(LED_RF_ACTIVE, led_rf_active_on);
}

void bts_model_print_help()
{
	printf(
		"  -w	--hw-version	Print the targeted HW Version\n"
		"  -M	--pcu-direct	Force PCU to access message queue for "
			"PDCH dchannel directly\n"
	      );
};

static void print_hwversion()
{
#ifdef HW_SYSMOBTS_V1
	printf("sysmobts was compiled for hw version 1.\n");
#else
	printf("sysmobts was compiled for hw version 2.\n");
#endif
}

int bts_model_handle_options(int argc, char **argv)
{
	int num_errors = 0;

	while (1) {
		int option_idx = 0, c;
		static const struct option long_options[] = {
			/* specific to this hardware */
			{ "hw-version", 0, 0, 'w' },
			{ "pcu-direct", 0, 0, 'M' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "wM",
				long_options, &option_idx);
		if (c == -1)
			break;

		switch (c) {
		case 'M':
			pcu_direct = 1;
			break;
		case 'w':
			print_hwversion();
			exit(0);
			break;
		default:
			num_errors++;
			break;
		}
	}

	return num_errors;
}

void bts_model_phy_link_set_defaults(struct phy_link *plink)
{
}

void bts_model_phy_instance_set_defaults(struct phy_instance *pinst)
{
	pinst->u.sysmobts.clk_use_eeprom = 1;
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
