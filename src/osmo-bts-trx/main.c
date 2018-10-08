/* Main program for OsmoBTS-TRX */

/* (C) 2011-2015 by Harald Welte <laforge@gnumonks.org>
 * (C) 2013 by Andreas Eversberg <jolly@eversberg.eu>
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
#include <sched.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>
#include <osmocom/vty/ports.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/bits.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/control_if.h>
#include <osmo-bts/scheduler.h>

#include "l1_if.h"
#include "trx_if.h"

/* dummy, since no direct dsp support */
uint32_t trx_get_hlayer1(struct gsm_bts_trx *trx)
{
	return 0;
}

void bts_model_print_help()
{
}

int bts_model_handle_options(int argc, char **argv)
{
	int num_errors = 0;

	while (1) {
		int option_idx = 0, c;
		static const struct option long_options[] = {
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "",
				long_options, &option_idx);

		if (c == -1)
			break;

		switch (c) {
		default:
			num_errors++;
			break;
		}
	}

	return num_errors;
}

int bts_model_init(struct gsm_bts *bts)
{
	bts->variant = BTS_OSMO_TRX;
	bts->support.ciphers = CIPHER_A5(1) | CIPHER_A5(2) | CIPHER_A5(3);

	/* FIXME: this needs to be overridden with the real hardrware
	 * value */
	bts->c0->nominal_power = 23;

	gsm_bts_set_feature(bts, BTS_FEAT_GPRS);
	gsm_bts_set_feature(bts, BTS_FEAT_OML_ALERTS);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_V1);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_H_V1);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_EFR);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_AMR);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_H_AMR);
	gsm_bts_set_feature(bts, BTS_FEAT_CBCH);

	bts_model_vty_init(bts);

	return 0;
}

void bts_model_phy_link_set_defaults(struct phy_link *plink)
{
	plink->u.osmotrx.local_ip = talloc_strdup(plink, "127.0.0.1");
	plink->u.osmotrx.remote_ip = talloc_strdup(plink, "127.0.0.1");
	plink->u.osmotrx.base_port_local = 5800;
	plink->u.osmotrx.base_port_remote = 5700;
	plink->u.osmotrx.clock_advance = 20;
	plink->u.osmotrx.rts_advance = 5;
	plink->u.osmotrx.trx_ta_loop = true;
	plink->u.osmotrx.trx_ms_power_loop = false;
	plink->u.osmotrx.trx_target_rssi = -10;
}

void bts_model_phy_instance_set_defaults(struct phy_instance *pinst)
{
	struct trx_l1h *l1h;
	l1h = talloc_zero(tall_bts_ctx, struct trx_l1h);
	l1h->phy_inst = pinst;
	pinst->u.osmotrx.hdl = l1h;

	l1h->config.power_oml = 1;
}

int main(int argc, char **argv)
{
	return bts_main(argc, argv);
}
