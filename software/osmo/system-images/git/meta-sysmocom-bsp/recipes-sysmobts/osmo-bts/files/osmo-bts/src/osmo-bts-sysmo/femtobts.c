/* sysmocom femtobts L1 API related definitions */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
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

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1const.h>
#include <sysmocom/femtobts/gsml1dbg.h>

#include "femtobts.h"

const enum l1prim_type femtobts_l1prim_type[GsmL1_PrimId_NUM] = {
	[GsmL1_PrimId_MphInitReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphCloseReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphConnectReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphDisconnectReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphActivateReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphDeactivateReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphConfigReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphMeasureReq]	= L1P_T_REQ,
	[GsmL1_PrimId_MphInitCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphCloseCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphConnectCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphDisconnectCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphActivateCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphDeactivateCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphConfigCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphMeasureCnf]	= L1P_T_CONF,
	[GsmL1_PrimId_MphTimeInd]	= L1P_T_IND,
	[GsmL1_PrimId_MphSyncInd]	= L1P_T_IND,
	[GsmL1_PrimId_PhEmptyFrameReq]	= L1P_T_REQ,
	[GsmL1_PrimId_PhDataReq]	= L1P_T_REQ,
	[GsmL1_PrimId_PhConnectInd]	= L1P_T_IND,
	[GsmL1_PrimId_PhReadyToSendInd]	= L1P_T_IND,
	[GsmL1_PrimId_PhDataInd]	= L1P_T_IND,
	[GsmL1_PrimId_PhRaInd]		= L1P_T_IND,
};

const struct value_string femtobts_l1prim_names[GsmL1_PrimId_NUM+1] = {
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

const GsmL1_PrimId_t femtobts_l1prim_req2conf[GsmL1_PrimId_NUM] = {
	[GsmL1_PrimId_MphInitReq]	= GsmL1_PrimId_MphInitCnf,
	[GsmL1_PrimId_MphCloseReq]	= GsmL1_PrimId_MphCloseCnf,
	[GsmL1_PrimId_MphConnectReq]	= GsmL1_PrimId_MphConnectCnf,
	[GsmL1_PrimId_MphDisconnectReq]	= GsmL1_PrimId_MphDisconnectCnf,
	[GsmL1_PrimId_MphActivateReq]	= GsmL1_PrimId_MphActivateCnf,
	[GsmL1_PrimId_MphDeactivateReq]	= GsmL1_PrimId_MphDeactivateCnf,
	[GsmL1_PrimId_MphConfigReq]	= GsmL1_PrimId_MphConfigCnf,
	[GsmL1_PrimId_MphMeasureReq]	= GsmL1_PrimId_MphMeasureCnf,
};

const enum l1prim_type femtobts_sysprim_type[SuperFemto_PrimId_NUM] = {
	[SuperFemto_PrimId_SystemInfoReq]		= L1P_T_REQ,
	[SuperFemto_PrimId_SystemInfoCnf]		= L1P_T_CONF,
	[SuperFemto_PrimId_SystemFailureInd]	= L1P_T_IND,
	[SuperFemto_PrimId_ActivateRfReq]		= L1P_T_REQ,
	[SuperFemto_PrimId_ActivateRfCnf]		= L1P_T_CONF,
	[SuperFemto_PrimId_DeactivateRfReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_DeactivateRfCnf]	= L1P_T_CONF,
	[SuperFemto_PrimId_SetTraceFlagsReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_RfClockInfoReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_RfClockInfoCnf]	= L1P_T_CONF,
	[SuperFemto_PrimId_RfClockSetupReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_RfClockSetupCnf]	= L1P_T_CONF,
	[SuperFemto_PrimId_Layer1ResetReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_Layer1ResetCnf]	= L1P_T_CONF,
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,1,0)
	[SuperFemto_PrimId_GetTxCalibTblReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_GetTxCalibTblCnf]	= L1P_T_CONF,
	[SuperFemto_PrimId_SetTxCalibTblReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_SetTxCalibTblCnf]	= L1P_T_CONF,
	[SuperFemto_PrimId_GetRxCalibTblReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_GetRxCalibTblCnf]	= L1P_T_CONF,
	[SuperFemto_PrimId_SetRxCalibTblReq]	= L1P_T_REQ,
	[SuperFemto_PrimId_SetRxCalibTblCnf]	= L1P_T_CONF,
#endif
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(3,6,0)
	[SuperFemto_PrimId_MuteRfReq]	        = L1P_T_REQ,
	[SuperFemto_PrimId_MuteRfCnf]	        = L1P_T_CONF,
#endif
};

const struct value_string femtobts_sysprim_names[SuperFemto_PrimId_NUM+1] = {
	{ SuperFemto_PrimId_SystemInfoReq,	"SYSTEM-INFO.req" },
	{ SuperFemto_PrimId_SystemInfoCnf,	"SYSTEM-INFO.conf" },
	{ SuperFemto_PrimId_SystemFailureInd,	"SYSTEM-FAILURE.ind" },
	{ SuperFemto_PrimId_ActivateRfReq,	"ACTIVATE-RF.req" },
	{ SuperFemto_PrimId_ActivateRfCnf,	"ACTIVATE-RF.conf" },
	{ SuperFemto_PrimId_DeactivateRfReq,	"DEACTIVATE-RF.req" },
	{ SuperFemto_PrimId_DeactivateRfCnf,	"DEACTIVATE-RF.conf" },
	{ SuperFemto_PrimId_SetTraceFlagsReq,	"SET-TRACE-FLAGS.req" },
	{ SuperFemto_PrimId_RfClockInfoReq,	"RF-CLOCK-INFO.req" },
	{ SuperFemto_PrimId_RfClockInfoCnf,	"RF-CLOCK-INFO.conf" },
	{ SuperFemto_PrimId_RfClockSetupReq,	"RF-CLOCK-SETUP.req" },
	{ SuperFemto_PrimId_RfClockSetupCnf,	"RF-CLOCK-SETUP.conf" },
	{ SuperFemto_PrimId_Layer1ResetReq,	"LAYER1-RESET.req" },
	{ SuperFemto_PrimId_Layer1ResetCnf,	"LAYER1-RESET.conf" },
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,1,0)
	{ SuperFemto_PrimId_GetTxCalibTblReq,	"GET-TX-CALIB.req" },
	{ SuperFemto_PrimId_GetTxCalibTblCnf,	"GET-TX-CALIB.cnf" },
	{ SuperFemto_PrimId_SetTxCalibTblReq,	"SET-TX-CALIB.req" },
	{ SuperFemto_PrimId_SetTxCalibTblCnf,	"SET-TX-CALIB.cnf" },
	{ SuperFemto_PrimId_GetRxCalibTblReq,	"GET-RX-CALIB.req" },
	{ SuperFemto_PrimId_GetRxCalibTblCnf,	"GET-RX-CALIB.cnf" },
	{ SuperFemto_PrimId_SetRxCalibTblReq,	"SET-RX-CALIB.req" },
	{ SuperFemto_PrimId_SetRxCalibTblCnf,	"SET-RX-CALIB.cnf" },
#endif
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(3,6,0)
	{ SuperFemto_PrimId_MuteRfReq,	        "MUTE-RF.req" },
	{ SuperFemto_PrimId_MuteRfCnf,	        "MUTE-RF.cnf" },
#endif
	{ 0, NULL }
};

const SuperFemto_PrimId_t femtobts_sysprim_req2conf[SuperFemto_PrimId_NUM] = {
	[SuperFemto_PrimId_SystemInfoReq]	= SuperFemto_PrimId_SystemInfoCnf,
	[SuperFemto_PrimId_ActivateRfReq]	= SuperFemto_PrimId_ActivateRfCnf,
	[SuperFemto_PrimId_DeactivateRfReq]	= SuperFemto_PrimId_DeactivateRfCnf,
	[SuperFemto_PrimId_RfClockInfoReq]	= SuperFemto_PrimId_RfClockInfoCnf,
	[SuperFemto_PrimId_RfClockSetupReq]	= SuperFemto_PrimId_RfClockSetupCnf,
	[SuperFemto_PrimId_Layer1ResetReq] 	= SuperFemto_PrimId_Layer1ResetCnf,
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,1,0)
	[SuperFemto_PrimId_GetTxCalibTblReq]	= SuperFemto_PrimId_GetTxCalibTblCnf,
	[SuperFemto_PrimId_SetTxCalibTblReq]	= SuperFemto_PrimId_SetTxCalibTblCnf,
	[SuperFemto_PrimId_GetRxCalibTblReq]	= SuperFemto_PrimId_GetRxCalibTblCnf,
	[SuperFemto_PrimId_SetRxCalibTblReq]	= SuperFemto_PrimId_SetRxCalibTblCnf,
#endif
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(3,6,0)
	[SuperFemto_PrimId_MuteRfReq]	        = SuperFemto_PrimId_MuteRfCnf,
#endif
};

const struct value_string femtobts_l1sapi_names[GsmL1_Sapi_NUM+1] = {
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

const struct value_string femtobts_l1status_names[GSML1_STATUS_NUM+1] = {
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
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(5,1,0)
	{ GsmL1_Status_ClockError,	"Clock error" },
#endif
	{ 0, NULL }
};

const struct value_string femtobts_tracef_names[29] = {
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

const struct value_string femtobts_tracef_docs[29] = {
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

const struct value_string femtobts_tch_pl_names[] = {
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

const struct value_string femtobts_clksrc_names[] = {
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(2,1,0)
	{ SuperFemto_ClkSrcId_None,	"None" },
	{ SuperFemto_ClkSrcId_Ocxo,	"ocxo" },
	{ SuperFemto_ClkSrcId_Tcxo,	"tcxo" },
	{ SuperFemto_ClkSrcId_External,	"ext" },
	{ SuperFemto_ClkSrcId_GpsPps,	"gps" },
	{ SuperFemto_ClkSrcId_Trx,	"trx" },
	{ SuperFemto_ClkSrcId_Rx,	"rx" },
	{ SuperFemto_ClkSrcId_Edge,	"edge" },
	{ SuperFemto_ClkSrcId_NetList,	"nwl" },
#else
	{ SF_CLKSRC_NONE,       "None" },
	{ SF_CLKSRC_OCXO,       "ocxo" },
	{ SF_CLKSRC_TCXO,       "tcxo" },
	{ SF_CLKSRC_EXT,        "ext" },
	{ SF_CLKSRC_GPS,        "gps" },
	{ SF_CLKSRC_TRX,        "trx" },
	{ SF_CLKSRC_RX,         "rx" },
#endif
	{ 0, NULL }
};

const struct value_string femtobts_dir_names[] = {
	{ GsmL1_Dir_TxDownlink,	"TxDL" },
	{ GsmL1_Dir_TxUplink,	"TxUL" },
	{ GsmL1_Dir_RxUplink,	"RxUL" },
	{ GsmL1_Dir_RxDownlink,	"RxDL" },
	{ GsmL1_Dir_TxDownlink|GsmL1_Dir_RxUplink, "BOTH" },
	{ 0, NULL }
};

const struct value_string femtobts_chcomb_names[] = {
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
