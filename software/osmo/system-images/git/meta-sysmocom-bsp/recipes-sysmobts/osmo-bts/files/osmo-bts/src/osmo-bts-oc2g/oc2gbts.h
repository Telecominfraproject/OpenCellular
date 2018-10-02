#ifndef OC2GBTS_H
#define OC2GBTS_H

#include <stdlib.h>
#include <osmocom/core/utils.h>
#include <osmocom/gsm/protocol/gsm_08_58.h>

#include <nrw/oc2g/oc2g.h>
#include <nrw/oc2g/gsml1const.h>

/*
 * Depending on the firmware version either GsmL1_Prim_t or Oc2g_Prim_t
 * is the bigger struct. For earlier firmware versions the GsmL1_Prim_t was the
 * bigger struct.
 */
#define OC2GBTS_PRIM_SIZE \
	(OSMO_MAX(sizeof(Oc2g_Prim_t), sizeof(GsmL1_Prim_t)) + 128)

enum l1prim_type {
	L1P_T_INVALID, /* this must be 0 to detect uninitialized elements */
	L1P_T_REQ,
	L1P_T_CONF,
	L1P_T_IND,
};

enum oc2g_pedestal_mode{
	OC2G_PEDESTAL_OFF = 0,
	OC2G_PEDESTAL_ON,
};

enum oc2g_led_control_mode{
	OC2G_LED_CONTROL_BTS = 0,
	OC2G_LED_CONTROL_EXT,
};

enum oc2g_auto_pwr_adjust_mode{
	OC2G_TX_PWR_ADJ_NONE = 0,
	OC2G_TX_PWR_ADJ_AUTO,
};

enum l1prim_type oc2gbts_get_l1prim_type(GsmL1_PrimId_t id);
const struct value_string oc2gbts_l1prim_names[GsmL1_PrimId_NUM+1];
GsmL1_PrimId_t oc2gbts_get_l1prim_conf(GsmL1_PrimId_t id);

enum l1prim_type oc2gbts_get_sysprim_type(Oc2g_PrimId_t id);
const struct value_string oc2gbts_sysprim_names[Oc2g_PrimId_NUM+1];
Oc2g_PrimId_t oc2gbts_get_sysprim_conf(Oc2g_PrimId_t id);

const struct value_string oc2gbts_l1sapi_names[GsmL1_Sapi_NUM+1];
const struct value_string oc2gbts_l1status_names[GSML1_STATUS_NUM+1];

const struct value_string oc2gbts_tracef_names[29];
const struct value_string oc2gbts_tracef_docs[29];

const struct value_string oc2gbts_tch_pl_names[15];

const struct value_string oc2gbts_clksrc_names[10];

const struct value_string oc2gbts_dir_names[6];

const struct value_string oc2gbts_rsl_ho_causes[IPAC_HO_RQD_CAUSE_MAX];

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

/* OC2G default parameters */
#define OC2G_BTS_MAX_CELL_SIZE_DEFAULT	166	/* 166 qbits is default  value */
#define OC2G_BTS_PEDESTAL_MODE_DEFAULT	0	/* Unused TS is off by default */
#define OC2G_BTS_LED_CTRL_MODE_DEFAULT	0	/* LED is controlled by BTS by default */
#define OC2G_BTS_DSP_ALIVE_TMR_DEFAULT	5	/* Default DSP alive timer is 5 seconds  */
#define OC2G_BTS_TX_PWR_ADJ_DEFAULT	0	/* Default Tx power auto adjustment is none */
#define OC2G_BTS_TX_RED_PWR_8PSK_DEFAULT	0	/* Default 8-PSK maximum power level is 0 dB */
#define OC2G_BTS_RTP_DRIFT_THRES_DEFAULT	0	/* Default RTP drift threshold is 0 ms (disabled) */
#define OC2G_BTS_TX_C0_IDLE_RED_PWR_DEFAULT	0	/* Default C0 idle slot reduction power level is 0 dB */

#endif /* OC2GBTS_H */
