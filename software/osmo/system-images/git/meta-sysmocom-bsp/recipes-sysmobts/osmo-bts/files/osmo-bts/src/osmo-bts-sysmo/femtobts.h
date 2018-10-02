#ifndef FEMTOBTS_H
#define FEMTOBTS_H

#include <stdlib.h>
#include <osmocom/core/utils.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1const.h>

#ifdef FEMTOBTS_API_VERSION
#define SuperFemto_PrimId_t FemtoBts_PrimId_t
#define SuperFemto_Prim_t FemtoBts_Prim_t
#define SuperFemto_PrimId_SystemInfoReq 	FemtoBts_PrimId_SystemInfoReq
#define SuperFemto_PrimId_SystemInfoCnf		FemtoBts_PrimId_SystemInfoCnf
#define SuperFemto_SystemInfoCnf_t		FemtoBts_SystemInfoCnf_t
#define SuperFemto_PrimId_SystemFailureInd	FemtoBts_PrimId_SystemFailureInd
#define SuperFemto_PrimId_ActivateRfReq		FemtoBts_PrimId_ActivateRfReq
#define SuperFemto_PrimId_ActivateRfCnf		FemtoBts_PrimId_ActivateRfCnf
#define SuperFemto_PrimId_DeactivateRfReq	FemtoBts_PrimId_DeactivateRfReq
#define SuperFemto_PrimId_DeactivateRfCnf	FemtoBts_PrimId_DeactivateRfCnf
#define SuperFemto_PrimId_SetTraceFlagsReq	FemtoBts_PrimId_SetTraceFlagsReq
#define SuperFemto_PrimId_RfClockInfoReq	FemtoBts_PrimId_RfClockInfoReq
#define SuperFemto_PrimId_RfClockInfoCnf	FemtoBts_PrimId_RfClockInfoCnf
#define SuperFemto_PrimId_RfClockSetupReq	FemtoBts_PrimId_RfClockSetupReq
#define SuperFemto_PrimId_RfClockSetupCnf	FemtoBts_PrimId_RfClockSetupCnf
#define SuperFemto_PrimId_Layer1ResetReq	FemtoBts_PrimId_Layer1ResetReq
#define SuperFemto_PrimId_Layer1ResetCnf	FemtoBts_PrimId_Layer1ResetCnf
#define SuperFemto_PrimId_NUM			FemtoBts_PrimId_NUM
#define HW_SYSMOBTS_V1				1
#define SUPERFEMTO_API(x,y,z)			FEMTOBTS_API(x,y,z)
#endif

#ifdef L1_HAS_RTP_MODE
/*
 * The bit ordering has been fixed on >= 3.10 but I am verifying
 * this on 3.11.
 */
#if SUPERFEMTO_API_VERSION >= SUPERFEMTO_API(3, 11, 0)
#define USE_L1_RTP_MODE		/* Tell L1 to use RTP mode */
#endif
#endif

/*
 * Depending on the firmware version either GsmL1_Prim_t or SuperFemto_Prim_t
 * is the bigger struct. For earlier firmware versions the GsmL1_Prim_t was the
 * bigger struct.
 */
#define SYSMOBTS_PRIM_SIZE \
	(OSMO_MAX(sizeof(SuperFemto_Prim_t), sizeof(GsmL1_Prim_t)) + 128)

enum l1prim_type {
	L1P_T_INVALID, /* this must be 0 to detect uninitialized elements */
	L1P_T_REQ,
	L1P_T_CONF,
	L1P_T_IND,
};

#if !defined(SUPERFEMTO_API_VERSION) || SUPERFEMTO_API_VERSION < SUPERFEMTO_API(2,1,0)
enum uperfemto_clk_src {
	SF_CLKSRC_NONE  = 0,
	SF_CLKSRC_OCXO  = 1,
	SF_CLKSRC_TCXO  = 2,
	SF_CLKSRC_EXT   = 3,
	SF_CLKSRC_GPS   = 4,
	SF_CLKSRC_TRX   = 5,
	SF_CLKSRC_RX    = 6,
	SF_CLKSRC_NL    = 7,
};
#endif

const enum l1prim_type femtobts_l1prim_type[GsmL1_PrimId_NUM];
const struct value_string femtobts_l1prim_names[GsmL1_PrimId_NUM+1];
const GsmL1_PrimId_t femtobts_l1prim_req2conf[GsmL1_PrimId_NUM];

const enum l1prim_type femtobts_sysprim_type[SuperFemto_PrimId_NUM];
const struct value_string femtobts_sysprim_names[SuperFemto_PrimId_NUM+1];
const SuperFemto_PrimId_t femtobts_sysprim_req2conf[SuperFemto_PrimId_NUM];

const struct value_string femtobts_l1sapi_names[GsmL1_Sapi_NUM+1];
const struct value_string femtobts_l1status_names[GSML1_STATUS_NUM+1];

const struct value_string femtobts_tracef_names[29];
const struct value_string femtobts_tracef_docs[29];

const struct value_string femtobts_tch_pl_names[15];
const struct value_string femtobts_chcomb_names[8];
const struct value_string femtobts_clksrc_names[10];

const struct value_string femtobts_dir_names[6];

enum pdch_cs {
	PDCH_CS_1,
	PDCH_CS_2,
	PDCH_CS_3,
	PDCH_CS_4,
	PDCH_MCS_1,
	PDCH_MCS_2,
	PDCH_MCS_3,
	PDCH_MCS_4,
	PDCH_MCS_5,
	PDCH_MCS_6,
	PDCH_MCS_7,
	PDCH_MCS_8,
	PDCH_MCS_9,
	_NUM_PDCH_CS
};

const uint8_t pdch_msu_size[_NUM_PDCH_CS];

#endif /* FEMTOBTS_H */
