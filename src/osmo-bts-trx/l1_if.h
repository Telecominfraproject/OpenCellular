#ifndef L1_IF_H_TRX
#define L1_IF_H_TRX

#include <osmo-bts/scheduler.h>
#include <osmo-bts/phy_link.h>
#include "trx_if.h"

struct trx_config {
	uint8_t			poweron;	/* poweron(1) or poweroff(0) */
	int			poweron_sent;

	int			arfcn_valid;
	uint16_t		arfcn;
	int			arfcn_sent;

	int			tsc_valid;
	uint8_t			tsc;
	int			tsc_sent;

	int			bsic_valid;
	uint8_t			bsic;
	int			bsic_sent;

	int			rxgain_valid;
	uint8_t			rxgain;
	int			rxgain_sent;

	int			power_valid;
	uint8_t			power;
	int			power_oml;
	int			power_sent;

	int			maxdly_valid;
	int			maxdly;
	int			maxdly_sent;

	int			maxdlynb_valid;
	int			maxdlynb;
	int			maxdlynb_sent;

	uint8_t			slotmask;

	int			slottype_valid[TRX_NR_TS];
	uint8_t			slottype[TRX_NR_TS];
	int			slottype_sent[TRX_NR_TS];
};

struct trx_l1h {
	struct llist_head	trx_ctrl_list;
	/* Latest RSPed cmd, used to catch duplicate RSPs from sent retransmissions */
	struct trx_ctrl_msg 	*last_acked;

	//struct gsm_bts_trx	*trx;
	struct phy_instance	*phy_inst;

	struct osmo_fd		trx_ofd_ctrl;
	struct osmo_timer_list	trx_ctrl_timer;
	struct osmo_fd		trx_ofd_data;

	/* transceiver config */
	struct trx_config	config;
	uint8_t			ho_rach_detect[TRX_NR_TS][TS_MAX_LCHAN];

	struct l1sched_trx	l1s;
};

int check_transceiver_availability(struct gsm_bts *bts, int avail);
int l1if_provision_transceiver_trx(struct trx_l1h *l1h);
int l1if_provision_transceiver(struct gsm_bts *bts);
int l1if_mph_time_ind(struct gsm_bts *bts, uint32_t fn);
int l1if_process_meas_res(struct gsm_bts_trx *trx, uint8_t tn, uint32_t fn, uint8_t chan_nr,
	int n_errors, int n_bits_total, float rssi, int16_t toa256);

static inline struct l1sched_trx *trx_l1sched_hdl(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx->role_bts.l1h;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	return &l1h->l1s;
}

#endif /* L1_IF_H_TRX */
