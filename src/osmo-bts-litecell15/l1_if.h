#ifndef _L1_IF_H
#define _L1_IF_H

#include <osmocom/core/select.h>
#include <osmocom/core/write_queue.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/timer.h>
#include <osmocom/gsm/gsm_utils.h>

#include <osmo-bts/phy_link.h>

#include <nrw/litecell15/gsml1prim.h>
#include <nrw/litecell15/gsml1types.h>

#include <stdbool.h>

enum {
	MQ_SYS_READ,
	MQ_L1_READ,
	MQ_TCH_READ,
	MQ_PDTCH_READ,
	_NUM_MQ_READ
};

enum {
	MQ_SYS_WRITE,
	MQ_L1_WRITE,
	MQ_TCH_WRITE,
	MQ_PDTCH_WRITE,
	_NUM_MQ_WRITE
};

struct calib_send_state {
	FILE *fp;
	const char *path;
	int last_file_idx;
};

struct lc15l1_hdl {
	struct gsm_time gsm_time;
	HANDLE hLayer1;				/* handle to the L1 instance in the DSP */
	uint32_t dsp_trace_f;			/* currently operational DSP trace flags */
	struct llist_head wlc_list;

	struct phy_instance *phy_inst;

	struct osmo_timer_list alive_timer;
	unsigned int alive_prim_cnt;

	struct osmo_fd read_ofd[_NUM_MQ_READ];	/* osmo file descriptors */
	struct osmo_wqueue write_q[_NUM_MQ_WRITE];

	struct {
		/* from DSP/FPGA after L1 Init */
		uint8_t dsp_version[3];
		uint8_t fpga_version[3];
		uint32_t band_support;
		uint8_t ver_major;
		uint8_t ver_minor;
	} hw_info;

	struct calib_send_state st;

	uint8_t last_rf_mute[8];
};

#define msgb_l1prim(msg)	((GsmL1_Prim_t *)(msg)->l1h)
#define msgb_sysprim(msg)	((Litecell15_Prim_t *)(msg)->l1h)

typedef int l1if_compl_cb(struct gsm_bts_trx *trx, struct msgb *l1_msg, void *data);

/* send a request primitive to the L1 and schedule completion call-back */
int l1if_req_compl(struct lc15l1_hdl *fl1h, struct msgb *msg,
		   l1if_compl_cb *cb, void *cb_data);
int l1if_gsm_req_compl(struct lc15l1_hdl *fl1h, struct msgb *msg,
		l1if_compl_cb *cb, void *cb_data);

struct lc15l1_hdl *l1if_open(struct phy_instance *pinst);
int l1if_close(struct lc15l1_hdl *hdl);
int l1if_reset(struct lc15l1_hdl *hdl);
int l1if_activate_rf(struct lc15l1_hdl *hdl, int on);
int l1if_set_trace_flags(struct lc15l1_hdl *hdl, uint32_t flags);
int l1if_set_txpower(struct lc15l1_hdl *fl1h, float tx_power);
int l1if_mute_rf(struct lc15l1_hdl *hdl, uint8_t mute[8], l1if_compl_cb *cb);

struct msgb *l1p_msgb_alloc(void);
struct msgb *sysp_msgb_alloc(void);

uint32_t l1if_lchan_to_hLayer(struct gsm_lchan *lchan);
struct gsm_lchan *l1if_hLayer_to_lchan(struct gsm_bts_trx *trx, uint32_t hLayer);

/* tch.c */
int l1if_tch_encode(struct gsm_lchan *lchan, uint8_t *data, uint8_t *len,
		    const uint8_t *rtp_pl, unsigned int rtp_pl_len, uint32_t fn,
		    bool use_cache, bool marker);
int l1if_tch_rx(struct gsm_bts_trx *trx, uint8_t chan_nr, struct msgb *l1p_msg);
int l1if_tch_fill(struct gsm_lchan *lchan, uint8_t *l1_buffer);
struct msgb *gen_empty_tch_msg(struct gsm_lchan *lchan, uint32_t fn);

/* ciphering */
int l1if_set_ciphering(struct lc15l1_hdl *fl1h,
			  struct gsm_lchan *lchan,
			  int dir_downlink);

/* channel control */
int l1if_rsl_chan_act(struct gsm_lchan *lchan);
int l1if_rsl_chan_rel(struct gsm_lchan *lchan);
int l1if_rsl_chan_mod(struct gsm_lchan *lchan);
int l1if_rsl_deact_sacch(struct gsm_lchan *lchan);
int l1if_rsl_mode_modify(struct gsm_lchan *lchan);

/* calibration loading */
int calib_load(struct lc15l1_hdl *fl1h);

/* public helpers for test */
int bts_check_for_ciph_cmd(struct lc15l1_hdl *fl1h,
			      struct msgb *msg, struct gsm_lchan *lchan);
int l1if_ms_pwr_ctrl(struct gsm_lchan *lchan, const int uplink_target,
			const uint8_t ms_power, const float rxLevel);

static inline struct lc15l1_hdl *trx_lc15l1_hdl(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	OSMO_ASSERT(pinst);
	return pinst->u.lc15.hdl;
}

static inline struct gsm_bts_trx *lc15l1_hdl_trx(struct lc15l1_hdl *fl1h)
{
	OSMO_ASSERT(fl1h->phy_inst);
	return fl1h->phy_inst->trx;
}

#endif /* _L1_IF_H */
