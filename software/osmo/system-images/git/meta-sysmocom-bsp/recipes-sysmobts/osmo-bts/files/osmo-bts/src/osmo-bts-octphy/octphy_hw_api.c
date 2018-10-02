/* Layer 1 (PHY) interface of osmo-bts OCTPHY integration */

/* Copyright (c) 2015 Harald Welte <laforge@gnumonks.org>
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
#include <stdint.h>
#include <errno.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>

#include <osmo-bts/logging.h>

#include "l1_if.h"
#include "l1_oml.h"
#include "l1_utils.h"
#include "octphy_hw_api.h"

#include <octphy/octvc1/octvc1_rc2string.h>
#include <octphy/octvc1/hw/octvc1_hw_api.h>
#include <octphy/octvc1/hw/octvc1_hw_api_swap.h>

/* Chapter 12.1 */
static int get_pcb_info_compl_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_HW_MSG_PCB_INFO_RSP *pir =
		(tOCTVC1_HW_MSG_PCB_INFO_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_PCB_INFO_RSP_SWAP(pir);

	LOGP(DL1C, LOGL_INFO, "HW-PCB-INFO.resp: Name=%s %s, Serial=%s, "
		"FileName=%s, InfoSource=%u, InfoState=%u, GpsName=%s, "
		"WiFiName=%s\n", pir->szName, pir->ulDeviceId ? "SEC" : "PRI",
		pir->szSerial, pir->szFilename, pir->ulInfoSource,
		pir->ulInfoState, pir->szGpsName, pir->szWifiName);

	msgb_free(resp);
	return 0;
}

/* Chapter 12.1 */
int octphy_hw_get_pcb_info(struct octphy_hdl *fl1h)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_PCB_INFO_CMD *pic;

	pic = (tOCTVC1_HW_MSG_PCB_INFO_CMD *) msgb_put(msg, sizeof(*pic));

	l1if_fill_msg_hdr(&pic->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_PCB_INFO_CID);

	mOCTVC1_HW_MSG_PCB_INFO_CMD_SWAP(pic);

	return l1if_req_compl(fl1h, msg, get_pcb_info_compl_cb, NULL);
}

/* Chapter 12.9 */
static int rf_port_info_compl_cb(struct octphy_hdl *fl1, struct msgb *resp,
				 void *data)
{
	tOCTVC1_HW_MSG_RF_PORT_INFO_RSP *pir =
		(tOCTVC1_HW_MSG_RF_PORT_INFO_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_RF_PORT_INFO_RSP_SWAP(pir);

	LOGP(DL1C, LOGL_INFO, "RF-PORT-INFO.resp Idx=%u, InService=%u, "
		"hOwner=0x%x, Id=%u, FreqMin=%u, FreqMax=%u\n",
		pir->ulPortIndex, pir->ulInService, pir->hOwner,
		pir->ulPortInterfaceId, pir->ulFrequencyMinKhz,
		pir->ulFrequencyMaxKhz);

	msgb_free(resp);
	return 0;
}

/* Chapter 12.9 */
int octphy_hw_get_rf_port_info(struct octphy_hdl *fl1h, uint32_t index)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_RF_PORT_INFO_CMD *pic;

	pic = (tOCTVC1_HW_MSG_RF_PORT_INFO_CMD *) msgb_put(msg, sizeof(*pic));

	l1if_fill_msg_hdr(&pic->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_RF_PORT_INFO_CID);

	pic->ulPortIndex = index;

	mOCTVC1_HW_MSG_RF_PORT_INFO_CMD_SWAP(pic);

	return l1if_req_compl(fl1h, msg, rf_port_info_compl_cb, NULL);
}

/* Chapter 12.10 */
static int rf_port_stats_compl_cb(struct octphy_hdl *fl1, struct msgb *resp,
				  void *data)
{
	struct octphy_hw_get_cb_data *get_cb_data;

	tOCTVC1_HW_MSG_RF_PORT_STATS_RSP *psr =
		(tOCTVC1_HW_MSG_RF_PORT_STATS_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_RF_PORT_STATS_RSP_SWAP(psr);

	LOGP(DL1C, LOGL_INFO, "RF-PORT-STATS.resp Idx=%u RadioStandard=%s, "
		"Rx(Bytes=%u, Overflow=%u, AvgBps=%u, Period=%uus, Freq=%u) "
		"Tx(Bytes=%i, Underflow=%u, AvgBps=%u, Period=%uus, Freq=%u)\n",
		psr->ulPortIndex,
		get_value_string(radio_std_vals, psr->ulRadioStandard),
		psr->RxStats.ulRxByteCnt, psr->RxStats.ulRxOverflowCnt,
		psr->RxStats.ulRxAverageBytePerSecond,
		psr->RxStats.ulRxAveragePeriodUs,
#if OCTPHY_USE_FREQUENCY == 1
		psr->RxStats.Frequency.ulValue,
#else
		psr->RxStats.ulFrequencyKhz,
#endif
		psr->TxStats.ulTxByteCnt, psr->TxStats.ulTxUnderflowCnt,
		psr->TxStats.ulTxAverageBytePerSecond,
		psr->TxStats.ulTxAveragePeriodUs,
#if OCTPHY_USE_FREQUENCY == 1
		psr->TxStats.Frequency.ulValue);
#else
		psr->TxStats.ulFrequencyKhz);
#endif

	get_cb_data = (struct octphy_hw_get_cb_data*) data;
	get_cb_data->cb(resp,get_cb_data->data);

	msgb_free(resp);
	return 0;
}

/* Chapter 12.10 */
int octphy_hw_get_rf_port_stats(struct octphy_hdl *fl1h, uint32_t index,
				struct octphy_hw_get_cb_data *cb_data)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_RF_PORT_STATS_CMD *psc;

	psc = (tOCTVC1_HW_MSG_RF_PORT_STATS_CMD *) msgb_put(msg, sizeof(*psc));

	l1if_fill_msg_hdr(&psc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_RF_PORT_STATS_CID);

	psc->ulPortIndex = index;
	psc->ulResetStatsFlag = cOCT_FALSE;

	mOCTVC1_HW_MSG_RF_PORT_STATS_CMD_SWAP(psc);

	return l1if_req_compl(fl1h, msg, rf_port_stats_compl_cb, cb_data);
}

static const struct value_string rx_gain_mode_vals[] = {
	{ cOCTVC1_RADIO_RX_GAIN_CTRL_MODE_ENUM_MGC, "Manual" },
	{ cOCTVC1_RADIO_RX_GAIN_CTRL_MODE_ENUM_AGC_FAST_ATK, "Automatic (fast)" },
	{ cOCTVC1_RADIO_RX_GAIN_CTRL_MODE_ENUM_AGC_SLOW_ATK, "Automatic (slow)" },
	{ 0, NULL }
};

/* Chapter 12.13 */
static int rf_ant_rx_compl_cb(struct octphy_hdl *fl1, struct msgb *resp,
				void *data)
{
	tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_RSP *arc =
		(tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_RSP_SWAP(arc);

	LOGP(DL1C, LOGL_INFO, "ANT-RX-CONFIG.resp(Port=%u, Ant=%u): %s, "
		"Gain %d dB, GainCtrlMode=%s\n",
		arc->ulPortIndex,  arc->ulAntennaIndex,
#ifdef OCTPHY_USE_RX_CONFIG
		arc->RxConfig.ulEnableFlag ? "Enabled" : "Disabled",
		arc->RxConfig.lRxGaindB/512,
		get_value_string(rx_gain_mode_vals, arc->RxConfig.ulRxGainMode));
#else
		arc->ulEnableFlag ? "Enabled" : "Disabled",
		arc->lRxGaindB/512,
		get_value_string(rx_gain_mode_vals, arc->ulRxGainMode));
#endif
	msgb_free(resp);
	return 0;
}

/* Chapter 12.13 */
int octphy_hw_get_rf_ant_rx_config(struct octphy_hdl *fl1h, uint32_t port_idx,
				   uint32_t ant_idx)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_CMD *psc;

	psc = (tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_CMD *)
			msgb_put(msg, sizeof(*psc));

	l1if_fill_msg_hdr(&psc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_CID);

	psc->ulPortIndex = port_idx;
	psc->ulAntennaIndex = ant_idx;

	mOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_RX_CONFIG_CMD_SWAP(psc);

	return l1if_req_compl(fl1h, msg, rf_ant_rx_compl_cb, NULL);

}

/* Chapter 12.14 */
static int rf_ant_tx_compl_cb(struct octphy_hdl *fl1, struct msgb *resp,
				void *data)
{
	tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_RSP *atc =
		(tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_RSP_SWAP(atc);

	LOGP(DL1C, LOGL_INFO, "ANT-TX-CONFIG.resp(Port=%u, Ant=%u): %s, "
		"Gain %d dB\n",
		atc->ulPortIndex, atc->ulAntennaIndex,
#ifdef OCTPHY_USE_TX_CONFIG
		atc->TxConfig.ulEnableFlag? "Enabled" : "Disabled",
		atc->TxConfig.lTxGaindB/512);
#else
		atc->ulEnableFlag ? "Enabled" : "Disabled",
		atc->lTxGaindB/512);

#endif
	msgb_free(resp);
	return 0;
}

/* Chapter 12.14 */
int octphy_hw_get_rf_ant_tx_config(struct octphy_hdl *fl1h, uint32_t port_idx,
				   uint32_t ant_idx)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_CMD *psc;

	psc = (tOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_CMD *)
			msgb_put(msg, sizeof(*psc));

	l1if_fill_msg_hdr(&psc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_CID);

	psc->ulPortIndex = port_idx;
	psc->ulAntennaIndex = ant_idx;

	mOCTVC1_HW_MSG_RF_PORT_INFO_ANTENNA_TX_CONFIG_CMD_SWAP(psc);

	return l1if_req_compl(fl1h, msg, rf_ant_tx_compl_cb, NULL);

}

static const struct value_string clocksync_source_vals[] = {
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_ENUM_FREQ_1HZ, "1 Hz" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_ENUM_FREQ_10MHZ, "10 MHz" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_ENUM_FREQ_30_72MHZ, "30.72 MHz" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_ENUM_FREQ_1HZ_EXT, "1 Hz (ext)"},
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_ENUM_NONE, "None" },
	{ 0, NULL }
};

#if OCTPHY_USE_CLK_SOURCE_SELECTION == 1
static const struct value_string clocksync_sel_vals[] = {
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_SELECTION_ENUM_AUTOSELECT,
		"Autoselect" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_SELECTION_ENUM_CONFIG_FILE,
		"Config File" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_SOURCE_SELECTION_ENUM_HOST_APPLICATION,
		"Host Application" },
	{ 0, NULL }
};
#endif

/* Chapter 12.15 */
static int get_clock_sync_compl_cb(struct octphy_hdl *fl1, struct msgb *resp,
				   void *data)
{
	tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_RSP *cir =
		(tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_RSP_SWAP(cir);

	LOGP(DL1C, LOGL_INFO, "CLOCK-SYNC-MGR-INFO.resp Reference=%s ",
		get_value_string(clocksync_source_vals, cir->ulClkSourceRef));

#if OCTPHY_USE_CLK_SOURCE_SELECTION == 1
	LOGPC(DL1C, LOGL_INFO, "Selection=%s)\n",
		get_value_string(clocksync_sel_vals, cir->ulClkSourceSelection));
#else
	LOGPC(DL1C, LOGL_INFO, "Clock Drift= %u Us\n",
		cir->ulMaxDriftDurationUs);
#endif

	msgb_free(resp);
	return 0;
}

/* Chapter 12.15 */
int octphy_hw_get_clock_sync_info(struct octphy_hdl *fl1h)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_CMD *cic;

	cic = (tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_CMD *)
		msgb_put(msg, sizeof(*cic));
	l1if_fill_msg_hdr(&cic->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_CID);

	mOCTVC1_HW_MSG_CLOCK_SYNC_MGR_INFO_CMD_SWAP(cic);

	return l1if_req_compl(fl1h, msg, get_clock_sync_compl_cb, NULL);
}

/* Chapter 12.16 */
static int get_clock_sync_stats_cb(struct octphy_hdl *fl1, struct msgb *resp,
				   void *data)
{
	struct octphy_hw_get_cb_data *get_cb_data;

	tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP *csr =
		(tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP *) resp->l2h;

	mOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP_SWAP(csr);

	LOGP(DL1C, LOGL_INFO, "CLOCK-SYNC-MGR-STATS.resp");
	LOGPC(DL1C, LOGL_INFO, " State=%s,",
	      get_value_string(clocksync_state_vals, csr->ulState));
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_CLOCK_ERROR == 1
	LOGPC(DL1C, LOGL_INFO, " ClockError=%d,", csr->lClockError);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_DROPPED_CYCLES == 1
	LOGPC(DL1C, LOGL_INFO, " DroppedCycles=%d,", csr->lDroppedCycles);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_PLL_FREQ_HZ == 1
	LOGPC(DL1C, LOGL_INFO, " PllFreqHz=%u,", csr->ulPllFreqHz);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_PLL_FRACTIONAL_FREQ_HZ == 1
	LOGPC(DL1C, LOGL_INFO, " PllFract=%u,", csr->ulPllFractionalFreqHz);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_SLIP_CNT == 1
	LOGPC(DL1C, LOGL_INFO, " SlipCnt=%u,", csr->ulSlipCnt);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_SYNC_LOSS_CNT == 1
	LOGPC(DL1C, LOGL_INFO, " SyncLosses=%u,", csr->ulSyncLossCnt);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_SYNC_LOSSE_CNT == 1
	LOGPC(DL1C, LOGL_INFO, " SyncLosses=%u,", csr->ulSyncLosseCnt);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_SOURCE_STATE == 1
	LOGPC(DL1C, LOGL_INFO, " SourceState=%u,", csr->ulSourceState);
#endif
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_DAC_STATE == 1
	LOGPC(DL1C, LOGL_INFO, " CLOCK-SYNC-MGR-STATS.resp State=%s,",
	      get_value_string(clocksync_dac_vals, csr->ulDacState));
#endif
	LOGPC(DL1C, LOGL_INFO, " LOCK-SYNC-MGR-USR-PROCESS.resp State=%s,",
	      get_value_string(usr_process_id, csr->ulOwnerProcessUid));
	LOGPC(DL1C, LOGL_INFO, " DacValue=%u,", csr->ulDacValue);
#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_DRIFT_ELAPSE_TIME_US == 1
	LOGPC(DL1C, LOGL_INFO, " DriftElapseTime=%u Us,",
	      csr->ulDriftElapseTimeUs);
#endif
	LOGPC(DL1C, LOGL_INFO, "\n");

	get_cb_data = (struct octphy_hw_get_cb_data*) data;
	get_cb_data->cb(resp,get_cb_data->data);

	msgb_free(resp);
	return 0;
}

/* Chapter 12.16 */
int octphy_hw_get_clock_sync_stats(struct octphy_hdl *fl1h,
				   struct octphy_hw_get_cb_data *cb_data)
{
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_CMD *csc;

	csc = (tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_CMD *)
				msgb_put(msg, sizeof(*csc));
	l1if_fill_msg_hdr(&csc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_CID);

	mOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_CMD_SWAP(csc);

	return l1if_req_compl(fl1h, msg, get_clock_sync_stats_cb, cb_data);
}

