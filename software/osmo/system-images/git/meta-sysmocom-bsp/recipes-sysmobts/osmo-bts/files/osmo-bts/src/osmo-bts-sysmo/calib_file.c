/* sysmocom femtobts L1 calibration file routines*/

/* (C) 2012 by Harald Welte <laforge@gnumonks.org>
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
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#include <osmocom/core/utils.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1const.h>

#include "l1_if.h"
#include "femtobts.h"
#include "eeprom.h"
#include "utils.h"

struct calib_file_desc {
	const char *fname;
	GsmL1_FreqBand_t band;
	int uplink;
	int rx;
};

static const struct calib_file_desc calib_files[] = {
	{
		.fname = "calib_rxu_850.cfg",
		.band = GsmL1_FreqBand_850,
		.uplink = 1,
		.rx = 1,
	}, {
		.fname = "calib_rxu_900.cfg",
		.band = GsmL1_FreqBand_900,
		.uplink = 1,
		.rx = 1,
	}, {
		.fname = "calib_rxu_1800.cfg",
		.band = GsmL1_FreqBand_1800,
		.uplink = 1,
		.rx = 1,
	}, {
		.fname = "calib_rxu_1900.cfg",
		.band = GsmL1_FreqBand_1900,
		.uplink = 1,
		.rx = 1,
	}, {
		.fname = "calib_rxd_850.cfg",
		.band = GsmL1_FreqBand_850,
		.uplink = 0,
		.rx = 1,
	}, {
		.fname = "calib_rxd_900.cfg",
		.band = GsmL1_FreqBand_900,
		.uplink = 0,
		.rx = 1,
	}, {
		.fname = "calib_rxd_1800.cfg",
		.band = GsmL1_FreqBand_1800,
		.uplink = 0,
		.rx = 1,
	}, {
		.fname = "calib_rxd_1900.cfg",
		.band = GsmL1_FreqBand_1900,
		.uplink = 0,
		.rx = 1,
	}, {
		.fname = "calib_tx_850.cfg",
		.band = GsmL1_FreqBand_850,
		.uplink = 0,
		.rx = 0,
	}, {
		.fname = "calib_tx_900.cfg",
		.band = GsmL1_FreqBand_900,
		.uplink = 0,
		.rx = 0,
	}, {
		.fname = "calib_tx_1800.cfg",
		.band = GsmL1_FreqBand_1800,
		.uplink = 0,
		.rx = 0,
	}, {
		.fname = "calib_tx_1900.cfg",
		.band = GsmL1_FreqBand_1900,
		.uplink = 0,
		.rx = 0,

	},
};

#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,4,0)
static const unsigned int arrsize_by_band[] = {
	[GsmL1_FreqBand_850] = 124,
	[GsmL1_FreqBand_900] = 194,
	[GsmL1_FreqBand_1800] = 374,
	[GsmL1_FreqBand_1900] = 299
};

static float read_float(FILE *in)
{
	int rc;
	float f = 0.0f;

	rc = fscanf(in, "%f\n", &f);
	if (rc != 1)
		LOGP(DL1C, LOGL_ERROR,
			"Reading a float from calib data failed.\n");
	return f;
}

static int read_int(FILE *in)
{
	int rc;
	int i = 0;

	rc = fscanf(in, "%d\n", &i);
	if (rc != 1)
		LOGP(DL1C, LOGL_ERROR,
			"Reading an int from calib data failed.\n");
	return i;
}

/* some particular units have calibration data that is incompatible with
 * firmware >= 3.3, so we need to alter it as follows: */
static const float delta_by_band[Num_GsmL1_FreqBand] = {
	[GsmL1_FreqBand_850] = -2.5f,
	[GsmL1_FreqBand_900] = -2.0f,
	[GsmL1_FreqBand_1800] = -8.0f,
	[GsmL1_FreqBand_1900] = -12.0f,
};

extern const uint8_t fixup_macs[95][6];

static void determine_fixup(struct femtol1_hdl *fl1h)
{
	uint8_t macaddr[6];
	int rc, i;

	rc = eeprom_ReadEthAddr(macaddr);
	if (rc != EEPROM_SUCCESS) {
		LOGP(DL1C, LOGL_ERROR,
		"Unable to read Ethenet MAC from EEPROM\n");
		return;
	}

	/* assume no fixup is needed */
	fl1h->fixup_needed = FIXUP_NOT_NEEDED;

	if (fl1h->hw_info.dsp_version[0] < 3 ||
	    (fl1h->hw_info.dsp_version[0] == 3 &&
	     fl1h->hw_info.dsp_version[1] < 3)) {
		LOGP(DL1C, LOGL_NOTICE, "No calibration table fix-up needed, "
			"firmware < 3.3\n");
		return;
	}

	for (i = 0; i < sizeof(fixup_macs)/6; i++) {
		if (!memcmp(fixup_macs[i], macaddr, 6)) {
			fl1h->fixup_needed = FIXUP_NEEDED;
			break;
		}
	}

	LOGP(DL1C, LOGL_NOTICE, "MAC Address is %02x:%02x:%02x:%02x:%02x:%02x -> %s\n",
		macaddr[0], macaddr[1], macaddr[2], macaddr[3],
		macaddr[4], macaddr[5],
		fl1h->fixup_needed == FIXUP_NEEDED ? "FIXUP" : "NO FIXUP");
}

static int fixup_needed(struct femtol1_hdl *fl1h)
{
	if (fl1h->fixup_needed == FIXUP_UNITILIAZED)
		determine_fixup(fl1h);

	return fl1h->fixup_needed == FIXUP_NEEDED;
}
#endif /* API 2.4.0 */

static void calib_fixup_rx(struct femtol1_hdl *fl1h, SuperFemto_Prim_t *prim)
{
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,4,0)
	SuperFemto_SetRxCalibTblReq_t *rx = &prim->u.setRxCalibTblReq;

	if (fixup_needed(fl1h))
		rx->fExtRxGain += delta_by_band[rx->freqBand];
#endif
}

static int calib_file_read(const char *path, const struct calib_file_desc *desc,
		    SuperFemto_Prim_t *prim)
{
	FILE *in;
	char fname[PATH_MAX];

	fname[0] = '\0';
	snprintf(fname, sizeof(fname)-1, "%s/%s", path, desc->fname);
	fname[sizeof(fname)-1] = '\0';

	in = fopen(fname, "r");
	if (!in) {
		LOGP(DL1C, LOGL_ERROR,
			"Failed to open '%s' for calibration data.\n", fname);
		return -1;
	}

#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,4,0)
	int i;
	if (desc->rx) {
		SuperFemto_SetRxCalibTblReq_t *rx = &prim->u.setRxCalibTblReq;
		memset(rx, 0, sizeof(*rx));

		prim->id = SuperFemto_PrimId_SetRxCalibTblReq;

		rx->freqBand = desc->band;
		rx->bUplink = desc->uplink;

		rx->fExtRxGain = read_float(in);
		rx->fRxMixGainCorr = read_float(in);

		for (i = 0; i < ARRAY_SIZE(rx->fRxLnaGainCorr); i++)
			rx->fRxLnaGainCorr[i] = read_float(in);

		for (i = 0; i < arrsize_by_band[desc->band]; i++)
			rx->fRxRollOffCorr[i] = read_float(in);

		if (desc->uplink) {
			rx->u8IqImbalMode = read_int(in);
			printf("%s: u8IqImbalMode=%d\n", desc->fname, rx->u8IqImbalMode);

			for (i = 0; i < ARRAY_SIZE(rx->u16IqImbalCorr); i++)
				rx->u16IqImbalCorr[i] = read_int(in);
		}
	} else {
		SuperFemto_SetTxCalibTblReq_t *tx = &prim->u.setTxCalibTblReq;
		memset(tx, 0, sizeof(*tx));

		prim->id = SuperFemto_PrimId_SetTxCalibTblReq;

		tx->freqBand = desc->band;

		for (i = 0; i < ARRAY_SIZE(tx->fTxGainGmsk); i++)
			tx->fTxGainGmsk[i] = read_float(in);

		tx->fTx8PskCorr = read_float(in);

		for (i = 0; i < ARRAY_SIZE(tx->fTxExtAttCorr); i++)
			tx->fTxExtAttCorr[i] = read_float(in);

		for (i = 0; i < arrsize_by_band[desc->band]; i++)
			tx->fTxRollOffCorr[i] = read_float(in);
	}
#else
#warning Format of calibration tables before API version 2.4.0 not supported
#endif
	fclose(in);

	return 0;
}

static int calib_eeprom_read(const struct calib_file_desc *desc, SuperFemto_Prim_t *prim)
{
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,4,0)
	eeprom_Error_t eerr;
	int i;
	if (desc->rx) {
		SuperFemto_SetRxCalibTblReq_t *rx = &prim->u.setRxCalibTblReq;
		eeprom_RxCal_t rx_cal;

		memset(rx, 0, sizeof(*rx));

		prim->id = SuperFemto_PrimId_SetRxCalibTblReq;

		rx->freqBand = desc->band;
		rx->bUplink = desc->uplink;

		eerr = eeprom_ReadRxCal(desc->band, desc->uplink, &rx_cal);
		if (eerr != EEPROM_SUCCESS) {
			LOGP(DL1C, LOGL_ERROR, "Error reading RxCalibration "
			     "from EEPROM, band=%d, ul=%d, err=%d\n",
			     desc->band, desc->uplink, eerr);
			return -EIO;
		}

		rx->fExtRxGain = rx_cal.fExtRxGain;
		rx->fRxMixGainCorr = rx_cal.fRxMixGainCorr;

		for (i = 0; i < ARRAY_SIZE(rx->fRxLnaGainCorr); i++)
			rx->fRxLnaGainCorr[i] = rx_cal.fRxLnaGainCorr[i];

		for (i = 0; i < arrsize_by_band[desc->band]; i++)
			rx->fRxRollOffCorr[i] = rx_cal.fRxRollOffCorr[i];

		if (desc->uplink) {
			rx->u8IqImbalMode = rx_cal.u8IqImbalMode;

			for (i = 0; i < ARRAY_SIZE(rx->u16IqImbalCorr); i++)
				rx->u16IqImbalCorr[i] = rx_cal.u16IqImbalCorr[i];
		}
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(5,1,0)
		rx->u8DspMajVer = rx_cal.u8DspMajVer;
		rx->u8DspMinVer = rx_cal.u8DspMinVer;
		rx->u8FpgaMajVer = rx_cal.u8FpgaMajVer;
		rx->u8FpgaMinVer = rx_cal.u8FpgaMinVer;
#endif
	} else {
		SuperFemto_SetTxCalibTblReq_t *tx = &prim->u.setTxCalibTblReq;
		eeprom_TxCal_t tx_cal;

		memset(tx, 0, sizeof(*tx));

		prim->id = SuperFemto_PrimId_SetTxCalibTblReq;
		tx->freqBand = desc->band;

		eerr = eeprom_ReadTxCal(desc->band, &tx_cal);
		if (eerr != EEPROM_SUCCESS) {
			LOGP(DL1C, LOGL_ERROR, "Error reading TxCalibration "
			     "from EEPROM, band=%d, err=%d\n",
			     desc->band, eerr);
			return -EIO;
		}

		for (i = 0; i < ARRAY_SIZE(tx->fTxGainGmsk); i++)
			tx->fTxGainGmsk[i] = tx_cal.fTxGainGmsk[i];

		tx->fTx8PskCorr = tx_cal.fTx8PskCorr;

		for (i = 0; i < ARRAY_SIZE(tx->fTxExtAttCorr); i++)
			tx->fTxExtAttCorr[i] = tx_cal.fTxExtAttCorr[i];

		for (i = 0; i < arrsize_by_band[desc->band]; i++)
			tx->fTxRollOffCorr[i] = tx_cal.fTxRollOffCorr[i];
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(5,1,0)
		tx->u8DspMajVer = tx_cal.u8DspMajVer;
		tx->u8DspMinVer = tx_cal.u8DspMinVer;
		tx->u8FpgaMajVer = tx_cal.u8FpgaMajVer;
		tx->u8FpgaMinVer = tx_cal.u8FpgaMinVer;
#endif
	}
#endif

	return 0;
}

/* determine next calibration file index based on supported bands */
static int next_calib_file_idx(uint32_t band_mask, int last_idx)
{
	int i;

	for (i = last_idx+1; i < ARRAY_SIZE(calib_files); i++) {
		int band = band_femto2osmo(calib_files[i].band);
		if (band < 0)
			continue;
		if (band_mask & band)
			return i;
	}
	return -1;
}

/* iteratively download the calibration data into the L1 */

static int calib_send_compl_cb(struct gsm_bts_trx *trx, struct msgb *l1_msg,
			       void *data);

/* send the calibration table for a single specified file */
static int calib_file_send(struct femtol1_hdl *fl1h,
			   const struct calib_file_desc *desc)
{
	struct calib_send_state *st = &fl1h->st;
	struct msgb *msg;
	char *calib_path = fl1h->phy_inst->u.sysmobts.calib_path;
	int rc;

	msg = sysp_msgb_alloc();

	if (calib_path)
		rc = calib_file_read(calib_path, desc, msgb_sysprim(msg));
	else
		rc = calib_eeprom_read(desc, msgb_sysprim(msg));
	if (rc < 0) {
		msgb_free(msg);

		/* still, we'd like to continue trying to load
		 * calibration for all other bands */
		st->last_file_idx = next_calib_file_idx(fl1h->hw_info.band_support,
						st->last_file_idx);
		if (st->last_file_idx >= 0)
			return calib_file_send(fl1h,
					       &calib_files[st->last_file_idx]);
		else
			return rc;
	}
	calib_fixup_rx(fl1h, msgb_sysprim(msg));

	return l1if_req_compl(fl1h, msg, calib_send_compl_cb, NULL);
}

/* completion callback after every SetCalibTbl is confirmed */
static int calib_send_compl_cb(struct gsm_bts_trx *trx, struct msgb *l1_msg,
				void *data)
{
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);
	struct calib_send_state *st = &fl1h->st;
	char *calib_path = fl1h->phy_inst->u.sysmobts.calib_path;

	LOGP(DL1C, LOGL_NOTICE, "L1 calibration table %s loaded (src: %s)\n",
		calib_files[st->last_file_idx].fname,
		calib_path ? "file" : "eeprom");

	msgb_free(l1_msg);

	st->last_file_idx = next_calib_file_idx(fl1h->hw_info.band_support,
						st->last_file_idx);
	if (st->last_file_idx >= 0)
		return calib_file_send(fl1h,
				       &calib_files[st->last_file_idx]);

	LOGP(DL1C, LOGL_INFO, "L1 calibration table loading complete!\n");
	eeprom_free_resources();

	return 0;
}


int calib_load(struct femtol1_hdl *fl1h)
{
#if SUPERFEMTO_API_VERSION < SUPERFEMTO_API(2,4,0)
	LOGP(DL1C, LOGL_ERROR, "L1 calibration is not supported on pre 2.4.0 firmware.\n");
	return -1;
#else
	int idx = next_calib_file_idx(fl1h->hw_info.band_support, -1);
	if (idx < 0) {
		LOGP(DL1C, LOGL_ERROR, "No band_support?!?\n");
		return -1;
	}
	return calib_file_send(fl1h, &calib_files[idx]);
#endif
}


#if 0
int main(int argc, char **argv)
{
	SuperFemto_Prim_t p;
	int i;

	for (i = 0; i < ARRAY_SIZE(calib_files); i++) {
		memset(&p, 0, sizeof(p));
		calib_read_file(argv[1], &calib_files[i], &p);
	}
	exit(0);
}
#endif
