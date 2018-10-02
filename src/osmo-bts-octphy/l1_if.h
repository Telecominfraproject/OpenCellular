#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/if_packet.h>

#include <osmocom/core/write_queue.h>
#include <osmocom/core/timer.h>

#include <osmocom/gsm/gsm_utils.h>
#include <osmocom/gsm/protocol/gsm_04_08.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>

#include <octphy/octvc1/gsm/octvc1_gsm_api.h>

#define BER_10K	10000

struct octphy_hdl {
	/* MAC address of the PHY */
	struct sockaddr_ll phy_addr;

	/* packet socket to talk with PHY */
	struct osmo_wqueue phy_wq;

	/* address parameters of the PHY */
	uint32_t session_id;
	uint32_t next_trans_id;
	uint32_t socket_id;

	/* clock manager state */
	uint32_t clkmgr_state;

	struct {
		struct {
			char *name;
			char *description;
			char *version;
		} app;
		struct {
			char *platform;
			char *version;
		} system;
	} info;

	/* This is a list of outstanding commands sent to the PHY, for which we
	 * currently still wait for a response. Represented by 'struct
	 * wait_l1_conf' in l1_if.c - Octasic calls this the 'Unacknowledged
	 * Command Window' */
	struct llist_head wlc_list;
	int wlc_list_len;
	struct {
		/* messages retransmitted due to discontinuity of transaction
		 * ID in responses from PHY */
		uint32_t retrans_cmds_trans_id;
		/* messages retransmitted due to supervisory messages by PHY */
		uint32_t retrans_cmds_supv;
		/* number of commands/wlcs that we ever had to postpone */
		uint32_t wlc_postponed;
	} stats;

	/* This is a list of wait_la_conf that OsmoBTS wanted to transmit to
	 * the PHY, but which couldn't yet been sent as the unacknowledged
	 * command window was full. */
	struct llist_head wlc_postponed;
	int wlc_postponed_len;

	/* back pointer to the PHY link */
	struct phy_link *phy_link;

	struct osmo_timer_list alive_timer;
	uint32_t alive_prim_cnt;

	/* were we already (re)opened after OsmoBTS start */
	int opened;
};

void l1if_fill_msg_hdr(tOCTVC1_MSG_HEADER *mh, struct msgb *msg,
			struct octphy_hdl *fl1h, uint32_t msg_type, uint32_t api_cmd);

typedef int l1if_compl_cb(struct octphy_hdl *fl1, struct msgb *l1_msg, void *data);

/* send a request primitive to the L1 and schedule completion call-back */
int l1if_req_compl(struct octphy_hdl *fl1h, struct msgb *msg,
		   l1if_compl_cb *cb, void *data);

#include <octphy/octvc1/gsm/octvc1_gsm_api.h>
struct gsm_lchan *get_lchan_by_lchid(struct gsm_bts_trx *trx,
				tOCTVC1_GSM_LOGICAL_CHANNEL_ID *lch_id);

struct octphy_hdl *l1if_open(struct phy_link *plink);
int l1if_close(struct octphy_hdl *hdl);

int l1if_trx_open(struct gsm_bts_trx *trx);
int l1if_trx_close_all(struct gsm_bts *bts);
int l1if_enable_events(struct gsm_bts_trx *trx);

int l1if_activate_rf(struct gsm_bts_trx *trx, int on);

int l1if_tch_rx(struct gsm_bts_trx *trx, uint8_t chan_nr,
		tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EVT *
		data_ind);

struct gsm_bts_trx *trx_by_l1h(struct octphy_hdl *fl1h, unsigned int trx_id);

struct msgb *l1p_msgb_alloc(void);

/* tch.c */
void l1if_tch_encode(struct gsm_lchan *lchan, uint32_t *payload_type,
		     uint8_t *data, uint32_t *len, const uint8_t *rtp_pl,
		     unsigned int rtp_pl_len);

tOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM
osmocom_to_octphy_band(enum gsm_band osmo_band, unsigned int arfcn);
