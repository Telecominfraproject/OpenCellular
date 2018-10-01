/* NuRAN Wireless OC-2G BTS L1 calibration file routines*/

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * Copyright (C) 2016 by Harald Welte <laforge@gnumonks.org>
 * 
 * Based on sysmoBTS:
 *     (C) 2012 by Harald Welte <laforge@gnumonks.org>
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

#include <nrw/oc2g/oc2g.h>
#include <nrw/oc2g/gsml1const.h>

#include "l1_if.h"
#include "oc2gbts.h"
#include "utils.h"
#include "osmo-bts/oml.h"

/* Maximum calibration data chunk size */
#define MAX_CALIB_TBL_SIZE  65536
/* Calibration header version */
#define CALIB_HDR_V1  0x01

struct calib_file_desc {
	const char *fname;
	int rx;
	int trx;
	int rxpath;
};

static const struct calib_file_desc calib_files[] = {
	{
		.fname = "calib_rx0.conf",
		.rx = 1,
		.trx = 0,
		.rxpath = 0,
	}, {
		.fname = "calib_tx0.conf",
		.rx = 0,
		.trx = 0,
	},
};

struct calTbl_t
{
    union
    {
        struct
        {
            uint8_t u8Version;                /* Header version (1) */
            uint8_t u8Parity;                 /* Parity byte (xor) */
            uint8_t u8Type;                   /* Table type (0:TX Downlink, 1:RX-A Uplink, 2:RX-B Uplink) */
            uint8_t u8Band;                   /* GSM Band (0:GSM-850, 1:EGSM-900, 2:DCS-1800, 3:PCS-1900) */
            uint32_t u32Len;                  /* Table length in bytes including the header */
            struct
            {
                uint32_t u32DescOfst;         /* Description section offset */
                uint32_t u32DateOfst;         /* Date section offset */
                uint32_t u32StationOfst;      /* Calibration test station section offset */
                uint32_t u32FpgaFwVerOfst;    /* Calibration FPGA firmware version section offset */
                uint32_t u32DspFwVerOfst;     /* Calibration DSP firmware section offset */
                uint32_t u32DataOfst;         /* Calibration data section offset */
            } toc;
        } v1;
    } hdr;

    uint8_t u8RawData[MAX_CALIB_TBL_SIZE - 32];
};


static int calib_file_send(struct oc2gl1_hdl *fl1h,
			   const struct calib_file_desc *desc);
static int calib_verify(struct oc2gl1_hdl *fl1h,
			const struct calib_file_desc *desc);

/* determine next calibration file index based on supported bands */
static int get_next_calib_file_idx(struct oc2gl1_hdl *fl1h, int last_idx)
{
	struct phy_link *plink = fl1h->phy_inst->phy_link;
        int i;

        for (i = last_idx+1; i < ARRAY_SIZE(calib_files); i++) {
                if (calib_files[i].trx == plink->num)
                        return i;
        }
        return -1;
}

static int calib_file_open(struct oc2gl1_hdl *fl1h,
                           const struct calib_file_desc *desc)
{
	struct calib_send_state *st = &fl1h->st;
	char *calib_path = fl1h->phy_inst->u.oc2g.calib_path;
	char fname[PATH_MAX];

	if (st->fp) {
		LOGP(DL1C, LOGL_NOTICE, "L1 calibration file was left opened !!\n");
		fclose(st->fp);
		st->fp = NULL;
	}

	fname[0] = '\0';
	snprintf(fname, sizeof(fname)-1, "%s/%s", calib_path, desc->fname);
	fname[sizeof(fname)-1] = '\0';

	st->fp = fopen(fname, "rb");
	if (!st->fp) {
		LOGP(DL1C, LOGL_NOTICE, "Failed to open '%s' for calibration data.\n", fname);

		/*if( fl1h->phy_inst->trx ){
			fl1h->phy_inst->trx->mo.obj_inst.trx_nr = fl1h->phy_inst->trx->nr;

			alarm_sig_data.mo = &fl1h->phy_inst->trx->mo;
			alarm_sig_data.add_text = (char*)&fname[0];
			osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_FAIL_OPEN_CALIB_ALARM, &alarm_sig_data);
		} */
		return -1;
	}
	return 0;
}

static int calib_file_close(struct oc2gl1_hdl *fl1h)
{
	struct calib_send_state *st = &fl1h->st;

	if (st->fp) {
		fclose(st->fp);
		st->fp = NULL;
	}
	return 0;
}

/* iteratively download the calibration data into the L1 */

static int calib_send_compl_cb(struct gsm_bts_trx *trx, struct msgb *l1_msg,
			       void *data);

/* send a chunk of calibration tabledata for a single specified file */
static int calib_file_send_next_chunk(struct oc2gl1_hdl *fl1h)
{
	struct calib_send_state *st = &fl1h->st;
	Oc2g_Prim_t *prim;
	struct msgb *msg;
	size_t n;

	msg = sysp_msgb_alloc();
	prim = msgb_sysprim(msg);

	prim->id = Oc2g_PrimId_SetCalibTblReq;
	prim->u.setCalibTblReq.offset = (uint32_t)ftell(st->fp);
	n = fread(prim->u.setCalibTblReq.u8Data, 1, 
			sizeof(prim->u.setCalibTblReq.u8Data), st->fp);
	prim->u.setCalibTblReq.length = n;


	if (n == 0) {
		/* The table data has been completely sent and acknowledged */
                LOGP(DL1C, LOGL_NOTICE, "L1 calibration table %s loaded\n",
                        calib_files[st->last_file_idx].fname);

                calib_file_close(fl1h);

                msgb_free(msg);

                /* Send the next one if any */
                st->last_file_idx = get_next_calib_file_idx(fl1h, st->last_file_idx);
                if (st->last_file_idx >= 0) {
                        return calib_file_send(fl1h,
                                       &calib_files[st->last_file_idx]);
                }

                LOGP(DL1C, LOGL_INFO, "L1 calibration table loading complete!\n");
                return 0;
	}

	return l1if_req_compl(fl1h, msg, calib_send_compl_cb, NULL);
}

/* send the calibration table for a single specified file */
static int calib_file_send(struct oc2gl1_hdl *fl1h,
			   const struct calib_file_desc *desc)
{
	struct calib_send_state *st = &fl1h->st;
	int rc;

	rc = calib_file_open(fl1h, desc);
	if (rc < 0) {
		/* still, we'd like to continue trying to load
		 * calibration for all other bands */
		st->last_file_idx = get_next_calib_file_idx(fl1h, st->last_file_idx);
		if (st->last_file_idx >= 0)
			return calib_file_send(fl1h,
					&calib_files[st->last_file_idx]);

		LOGP(DL1C, LOGL_INFO, "L1 calibration table loading complete!\n");
		return 0;
	}

	rc = calib_verify(fl1h, desc);
	if (rc < 0) {
		LOGP(DL1C, LOGL_NOTICE,"Verify L1 calibration table %s -> failed (%d)\n", desc->fname, rc);

		/*
		if (fl1h->phy_inst->trx) {
			fl1h->phy_inst->trx->mo.obj_inst.trx_nr = fl1h->phy_inst->trx->nr;

			alarm_sig_data.mo = &fl1h->phy_inst->trx->mo;
			alarm_sig_data.add_text = (char*)&desc->fname[0];
			memcpy(alarm_sig_data.spare, &rc, sizeof(int));
			osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_FAIL_VERIFY_CALIB_ALARM, &alarm_sig_data);
		} */

		st->last_file_idx = get_next_calib_file_idx(fl1h, st->last_file_idx);

		if (st->last_file_idx >= 0)
			return calib_file_send(fl1h,
				&calib_files[st->last_file_idx]);
		return 0;

	}

	LOGP(DL1C, LOGL_INFO, "Verify L1 calibration table %s -> done\n", desc->fname);

	return calib_file_send_next_chunk(fl1h);
}

/* completion callback after every SetCalibTbl is confirmed */
static int calib_send_compl_cb(struct gsm_bts_trx *trx, struct msgb *l1_msg,
				void *data)
{
	struct oc2gl1_hdl *fl1h = trx_oc2gl1_hdl(trx);
	struct calib_send_state *st = &fl1h->st;
	Oc2g_Prim_t *prim = msgb_sysprim(l1_msg);

	if (prim->u.setCalibTblCnf.status != GsmL1_Status_Success) {
		LOGP(DL1C, LOGL_ERROR, "L1 rejected calibration table\n");

		msgb_free(l1_msg);

		calib_file_close(fl1h);

		/* Skip this one and try the next one */
		st->last_file_idx = get_next_calib_file_idx(fl1h, st->last_file_idx);
		if (st->last_file_idx >= 0) {
			return calib_file_send(fl1h,
					&calib_files[st->last_file_idx]);
		}

		LOGP(DL1C, LOGL_INFO, "L1 calibration table loading complete!\n");
		return 0;
	}

	msgb_free(l1_msg);

	/* Keep sending the calibration file data */
	return calib_file_send_next_chunk(fl1h);
}

int calib_load(struct oc2gl1_hdl *fl1h)
{
	int rc;
	struct calib_send_state *st = &fl1h->st;
	char *calib_path = fl1h->phy_inst->u.oc2g.calib_path;

	if (!calib_path) {
		LOGP(DL1C, LOGL_NOTICE, "Calibration file path not specified\n");

		/*if( fl1h->phy_inst->trx ){
			fl1h->phy_inst->trx->mo.obj_inst.trx_nr = fl1h->phy_inst->trx->nr;

			alarm_sig_data.mo = &fl1h->phy_inst->trx->mo;
			osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_NO_CALIB_PATH_ALARM, &alarm_sig_data);
		}*/
		return -1;
	}

	rc = get_next_calib_file_idx(fl1h, -1);
	if (rc < 0) {
		return -1;
	}
	st->last_file_idx = rc;

	return calib_file_send(fl1h, &calib_files[st->last_file_idx]);
}


static int calib_verify(struct oc2gl1_hdl *fl1h, const struct calib_file_desc *desc)
{
	int rc, sz;
	struct calib_send_state *st = &fl1h->st;
	struct phy_link *plink = fl1h->phy_inst->phy_link;
	char *rbuf;
	struct calTbl_t *calTbl;
	char calChkSum ;


	/* calculate file size in bytes */
	fseek(st->fp, 0L, SEEK_END);
	sz = ftell(st->fp);

	/* rewind read poiner */
	fseek(st->fp, 0L, SEEK_SET);

	/* read file */
	rbuf = (char *) malloc( sizeof(char) * sz );

	rc = fread(rbuf, 1, sizeof(char) * sz, st->fp);
	if (rc != sz) {
		LOGP(DL1C, LOGL_ERROR, "%s reading error\n", desc->fname);
		free(rbuf);

		/* close file */
		rc = calib_file_close(fl1h);
		if (rc < 0 ) {
			LOGP(DL1C, LOGL_ERROR, "%s can not close\n", desc->fname);
			return rc;
		}

		return -2;
	}


	calTbl = (struct calTbl_t*) rbuf;
	/* calculate file checksum */
	calChkSum = 0;
	while (sz--) {
		calChkSum ^= rbuf[sz];
	}

	/* validate Tx calibration parity */
	if (calChkSum) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid checksum %x.\n", desc->fname, calChkSum);
		return -4;
	}

	/* validate Tx calibration header */
	if (calTbl->hdr.v1.u8Version != CALIB_HDR_V1) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid header version %u.\n", desc->fname, calTbl->hdr.v1.u8Version);
		return -5;
	}

	/* validate calibration description */
	if (calTbl->hdr.v1.toc.u32DescOfst == 0xFFFFFFFF) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid calibration description  offset.\n", desc->fname);
		return -6;
	}

	/* validate calibration date */
	if (calTbl->hdr.v1.toc.u32DateOfst == 0xFFFFFFFF) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid calibration date offset.\n", desc->fname);
		return -7;
	}

	LOGP(DL1C, LOGL_INFO, "L1 calibration table %s created on %s\n",
		desc->fname,
		calTbl->u8RawData + calTbl->hdr.v1.toc.u32DateOfst);

	/* validate calibration station */
	if (calTbl->hdr.v1.toc.u32StationOfst == 0xFFFFFFFF) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid calibration station ID offset.\n", desc->fname);
		return -8;
	}

	/* validate FPGA FW version */
	if (calTbl->hdr.v1.toc.u32FpgaFwVerOfst == 0xFFF) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid FPGA FW version offset.\n", desc->fname);
		return -9;
	}

	/* validate DSP FW version */
	if (calTbl->hdr.v1.toc.u32DspFwVerOfst == 0xFFFFFFFF) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid DSP FW version offset.\n", desc->fname);
		return -10;
	}

	/* validate Tx calibration data offset */
	if (calTbl->hdr.v1.toc.u32DataOfst == 0xFFFFFFFF) {
		LOGP(DL1C, LOGL_ERROR, "%s has invalid calibration data offset.\n", desc->fname);
		return -11;
	}

	if (!desc->rx) {

		/* parse min/max Tx power */
		fl1h->phy_inst->u.oc2g.minTxPower = calTbl->u8RawData[calTbl->hdr.v1.toc.u32DataOfst + (5 << 2)];
		fl1h->phy_inst->u.oc2g.maxTxPower = calTbl->u8RawData[calTbl->hdr.v1.toc.u32DataOfst + (6 << 2)];

		/* override nominal Tx power of given TRX if needed */
		if (fl1h->phy_inst->trx->nominal_power > fl1h->phy_inst->u.oc2g.maxTxPower) {
			LOGP(DL1C, LOGL_INFO, "Set TRX %u nominal Tx power to %d dBm (%d)\n",
				plink->num,
				fl1h->phy_inst->u.oc2g.maxTxPower,
				fl1h->phy_inst->trx->nominal_power);

			fl1h->phy_inst->trx->nominal_power = fl1h->phy_inst->u.oc2g.maxTxPower;
		}

		if (fl1h->phy_inst->trx->nominal_power < fl1h->phy_inst->u.oc2g.minTxPower) {
			LOGP(DL1C, LOGL_INFO, "Set TRX %u nominal Tx power to %d dBm (%d)\n",
				plink->num,
				fl1h->phy_inst->u.oc2g.minTxPower,
				fl1h->phy_inst->trx->nominal_power);

			fl1h->phy_inst->trx->nominal_power = fl1h->phy_inst->u.lc15.minTxPower;
		}

		if (fl1h->phy_inst->trx->power_params.trx_p_max_out_mdBm > to_mdB(fl1h->phy_inst->u.oc2g.maxTxPower) ) {
			LOGP(DL1C, LOGL_INFO, "Set TRX %u Tx power parameter to %d dBm (%d)\n",
				plink->num,
				to_mdB(fl1h->phy_inst->u.oc2g.maxTxPower),
				fl1h->phy_inst->trx->power_params.trx_p_max_out_mdBm);

			fl1h->phy_inst->trx->power_params.trx_p_max_out_mdBm = to_mdB(fl1h->phy_inst->u.oc2g.maxTxPower);
		}

		if (fl1h->phy_inst->trx->power_params.trx_p_max_out_mdBm < to_mdB(fl1h->phy_inst->u.oc2g.minTxPower) ) {
			LOGP(DL1C, LOGL_INFO, "Set TRX %u Tx power parameter to %d dBm (%d)\n",
				plink->num,
				to_mdB(fl1h->phy_inst->u.oc2g.minTxPower),
				fl1h->phy_inst->trx->power_params.trx_p_max_out_mdBm);

			fl1h->phy_inst->trx->power_params.trx_p_max_out_mdBm = to_mdB(fl1h->phy_inst->u.oc2g.minTxPower);
		}

		LOGP(DL1C, LOGL_DEBUG, "%s: minTxPower=%d, maxTxPower=%d\n",
			desc->fname,
			fl1h->phy_inst->u.oc2g.minTxPower,
			fl1h->phy_inst->u.oc2g.maxTxPower );
	}

	/* rewind read pointer for subsequence tasks */
	fseek(st->fp, 0L, SEEK_SET);
	free(rbuf);

	return 0;
}

