#ifndef _GSM_DATA_H
#define _GSM_DATA_H

#include <osmocom/core/timer.h>
#include <osmocom/core/linuxlist.h>
#include <osmocom/gsm/lapdm.h>
#include <osmocom/gsm/gsm23003.h>

#include <osmo-bts/paging.h>
#include <osmo-bts/tx_power.h>

#define GSM_FR_BITS	260
#define GSM_EFR_BITS	244

#define GSM_FR_BYTES	33	/* TS 101318 Chapter 5.1: 260 bits + 4bit sig */
#define GSM_HR_BYTES	14	/* TS 101318 Chapter 5.2: 112 bits, no sig */
#define GSM_EFR_BYTES	31	/* TS 101318 Chapter 5.3: 244 bits + 4bit sig */

#define GSM_SUPERFRAME	(26*51)			/* 1326 TDMA frames */
#define GSM_HYPERFRAME	(2048*GSM_SUPERFRAME)	/* GSM_HYPERFRAME frames */

#define GSM_BTS_AGCH_QUEUE_THRESH_LEVEL_DEFAULT 41
#define GSM_BTS_AGCH_QUEUE_THRESH_LEVEL_DISABLE 999999
#define GSM_BTS_AGCH_QUEUE_LOW_LEVEL_DEFAULT 41
#define GSM_BTS_AGCH_QUEUE_HIGH_LEVEL_DEFAULT 91

struct gsm_network {
	struct llist_head bts_list;
	unsigned int num_bts;
	struct osmo_plmn_id plmn;
	struct pcu_sock_state *pcu_state;
};

enum lchan_ciph_state {
	LCHAN_CIPH_NONE,
	LCHAN_CIPH_RX_REQ,
	LCHAN_CIPH_RX_CONF,
	LCHAN_CIPH_RXTX_REQ,
	LCHAN_CIPH_RX_CONF_TX_REQ,
	LCHAN_CIPH_RXTX_CONF,
};

#include <osmo-bts/gsm_data_shared.h>

void lchan_set_state(struct gsm_lchan *lchan, enum gsm_lchan_state state);
int conf_lchans_as_pchan(struct gsm_bts_trx_ts *ts,
			 enum gsm_phys_chan_config pchan);

/* cipher code */
#define CIPHER_A5(x) (1 << (x-1))

int bts_supports_cipher(struct gsm_bts *bts, int rsl_cipher);

bool ts_is_pdch(const struct gsm_bts_trx_ts *ts);

int bts_model_check_cm_mode(enum gsm_phys_chan_config pchan, enum gsm48_chan_mode cm);

#endif /* _GSM_DATA_H */
