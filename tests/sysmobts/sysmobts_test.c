/*
 * (C) 2013,2014 by Holger Hans Peter Freyther
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
 */

#include <osmo-bts/bts.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/power_control.h>

#include "femtobts.h"
#include "l1_if.h"
#include "utils.h"

#include <sysmocom/femtobts/gsml1prim.h>

#include <stdio.h>

static int direct_map[][3] = {
	{ GSM_BAND_850,		GsmL1_FreqBand_850,	128	},
	{ GSM_BAND_900,		GsmL1_FreqBand_900,	1	},
	{ GSM_BAND_1800, 	GsmL1_FreqBand_1800,	600	},
	{ GSM_BAND_1900,	GsmL1_FreqBand_1900,	600	},
};

static int dcs_to_dcs[][3] = {
	{ GSM_BAND_900,		GsmL1_FreqBand_1800,	600	},
	{ GSM_BAND_1800,	GsmL1_FreqBand_900,	1	},
	{ GSM_BAND_900,		-1,			438	},
};

static int pcs_to_pcs[][3] = {
	{ GSM_BAND_850,		GsmL1_FreqBand_1900,	512	},
	{ GSM_BAND_1900,	GsmL1_FreqBand_850,	128	},
	{ GSM_BAND_900,		-1,			438	},
};

static void test_sysmobts_auto_band(void)
{
	struct gsm_bts bts;
	struct gsm_bts_trx trx;
	struct femtol1_hdl hdl;
	int i;

	memset(&bts, 0, sizeof(bts));
	memset(&trx, 0, sizeof(trx));
	memset(&hdl, 0, sizeof(hdl));
	trx.bts = &bts;
	trx.role_bts.l1h = &hdl;

	/* claim to support all hw_info's */
	hdl.hw_info.band_support = GSM_BAND_850 | GSM_BAND_900 |
					GSM_BAND_1800 | GSM_BAND_1900;

	/* start with the current option */
	printf("Testing the no auto-band mapping.\n");
	for (i = 0; i < ARRAY_SIZE(direct_map); ++i) {
		uint16_t arfcn;
		int res;

		bts.auto_band = 0;
		bts.band = direct_map[i][0];
		arfcn = direct_map[i][2];
		res = sysmobts_select_femto_band(&trx, arfcn);
		printf("No auto-band band(%d) arfcn(%u) want(%d) got(%d)\n",
			bts.band, arfcn, direct_map[i][1], res);
		OSMO_ASSERT(res == direct_map[i][1]);
	}

	/* Check if auto-band does not break things */
	printf("Checking the mapping with auto-band.\n");
	for (i = 0; i < ARRAY_SIZE(direct_map); ++i) {
		uint16_t arfcn;
		int res;

		bts.auto_band = 1;
		bts.band = direct_map[i][0];
		arfcn = direct_map[i][2];
		res = sysmobts_select_femto_band(&trx, arfcn);
		printf("Auto-band band(%d) arfcn(%u) want(%d) got(%d)\n",
			bts.band, arfcn, direct_map[i][1], res);
		OSMO_ASSERT(res == direct_map[i][1]);
	}

	/* Check DCS to DCS change */
	printf("Checking DCS to DCS\n");
	for (i = 0; i < ARRAY_SIZE(dcs_to_dcs); ++i) {
		uint16_t arfcn;
		int res;

		bts.auto_band = 1;
		bts.band = dcs_to_dcs[i][0];
		arfcn = dcs_to_dcs[i][2];
		res = sysmobts_select_femto_band(&trx, arfcn);
		printf("DCS to DCS band(%d) arfcn(%u) want(%d) got(%d)\n",
			bts.band, arfcn, dcs_to_dcs[i][1], res);
		OSMO_ASSERT(res == dcs_to_dcs[i][1]);
	}

	/* Check for a PCS to PCS change */
	printf("Checking PCS to PCS\n");
	for (i = 0; i < ARRAY_SIZE(pcs_to_pcs); ++i) {
		uint16_t arfcn;
		int res;

		bts.auto_band = 1;
		bts.band = pcs_to_pcs[i][0];
		arfcn = pcs_to_pcs[i][2];
		res = sysmobts_select_femto_band(&trx, arfcn);
		printf("PCS to PCS band(%d) arfcn(%u) want(%d) got(%d)\n",
			bts.band, arfcn, pcs_to_pcs[i][1], res);
		OSMO_ASSERT(res == pcs_to_pcs[i][1]);
	}
}

static void test_sysmobts_cipher(void)
{
 	static const uint8_t cipher_cmd[] = {
		0x03, 0x00, 0x0d, 0x06, 0x35, 0x11, 0x2b, 0x2b,
		0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
		0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b };
	static const uint8_t too_early_classmark[] = {
		0x01, 0x00, 0x4d, 0x06, 0x16, 0x03, 0x30, 0x18,
		0xa2, 0x20, 0x0b, 0x60, 0x14, 0x4c, 0xa7, 0x7b,
		0x29, 0x11, 0xdc, 0x40, 0x04, 0x00, 0x2b };
	static const uint8_t first_ciphered_cipher_cmpl[] = {
		0x01, 0x30, 0x4d, 0x06, 0x16, 0x03, 0x30, 0x18,
		0xa2, 0x20, 0x0b, 0x60, 0x14, 0x4c, 0xa7, 0x7b,
		0x29, 0x11, 0xdc, 0x40, 0x04, 0x00, 0x2b };

	struct gsm_lchan lchan;
	struct femtol1_hdl fl1h;
	struct msgb *msg;
	GsmL1_MsgUnitParam_t unit;
	int rc;

	memset(&lchan, 0, sizeof(lchan));
	memset(&fl1h, 0, sizeof(fl1h));

	/* Inject the cipher mode command */
	msg = msgb_alloc_headroom(128, 64, "ciphering mode command");
	lchan.ciph_state = LCHAN_CIPH_NONE;
	memcpy(msgb_put(msg, ARRAY_SIZE(cipher_cmd)), cipher_cmd, ARRAY_SIZE(cipher_cmd));
	rc = bts_check_for_ciph_cmd(&fl1h, msg, &lchan);
	OSMO_ASSERT(rc == 1);
	OSMO_ASSERT(lchan.ciph_state == LCHAN_CIPH_RX_REQ);
	OSMO_ASSERT(lchan.ciph_ns == 1);
	msgb_free(msg);

	/* Move to the confirmed state */
	lchan.ciph_state = LCHAN_CIPH_RX_CONF;

	/* Handle message sent before ciphering was received */
	memcpy(&unit.u8Buffer[0], too_early_classmark, ARRAY_SIZE(too_early_classmark));
	unit.u8Size = ARRAY_SIZE(too_early_classmark);
	rc = bts_check_for_first_ciphrd(&lchan, unit.u8Buffer, unit.u8Size);
	OSMO_ASSERT(rc == 0);
	OSMO_ASSERT(lchan.ciph_state == LCHAN_CIPH_RX_CONF);

	/* Now send the first ciphered message */
	memcpy(&unit.u8Buffer[0], first_ciphered_cipher_cmpl, ARRAY_SIZE(first_ciphered_cipher_cmpl));
	unit.u8Size = ARRAY_SIZE(first_ciphered_cipher_cmpl);
	rc = bts_check_for_first_ciphrd(&lchan, unit.u8Buffer, unit.u8Size);
	OSMO_ASSERT(rc == 1);
	/* we cannot test for lchan.ciph_state == * LCHAN_CIPH_RX_CONF_TX_REQ, as
	 * this happens asynchronously on the other side of the l1sap queue */
}

int main(int argc, char **argv)
{
	printf("Testing sysmobts routines\n");
	test_sysmobts_auto_band();
	test_sysmobts_cipher();

	return 0;
}


/*
 * some local stubs. We need to pull in a lot more code and can't
 * use the generic stubs unless we make all of them weak
 */
void bts_update_status(enum bts_global_status which, int on)
{}

int bts_model_init(struct gsm_bts *bts)
{ return 0; }
int bts_model_oml_estab(struct gsm_bts *bts)
{ return 0; }
void bts_model_abis_close(struct gsm_bts *bts)
{ }
void bts_model_phy_link_set_defaults(struct phy_link *plink)
{ }
void bts_model_phy_instance_set_defaults(struct phy_instance *pinst)
{ }
