/* Control Interface for sysmoBTS */

/* (C) 2014 by Harald Welte <laforge@gnumonks.org>
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
#include <errno.h>
#include <fcntl.h>

#include <osmocom/ctrl/control_cmd.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/paging.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/handover.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1prim.h>
#include <sysmocom/femtobts/gsml1const.h>
#include <sysmocom/femtobts/gsml1types.h>

#include "femtobts.h"
#include "l1_if.h"


/* for control interface */

#ifndef HW_SYSMOBTS_V1
CTRL_CMD_DEFINE(clock_info, "clock-info");
static int ctrl_clkinfo_cb(struct gsm_bts_trx *trx, struct msgb *resp,
			   void *data)
{
	SuperFemto_Prim_t *sysp = msgb_sysprim(resp);
	struct ctrl_cmd_def *cd = data;
	struct ctrl_cmd *cmd = cd->cmd;

	LOGP(DL1C, LOGL_NOTICE,
		"RfClockInfo iClkCor=%d/clkSrc=%s Err=%d/ErrRes=%d/clkSrc=%s\n",
		sysp->u.rfClockInfoCnf.rfTrx.iClkCor,
		get_value_string(femtobts_clksrc_names,
				sysp->u.rfClockInfoCnf.rfTrx.clkSrc),
		sysp->u.rfClockInfoCnf.rfTrxClkCal.iClkErr,
		sysp->u.rfClockInfoCnf.rfTrxClkCal.iClkErrRes,
		get_value_string(femtobts_clksrc_names,
				sysp->u.rfClockInfoCnf.rfTrxClkCal.clkSrc));

	if (ctrl_cmd_def_is_zombie(cd)) {
		msgb_free(resp);
		return 0;
	}

	cmd->reply = talloc_asprintf(cmd, "%d,%s,%d,%d,%s",
		sysp->u.rfClockInfoCnf.rfTrx.iClkCor,
		get_value_string(femtobts_clksrc_names,
				sysp->u.rfClockInfoCnf.rfTrx.clkSrc),
		sysp->u.rfClockInfoCnf.rfTrxClkCal.iClkErr,
		sysp->u.rfClockInfoCnf.rfTrxClkCal.iClkErrRes,
		get_value_string(femtobts_clksrc_names,
				sysp->u.rfClockInfoCnf.rfTrxClkCal.clkSrc));

	ctrl_cmd_def_send(cd);

	msgb_free(resp);

	return 0;
}
static int get_clock_info(struct ctrl_cmd *cmd, void *data)
{
	struct gsm_bts_trx *trx = cmd->node;
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);
	struct msgb *msg = sysp_msgb_alloc();
	SuperFemto_Prim_t *sysp = msgb_sysprim(msg);
	struct ctrl_cmd_def *cd;

	/* geneate a deferred control command */
	cd = ctrl_cmd_def_make(fl1h, cmd, NULL, 10);

	sysp->id = SuperFemto_PrimId_RfClockInfoReq;
	sysp->u.rfClockInfoReq.u8RstClkCal = 0;

	return l1if_req_compl(fl1h, msg, ctrl_clkinfo_cb, cd);
}
static int ctrl_set_clkinfo_cb(struct gsm_bts_trx *trx, struct msgb *resp,
				void *data)
{
	struct ctrl_cmd_def *cd = data;
	struct ctrl_cmd *cmd = cd->cmd;

	if (ctrl_cmd_def_is_zombie(cd)) {
		msgb_free(resp);
		return 0;
	}

	cmd->reply = "success";

	ctrl_cmd_def_send(cd);

	msgb_free(resp);

	return 0;
}
static int clock_setup_cb(struct gsm_bts_trx *trx, struct msgb *resp,
			void *data)
{
	msgb_free(resp);
	return 0;
}

static int set_clock_info(struct ctrl_cmd *cmd, void *data)
{
	struct gsm_bts_trx *trx = cmd->node;
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);
	struct msgb *msg = sysp_msgb_alloc();
	SuperFemto_Prim_t *sysp = msgb_sysprim(msg);
	struct ctrl_cmd_def *cd;

	/* geneate a deferred control command */
	cd = ctrl_cmd_def_make(fl1h, cmd, NULL, 10);

	/* Set GPS/PPS as reference */
	sysp->id = SuperFemto_PrimId_RfClockSetupReq;
	sysp->u.rfClockSetupReq.rfTrx.iClkCor = get_clk_cal(fl1h);
	sysp->u.rfClockSetupReq.rfTrx.clkSrc = fl1h->clk_src;
	sysp->u.rfClockSetupReq.rfTrxClkCal.clkSrc = SuperFemto_ClkSrcId_GpsPps;
	l1if_req_compl(fl1h, msg, clock_setup_cb, NULL);

	/* Reset the error counters */
	msg = sysp_msgb_alloc();
	sysp = msgb_sysprim(msg);
	sysp->id = SuperFemto_PrimId_RfClockInfoReq;
	sysp->u.rfClockInfoReq.u8RstClkCal = 1;

	l1if_req_compl(fl1h, msg, ctrl_set_clkinfo_cb, cd);

	return CTRL_CMD_HANDLED;
}

static int verify_clock_info(struct ctrl_cmd *cmd, const char *value, void *data)
{
	return 0;
}


CTRL_CMD_DEFINE(clock_corr, "clock-correction");
static int ctrl_get_clkcorr_cb(struct gsm_bts_trx *trx, struct msgb *resp,
				void *data)
{
	SuperFemto_Prim_t *sysp = msgb_sysprim(resp);
	struct ctrl_cmd_def *cd = data;
	struct ctrl_cmd *cmd = cd->cmd;

	if (ctrl_cmd_def_is_zombie(cd)) {
		msgb_free(resp);
		return 0;
	}

	cmd->reply = talloc_asprintf(cmd, "%d",
		sysp->u.rfClockInfoCnf.rfTrx.iClkCor);

	ctrl_cmd_def_send(cd);

	msgb_free(resp);

	return 0;
}
static int get_clock_corr(struct ctrl_cmd *cmd, void *data)
{
	struct gsm_bts_trx *trx = cmd->node;
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);
	struct msgb *msg = sysp_msgb_alloc();
	SuperFemto_Prim_t *sysp = msgb_sysprim(msg);
	struct ctrl_cmd_def *cd;

	/* we could theoretically simply respond with a cached value, but I
	 * prefer to to ask the actual L1 about the currently used value to
	 * avoid any mistakes */

	/* geneate a deferred control command */
	cd = ctrl_cmd_def_make(fl1h, cmd, NULL, 10);

	sysp->id = SuperFemto_PrimId_RfClockInfoReq;
	sysp->u.rfClockInfoReq.u8RstClkCal = 0;

	l1if_req_compl(fl1h, msg, ctrl_get_clkcorr_cb, cd);

	return CTRL_CMD_HANDLED;
}
static int ctrl_set_clkcorr_cb(struct gsm_bts_trx *trx, struct msgb *resp,
				void *data)
{
	SuperFemto_Prim_t *sysp = msgb_sysprim(resp);
	struct ctrl_cmd_def *cd = data;
	struct ctrl_cmd *cmd = cd->cmd;

	if (ctrl_cmd_def_is_zombie(cd)) {
		msgb_free(resp);
		return 0;
	}

	if (sysp->u.rfClockSetupCnf.status != GsmL1_Status_Success) {
		cmd->type = CTRL_CMD_ERROR;
		cmd->reply = "Error setting new correction value.";
	} else
		cmd->reply = "success";

	ctrl_cmd_def_send(cd);

	msgb_free(resp);

	return 0;
}
static int set_clock_corr(struct ctrl_cmd *cmd, void *data)
{
	struct gsm_bts_trx *trx = cmd->node;
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);
	struct msgb *msg = sysp_msgb_alloc();
	SuperFemto_Prim_t *sysp = msgb_sysprim(msg);
	struct ctrl_cmd_def *cd;

	fl1h->clk_cal = atoi(cmd->value);

	/* geneate a deferred control command */
	cd = ctrl_cmd_def_make(fl1h, cmd, NULL, 10);

	sysp->id = SuperFemto_PrimId_RfClockSetupReq;
	sysp->u.rfClockSetupReq.rfTrx.iClkCor = fl1h->clk_cal;
	sysp->u.rfClockSetupReq.rfTrx.clkSrc = fl1h->clk_src;
	sysp->u.rfClockSetupReq.rfTrxClkCal.clkSrc = SuperFemto_ClkSrcId_GpsPps;

	l1if_req_compl(fl1h, msg, ctrl_set_clkcorr_cb, cd);

	return CTRL_CMD_HANDLED;
}

static int verify_clock_corr(struct ctrl_cmd *cmd, const char *value, void *data)
{
	/* FIXME: check the range */
	return 0;
}
#endif /* HW_SYSMOBTS_V1 */

int bts_model_ctrl_cmds_install(struct gsm_bts *bts)
{
	int rc = 0;

#ifndef HW_SYSMOBTS_V1
	rc |= ctrl_cmd_install(CTRL_NODE_TRX, &cmd_clock_info);
	rc |= ctrl_cmd_install(CTRL_NODE_TRX, &cmd_clock_corr);
#endif /* HW_SYSMOBTS_V1 */

	return rc;
}
