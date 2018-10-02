/* NuRAN Wireless Litecell 1.5 L1 API related definitions */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * based on:
 *     sysmobts.c
 *     (C) 2011 by Harald Welte <laforge@gnumonks.org>
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

#include <nrw/litecell15/litecell15.h>
#include <nrw/litecell15/gsml1const.h>
#include <nrw/litecell15/gsml1dbg.h>

#include "lc15bts.h"

enum l1prim_type lc15bts_get_l1prim_type(GsmL1_PrimId_t id)
{
	switch (id) {
	case GsmL1_PrimId_MphInitReq:       return L1P_T_REQ;
	case GsmL1_PrimId_MphCloseReq:      return L1P_T_REQ;
	case GsmL1_PrimId_MphConnectReq:    return L1P_T_REQ;
	case GsmL1_PrimId_MphDisconnectReq: return L1P_T_REQ;
	case GsmL1_PrimId_MphActivateReq:   return L1P_T_REQ;
	case GsmL1_PrimId_MphDeactivateReq: return L1P_T_REQ;
	case GsmL1_PrimId_MphConfigReq:     return L1P_T_REQ;
	case GsmL1_PrimId_MphMeasureReq:    return L1P_T_REQ;
	case GsmL1_PrimId_MphInitCnf:       return L1P_T_CONF;
	case GsmL1_PrimId_MphCloseCnf:      return L1P_T_CONF;
	case GsmL1_PrimId_MphConnectCnf:    return L1P_T_CONF;
	case GsmL1_PrimId_MphDisconnectCnf: return L1P_T_CONF;
	case GsmL1_PrimId_MphActivateCnf:   return L1P_T_CONF;
	case GsmL1_PrimId_MphDeactivateCnf: return L1P_T_CONF;
	case GsmL1_PrimId_MphConfigCnf:     return L1P_T_CONF;
	case GsmL1_PrimId_MphMeasureCnf:    return L1P_T_CONF;
	case GsmL1_PrimId_PhEmptyFrameReq:  return L1P_T_REQ;
	case GsmL1_PrimId_PhDataReq:        return L1P_T_REQ;
	case GsmL1_PrimId_MphTimeInd:       return L1P_T_IND;
	case GsmL1_PrimId_MphSyncInd:       return L1P_T_IND;
	case GsmL1_PrimId_PhConnectInd:     return L1P_T_IND;
	case GsmL1_PrimId_PhReadyToSendInd: return L1P_T_IND;
	case GsmL1_PrimId_PhDataInd:        return L1P_T_IND;
	case GsmL1_PrimId_PhRaInd:          return L1P_T_IND;
	default:                            return L1P_T_INVALID;
	}
}

const struct value_string lc15bts_l1prim_names[GsmL1_PrimId_NUM+1] = {
	{ GsmL1_PrimId_MphInitReq,	"MPH-INIT.req" },
	{ GsmL1_PrimId_MphCloseReq,	"MPH-CLOSE.req" },
	{ GsmL1_PrimId_MphConnectReq,	"MPH-CONNECT.req" },
	{ GsmL1_PrimId_MphDisconnectReq,"MPH-DISCONNECT.req" },
	{ GsmL1_PrimId_MphActivateReq,	"MPH-ACTIVATE.req" },
	{ GsmL1_PrimId_MphDeactivateReq,"MPH-DEACTIVATE.req" },
	{ GsmL1_PrimId_MphConfigReq,	"MPH-CONFIG.req" },
	{ GsmL1_PrimId_MphMeasureReq,	"MPH-MEASURE.req" },
	{ GsmL1_PrimId_MphInitCnf,	"MPH-INIT.conf" },
	{ GsmL1_PrimId_MphCloseCnf,	"MPH-CLOSE.conf" },
	{ GsmL1_PrimId_MphConnectCnf,	"MPH-CONNECT.conf" },
	{ GsmL1_PrimId_MphDisconnectCnf,"MPH-DISCONNECT.conf" },
	{ GsmL1_PrimId_MphActivateCnf,	"MPH-ACTIVATE.conf" },
	{ GsmL1_PrimId_MphDeactivateCnf,"MPH-DEACTIVATE.conf" },
	{ GsmL1_PrimId_MphConfigCnf,	"MPH-CONFIG.conf" },
	{ GsmL1_PrimId_MphMeasureCnf,	"MPH-MEASURE.conf" },
	{ GsmL1_PrimId_MphTimeInd,	"MPH-TIME.ind" },
	{ GsmL1_PrimId_MphSyncInd,	"MPH-SYNC.ind" },
	{ GsmL1_PrimId_PhEmptyFrameReq,	"PH-EMPTY_FRAME.req" },
	{ GsmL1_PrimId_PhDataReq,	"PH-DATA.req" },
	{ GsmL1_PrimId_PhConnectInd,	"PH-CONNECT.ind" },
	{ GsmL1_PrimId_PhReadyToSendInd,"PH-READY_TO_SEND.ind" },
	{ GsmL1_PrimId_PhDataInd,	"PH-DATA.ind" },
	{ GsmL1_PrimId_PhRaInd,		"PH-RA.ind" },
	{ 0, NULL }
};

GsmL1_PrimId_t lc15bts_get_l1prim_conf(GsmL1_PrimId_t id)
{
        switch (id) {
        case GsmL1_PrimId_MphInitReq:       return GsmL1_PrimId_MphInitCnf;
        case GsmL1_PrimId_MphCloseReq:      return GsmL1_PrimId_MphCloseCnf;
        case GsmL1_PrimId_MphConnectReq:    return GsmL1_PrimId_MphConnectCnf;
        case GsmL1_PrimId_MphDisconnectReq: return GsmL1_PrimId_MphDisconnectCnf;
        case GsmL1_PrimId_MphActivateReq:   return GsmL1_PrimId_MphActivateCnf;
        case GsmL1_PrimId_MphDeactivateReq: return GsmL1_PrimId_MphDeactivateCnf;
        case GsmL1_PrimId_MphConfigReq:     return GsmL1_PrimId_MphConfigCnf;
        case GsmL1_PrimId_MphMeasureReq:    return GsmL1_PrimId_MphMeasureCnf;
        default:                            return -1;	// Weak
        }
}

enum l1prim_type lc15bts_get_sysprim_type(Litecell15_PrimId_t id)
{
	switch (id) {
	case Litecell15_PrimId_SystemInfoReq:    return L1P_T_REQ;
	case Litecell15_PrimId_SystemInfoCnf:    return L1P_T_CONF;
	case Litecell15_PrimId_SystemFailureInd: return L1P_T_IND;
	case Litecell15_PrimId_ActivateRfReq:    return L1P_T_REQ;
	case Litecell15_PrimId_ActivateRfCnf:    return L1P_T_CONF;
	case Litecell15_PrimId_DeactivateRfReq:  return L1P_T_REQ;
	case Litecell15_PrimId_DeactivateRfCnf:  return L1P_T_CONF;
	case Litecell15_PrimId_SetTraceFlagsReq: return L1P_T_REQ;
	case Litecell15_PrimId_Layer1ResetReq:   return L1P_T_REQ;
	case Litecell15_PrimId_Layer1ResetCnf:   return L1P_T_CONF;
	case Litecell15_PrimId_SetCalibTblReq:   return L1P_T_REQ;
	case Litecell15_PrimId_SetCalibTblCnf:   return L1P_T_CONF;
	case Litecell15_PrimId_MuteRfReq:        return L1P_T_REQ;
	case Litecell15_PrimId_MuteRfCnf:        return L1P_T_CONF;
	case Litecell15_PrimId_SetRxAttenReq:    return L1P_T_REQ;
	case Litecell15_PrimId_SetRxAttenCnf:    return L1P_T_CONF;
	default:                                 return L1P_T_INVALID;
	}
}

const struct value_string lc15bts_sysprim_names[Litecell15_PrimId_NUM+1] = {
	{ Litecell15_PrimId_SystemInfoReq,	"SYSTEM-INFO.req" },
	{ Litecell15_PrimId_SystemInfoCnf,	"SYSTEM-INFO.conf" },
	{ Litecell15_PrimId_SystemFailureInd,	"SYSTEM-FAILURE.ind" },
	{ Litecell15_PrimId_ActivateRfReq,	"ACTIVATE-RF.req" },
	{ Litecell15_PrimId_ActivateRfCnf,	"ACTIVATE-RF.conf" },
	{ Litecell15_PrimId_DeactivateRfReq,	"DEACTIVATE-RF.req" },
	{ Litecell15_PrimId_DeactivateRfCnf,	"DEACTIVATE-RF.conf" },
	{ Litecell15_PrimId_SetTraceFlagsReq,	"SET-TRACE-FLAGS.req" },
	{ Litecell15_PrimId_Layer1ResetReq,	"LAYER1-RESET.req" },
	{ Litecell15_PrimId_Layer1ResetCnf,	"LAYER1-RESET.conf" },
	{ Litecell15_PrimId_SetCalibTblReq,	"SET-CALIB.req" },
	{ Litecell15_PrimId_SetCalibTblCnf,	"SET-CALIB.cnf" },
	{ Litecell15_PrimId_MuteRfReq,	        "MUTE-RF.req" },
	{ Litecell15_PrimId_MuteRfCnf,	        "MUTE-RF.cnf" },
	{ Litecell15_PrimId_SetRxAttenReq,	"SET-RX-ATTEN.req" },
	{ Litecell15_PrimId_SetRxAttenCnf,	"SET-RX-ATTEN-CNF.cnf" },
	{ 0, NULL }
};

Litecell15_PrimId_t lc15bts_get_sysprim_conf(Litecell15_PrimId_t id)
{
        switch (id) {
	case Litecell15_PrimId_SystemInfoReq:    return Litecell15_PrimId_SystemInfoCnf;
	case Litecell15_PrimId_ActivateRfReq:    return Litecell15_PrimId_ActivateRfCnf;
	case Litecell15_PrimId_DeactivateRfReq:  return Litecell15_PrimId_DeactivateRfCnf;
	case Litecell15_PrimId_Layer1ResetReq:   return Litecell15_PrimId_Layer1ResetCnf;
	case Litecell15_PrimId_SetCalibTblReq:   return Litecell15_PrimId_SetCalibTblCnf;
	case Litecell15_PrimId_MuteRfReq:        return Litecell15_PrimId_MuteRfCnf;
	case Litecell15_PrimId_SetRxAttenReq:    return Litecell15_PrimId_SetRxAttenCnf;
        default:                                 return -1;	// Weak
        }
}

const struct value_string lc15bts_l1sapi_names[GsmL1_Sapi_NUM+1] = {
	{ GsmL1_Sapi_Idle,	"IDLE" },
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

const struct value_string lc15bts_l1status_names[GSML1_STATUS_NUM+1] = {
	{ GsmL1_Status_Success,		"Success" },
	{ GsmL1_Status_Generic,		"Generic error" },
	{ GsmL1_Status_NoMemory,	"Not enough memory" },
	{ GsmL1_Status_Timeout,		"Timeout" },
	{ GsmL1_Status_InvalidParam,	"Invalid parameter" },
	{ GsmL1_Status_Busy,		"Resource busy" },
	{ GsmL1_Status_NoRessource,	"No more resources" },
	{ GsmL1_Status_Uninitialized,	"Trying to use uninitialized resource" },
	{ GsmL1_Status_NullInterface,	"Trying to call a NULL interface" },
	{ GsmL1_Status_NullFctnPtr,	"Trying to call a NULL function ptr" },
	{ GsmL1_Status_BadCrc,		"Bad CRC" },
	{ GsmL1_Status_BadUsf,		"Bad USF" },
	{ GsmL1_Status_InvalidCPS,	"Invalid CPS field" },
	{ GsmL1_Status_UnexpectedBurst,	"Unexpected burst" },
	{ GsmL1_Status_UnavailCodec,	"AMR codec is unavailable" },
	{ GsmL1_Status_CriticalError,	"Critical error" },
	{ GsmL1_Status_OverheatError,	"Overheat error" },
	{ GsmL1_Status_DeviceError,	"Device error" },
	{ GsmL1_Status_FacchError,	"FACCH / TCH order error" },
	{ GsmL1_Status_AlreadyDeactivated, "Lchan already deactivated" },
	{ GsmL1_Status_TxBurstFifoOvrn,	"FIFO overrun" },
	{ GsmL1_Status_TxBurstFifoUndr,	"FIFO underrun" },
	{ GsmL1_Status_NotSynchronized,	"Not synchronized" },
	{ GsmL1_Status_Unsupported,	"Unsupported feature" },
	{ GsmL1_Status_ClockError,	"System clock error" },
	{ 0, NULL }
};

const struct value_string lc15bts_tracef_names[29] = {
	{ DBG_DEBUG,			"DEBUG" },
	{ DBG_L1WARNING,		"L1_WARNING" },
	{ DBG_ERROR,			"ERROR" },
	{ DBG_L1RXMSG,			"L1_RX_MSG" },
	{ DBG_L1RXMSGBYTE,		"L1_RX_MSG_BYTE" },
	{ DBG_L1TXMSG,			"L1_TX_MSG" },
	{ DBG_L1TXMSGBYTE,		"L1_TX_MSG_BYTE" },
	{ DBG_MPHCNF,			"MPH_CNF" },
	{ DBG_MPHIND,			"MPH_IND" },
	{ DBG_MPHREQ,			"MPH_REQ" },
	{ DBG_PHIND,			"PH_IND" },
	{ DBG_PHREQ,			"PH_REQ" },
	{ DBG_PHYRF,			"PHY_RF" },
	{ DBG_PHYRFMSGBYTE,		"PHY_MSG_BYTE" },
	{ DBG_MODE,			"MODE" },
	{ DBG_TDMAINFO,			"TDMA_INFO" },
	{ DBG_BADCRC,			"BAD_CRC" },
	{ DBG_PHINDBYTE,		"PH_IND_BYTE" },
	{ DBG_PHREQBYTE,		"PH_REQ_BYTE" },
	{ DBG_DEVICEMSG,		"DEVICE_MSG" },
	{ DBG_RACHINFO,			"RACH_INFO" },
	{ DBG_LOGCHINFO,		"LOG_CH_INFO" },
	{ DBG_MEMORY,			"MEMORY" },
	{ DBG_PROFILING,		"PROFILING" },
	{ DBG_TESTCOMMENT,		"TEST_COMMENT" },
	{ DBG_TEST,			"TEST" },
	{ DBG_STATUS,			"STATUS" },
	{ 0, NULL }
};

const struct value_string lc15bts_tracef_docs[29] = {
	{ DBG_DEBUG,			"Debug Region" },
	{ DBG_L1WARNING,		"L1 Warning Region" },
	{ DBG_ERROR,			"Error Region" },
	{ DBG_L1RXMSG,			"L1_RX_MSG Region" },
	{ DBG_L1RXMSGBYTE,		"L1_RX_MSG_BYTE Region" },
	{ DBG_L1TXMSG,			"L1_TX_MSG Region" },
	{ DBG_L1TXMSGBYTE,		"L1_TX_MSG_BYTE Region" },
	{ DBG_MPHCNF,			"MphConfirmation Region" },
	{ DBG_MPHIND,			"MphIndication Region" },
	{ DBG_MPHREQ,			"MphRequest Region" },
	{ DBG_PHIND,			"PhIndication Region" },
	{ DBG_PHREQ,			"PhRequest Region" },
	{ DBG_PHYRF,			"PhyRF Region" },
	{ DBG_PHYRFMSGBYTE,		"PhyRF Message Region" },
	{ DBG_MODE,			"Mode Region" },
	{ DBG_TDMAINFO,			"TDMA Info Region" },
	{ DBG_BADCRC,			"Bad CRC Region" },
	{ DBG_PHINDBYTE,		"PH_IND_BYTE" },
	{ DBG_PHREQBYTE,		"PH_REQ_BYTE" },
	{ DBG_DEVICEMSG,		"Device Message Region" },
	{ DBG_RACHINFO,			"RACH Info" },
	{ DBG_LOGCHINFO,		"LOG_CH_INFO" },
	{ DBG_MEMORY,			"Memory Region" },
	{ DBG_PROFILING,		"Profiling Region" },
	{ DBG_TESTCOMMENT,		"Test Comments" },
	{ DBG_TEST,			"Test Region" },
	{ DBG_STATUS,			"Status Region" },
	{ 0, NULL }
};

const struct value_string lc15bts_tch_pl_names[] = {
	{ GsmL1_TchPlType_NA,			"N/A" },
	{ GsmL1_TchPlType_Fr,			"FR" },
	{ GsmL1_TchPlType_Hr,			"HR" },
	{ GsmL1_TchPlType_Efr,			"EFR" },
	{ GsmL1_TchPlType_Amr,			"AMR(IF2)" },
	{ GsmL1_TchPlType_Amr_SidBad,		"AMR(SID BAD)" },
	{ GsmL1_TchPlType_Amr_Onset,		"AMR(ONSET)" },
	{ GsmL1_TchPlType_Amr_Ratscch,		"AMR(RATSCCH)" },
	{ GsmL1_TchPlType_Amr_SidUpdateInH,	"AMR(SID_UPDATE INH)" },
	{ GsmL1_TchPlType_Amr_SidFirstP1,	"AMR(SID_FIRST P1)" },
	{ GsmL1_TchPlType_Amr_SidFirstP2,	"AMR(SID_FIRST P2)" },
	{ GsmL1_TchPlType_Amr_SidFirstInH,	"AMR(SID_FIRST INH)" },
	{ GsmL1_TchPlType_Amr_RatscchMarker,	"AMR(RATSCCH MARK)" },
	{ GsmL1_TchPlType_Amr_RatscchData,	"AMR(RATSCCH DATA)" },
	{ 0, NULL }
};

const struct value_string lc15bts_dir_names[] = {
	{ GsmL1_Dir_TxDownlink,	"TxDL" },
	{ GsmL1_Dir_TxUplink,	"TxUL" },
	{ GsmL1_Dir_RxUplink,	"RxUL" },
	{ GsmL1_Dir_RxDownlink,	"RxDL" },
	{ GsmL1_Dir_TxDownlink|GsmL1_Dir_RxUplink, "BOTH" },
	{ 0, NULL }
};

const struct value_string lc15bts_chcomb_names[] = {
	{ GsmL1_LogChComb_0,	"dummy" },
	{ GsmL1_LogChComb_I,	"tch_f" },
	{ GsmL1_LogChComb_II,	"tch_h" },
	{ GsmL1_LogChComb_IV,	"ccch" },
	{ GsmL1_LogChComb_V,	"ccch_sdcch4" },
	{ GsmL1_LogChComb_VII,	"sdcch8" },
	{ GsmL1_LogChComb_XIII,	"pdtch" },
	{ 0, NULL }
};

const uint8_t pdch_msu_size[_NUM_PDCH_CS] = {
	[PDCH_CS_1]	= 23,
	[PDCH_CS_2]	= 34,
	[PDCH_CS_3]	= 40,
	[PDCH_CS_4]	= 54,
	[PDCH_MCS_1]	= 27,
	[PDCH_MCS_2]	= 33,
	[PDCH_MCS_3]	= 42,
	[PDCH_MCS_4]	= 49,
	[PDCH_MCS_5]	= 60,
	[PDCH_MCS_6]	= 78,
	[PDCH_MCS_7]	= 118,
	[PDCH_MCS_8]	= 142,
	[PDCH_MCS_9]	= 154
};
