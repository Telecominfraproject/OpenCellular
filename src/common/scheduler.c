/* Scheduler for OsmoBTS-TRX */

/* (C) 2013 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2015 by Alexander Chemeris <Alexander.Chemeris@fairwaves.co>
 * (C) 2015 by Harald Welte <laforge@gnumonks.org>
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/bits.h>
#include <osmocom/gsm/a5.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/scheduler.h>
#include <osmo-bts/scheduler_backend.h>

extern void *tall_bts_ctx;

static int rts_data_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan);
static int rts_tchf_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan);
static int rts_tchh_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan);
/*! \brief Dummy Burst (TS 05.02 Chapter 5.2.6) */
static const ubit_t dummy_burst[GSM_BURST_LEN] = {
	0,0,0,
	1,1,1,1,1,0,1,1,0,1,1,1,0,1,1,0,0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,0,
	0,0,0,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,1,1,1,0,0,
	0,1,0,1,1,1,0,0,0,1,0,1,1,1,0,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,0,
	0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,1,
	0,0,1,0,1,1,1,1,1,0,1,0,1,0,
	0,0,0,
};

/*! \brief FCCH Burst (TS 05.02 Chapter 5.2.4) */
const ubit_t _sched_fcch_burst[GSM_BURST_LEN] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/*! \brief Training Sequences (TS 05.02 Chapter 5.2.3) */
const ubit_t _sched_tsc[8][26] = {
	{ 0,0,1,0,0,1,0,1,1,1,0,0,0,0,1,0,0,0,1,0,0,1,0,1,1,1, },
	{ 0,0,1,0,1,1,0,1,1,1,0,1,1,1,1,0,0,0,1,0,1,1,0,1,1,1, },
	{ 0,1,0,0,0,0,1,1,1,0,1,1,1,0,1,0,0,1,0,0,0,0,1,1,1,0, },
	{ 0,1,0,0,0,1,1,1,1,0,1,1,0,1,0,0,0,1,0,0,0,1,1,1,1,0, },
	{ 0,0,0,1,1,0,1,0,1,1,1,0,0,1,0,0,0,0,0,1,1,0,1,0,1,1, },
	{ 0,1,0,0,1,1,1,0,1,0,1,1,0,0,0,0,0,1,0,0,1,1,1,0,1,0, },
	{ 1,0,1,0,0,1,1,1,1,1,0,1,1,0,0,0,1,0,1,0,0,1,1,1,1,1, },
	{ 1,1,1,0,1,1,1,1,0,0,0,1,0,0,1,0,1,1,1,0,1,1,1,1,0,0, },
};

const ubit_t _sched_egprs_tsc[8][78] = {
	{ 1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,0,0,
	  1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,
	  1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,0,0,1,0,0,1, },
	{ 1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,0,0,1,1,1,1,0,0,1,0,0,
	  1,0,0,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,1,1,1,1,1,1,1,1,
	  1,1,0,0,1,1,1,1,0,0,1,0,0,1,1,1,1,0,0,1,0,0,1,0,0,1, },
	{ 1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,
	  1,1,1,1,0,0,1,0,0,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,1,0,
	  0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,1,1,1, },
	{ 1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,
	  1,1,1,1,0,0,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,
	  0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,1,1,1,1, },
	{ 1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,1,1,1,0,0,1,1,1,1,0,0,
	  1,0,0,1,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,
	  1,1,1,1,1,0,0,1,0,0,1,1,1,1,0,0,1,1,1,1,0,0,1,0,0,1, },
	{ 1,1,1,0,0,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,1,1,1,0,0,
	  1,1,1,1,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	  0,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,1,1,1,0,0,1,1,1,1, },
	{ 0,0,1,1,1,1,0,0,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,
	  1,0,0,1,1,1,1,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,
	  1,1,0,0,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1, },
	{ 0,0,1,0,0,1,0,0,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,1,1,1,
	  1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,0,
	  0,1,0,0,1,1,1,1,0,0,1,0,0,1,0,0,1,0,0,1,1,1,1,1,1,1, },
};

/*! \brief SCH training sequence (TS 05.02 Chapter 5.2.5) */
const ubit_t _sched_sch_train[64] = {
	1,0,1,1,1,0,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,1,1,
	0,0,1,0,1,1,0,1,0,1,0,0,0,1,0,1,0,1,1,1,0,1,1,0,0,0,0,1,1,0,1,1,
};

/*
 * subchannel description structure
 */

const struct trx_chan_desc trx_chan_desc[_TRX_CHAN_MAX] = {
  /*   	is_pdch	chan_type	chan_nr	link_id		name		rts_fn		dl_fn		ul_fn	auto_active */
      {	0,	TRXC_IDLE,	0,	LID_DEDIC,	"IDLE",		NULL,		tx_idle_fn,	NULL,		1 },
      {	0,	TRXC_FCCH,	0,	LID_DEDIC,	"FCCH",		NULL,		tx_fcch_fn,	NULL,		1 },
      {	0,	TRXC_SCH,	0,	LID_DEDIC,	"SCH",		NULL,		tx_sch_fn,	NULL,		1 },
      {	0,	TRXC_BCCH,	0x80,	LID_DEDIC,	"BCCH",		rts_data_fn,	tx_data_fn,	NULL,		1 },
      {	0,	TRXC_RACH,	0x88,	LID_DEDIC,	"RACH",		NULL,		NULL,		rx_rach_fn,	1 },
      {	0,	TRXC_CCCH,	0x90,	LID_DEDIC,	"CCCH",		rts_data_fn,	tx_data_fn,	NULL,		1 },
      {	0,	TRXC_TCHF,	0x08,	LID_DEDIC,	"TCH/F",	rts_tchf_fn,	tx_tchf_fn,	rx_tchf_fn,	0 },
      {	0,	TRXC_TCHH_0,	0x10,	LID_DEDIC,	"TCH/H(0)",	rts_tchh_fn,	tx_tchh_fn,	rx_tchh_fn,	0 },
      {	0,	TRXC_TCHH_1,	0x18,	LID_DEDIC,	"TCH/H(1)",	rts_tchh_fn,	tx_tchh_fn,	rx_tchh_fn,	0 },
      {	0,	TRXC_SDCCH4_0,	0x20,	LID_DEDIC,	"SDCCH/4(0)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH4_1,	0x28,	LID_DEDIC,	"SDCCH/4(1)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH4_2,	0x30,	LID_DEDIC,	"SDCCH/4(2)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH4_3,	0x38,	LID_DEDIC,	"SDCCH/4(3)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_0,	0x40,	LID_DEDIC,	"SDCCH/8(0)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_1,	0x48,	LID_DEDIC,	"SDCCH/8(1)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_2,	0x50,	LID_DEDIC,	"SDCCH/8(2)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_3,	0x58,	LID_DEDIC,	"SDCCH/8(3)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_4,	0x60,	LID_DEDIC,	"SDCCH/8(4)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_5,	0x68,	LID_DEDIC,	"SDCCH/8(5)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_6,	0x70,	LID_DEDIC,	"SDCCH/8(6)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SDCCH8_7,	0x78,	LID_DEDIC,	"SDCCH/8(7)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCHTF,	0x08,	LID_SACCH,	"SACCH/TF",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCHTH_0,	0x10,	LID_SACCH,	"SACCH/TH(0)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCHTH_1,	0x18,	LID_SACCH,	"SACCH/TH(1)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH4_0, 	0x20,	LID_SACCH,	"SACCH/4(0)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH4_1,	0x28,	LID_SACCH,	"SACCH/4(1)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH4_2,	0x30,	LID_SACCH,	"SACCH/4(2)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH4_3,	0x38,	LID_SACCH,	"SACCH/4(3)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_0,	0x40,	LID_SACCH,	"SACCH/8(0)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_1,	0x48,	LID_SACCH,	"SACCH/8(1)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_2,	0x50,	LID_SACCH,	"SACCH/8(2)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_3,	0x58,	LID_SACCH,	"SACCH/8(3)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_4,	0x60,	LID_SACCH,	"SACCH/8(4)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_5,	0x68,	LID_SACCH,	"SACCH/8(5)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_6,	0x70,	LID_SACCH,	"SACCH/8(6)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_SACCH8_7,	0x78,	LID_SACCH,	"SACCH/8(7)",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	1,	TRXC_PDTCH,	0xc0,	LID_DEDIC,	"PDTCH",	rts_data_fn,	tx_pdtch_fn,	rx_pdtch_fn,	0 },
      {	1,	TRXC_PTCCH,	0xc0,	LID_DEDIC,	"PTCCH",	rts_data_fn,	tx_data_fn,	rx_data_fn,	0 },
      {	0,	TRXC_CBCH,	0xc8,	LID_DEDIC,	"CBCH",		rts_data_fn,	tx_data_fn,	NULL,		1 },
};

const struct value_string trx_chan_type_names[] = {
	OSMO_VALUE_STRING(TRXC_IDLE),
	OSMO_VALUE_STRING(TRXC_FCCH),
	OSMO_VALUE_STRING(TRXC_SCH),
	OSMO_VALUE_STRING(TRXC_BCCH),
	OSMO_VALUE_STRING(TRXC_RACH),
	OSMO_VALUE_STRING(TRXC_CCCH),
	OSMO_VALUE_STRING(TRXC_TCHF),
	OSMO_VALUE_STRING(TRXC_TCHH_0),
	OSMO_VALUE_STRING(TRXC_TCHH_1),
	OSMO_VALUE_STRING(TRXC_SDCCH4_0),
	OSMO_VALUE_STRING(TRXC_SDCCH4_1),
	OSMO_VALUE_STRING(TRXC_SDCCH4_2),
	OSMO_VALUE_STRING(TRXC_SDCCH4_3),
	OSMO_VALUE_STRING(TRXC_SDCCH8_0),
	OSMO_VALUE_STRING(TRXC_SDCCH8_1),
	OSMO_VALUE_STRING(TRXC_SDCCH8_2),
	OSMO_VALUE_STRING(TRXC_SDCCH8_3),
	OSMO_VALUE_STRING(TRXC_SDCCH8_4),
	OSMO_VALUE_STRING(TRXC_SDCCH8_5),
	OSMO_VALUE_STRING(TRXC_SDCCH8_6),
	OSMO_VALUE_STRING(TRXC_SDCCH8_7),
	OSMO_VALUE_STRING(TRXC_SACCHTF),
	OSMO_VALUE_STRING(TRXC_SACCHTH_0),
	OSMO_VALUE_STRING(TRXC_SACCHTH_1),
	OSMO_VALUE_STRING(TRXC_SACCH4_0),
	OSMO_VALUE_STRING(TRXC_SACCH4_1),
	OSMO_VALUE_STRING(TRXC_SACCH4_2),
	OSMO_VALUE_STRING(TRXC_SACCH4_3),
	OSMO_VALUE_STRING(TRXC_SACCH8_0),
	OSMO_VALUE_STRING(TRXC_SACCH8_1),
	OSMO_VALUE_STRING(TRXC_SACCH8_2),
	OSMO_VALUE_STRING(TRXC_SACCH8_3),
	OSMO_VALUE_STRING(TRXC_SACCH8_4),
	OSMO_VALUE_STRING(TRXC_SACCH8_5),
	OSMO_VALUE_STRING(TRXC_SACCH8_6),
	OSMO_VALUE_STRING(TRXC_SACCH8_7),
	OSMO_VALUE_STRING(TRXC_PDTCH),
	OSMO_VALUE_STRING(TRXC_PTCCH),
	OSMO_VALUE_STRING(TRXC_CBCH),
	OSMO_VALUE_STRING(_TRX_CHAN_MAX),
 	{ 0, NULL }
};

/*
 * init / exit
 */

int trx_sched_init(struct l1sched_trx *l1t, struct gsm_bts_trx *trx)
{
	uint8_t tn;
	unsigned int i;

	if (!trx)
		return -EINVAL;

	l1t->trx = trx;

	LOGP(DL1C, LOGL_NOTICE, "Init scheduler for trx=%u\n", l1t->trx->nr);

	for (tn = 0; tn < ARRAY_SIZE(l1t->ts); tn++) {
		struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);

		l1ts->mf_index = 0;
		l1ts->mf_last_fn = 0;
		INIT_LLIST_HEAD(&l1ts->dl_prims);
		for (i = 0; i < ARRAY_SIZE(l1ts->chan_state); i++) {
			struct l1sched_chan_state *chan_state;
			chan_state = &l1ts->chan_state[i];
			chan_state->active = 0;
		}
	}

	return 0;
}

void trx_sched_exit(struct l1sched_trx *l1t)
{
	struct gsm_bts_trx_ts *ts;
	uint8_t tn;
	int i;

	LOGP(DL1C, LOGL_NOTICE, "Exit scheduler for trx=%u\n", l1t->trx->nr);

	for (tn = 0; tn < ARRAY_SIZE(l1t->ts); tn++) {
		struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
		msgb_queue_flush(&l1ts->dl_prims);
		for (i = 0; i < _TRX_CHAN_MAX; i++) {
			struct l1sched_chan_state *chan_state;
			chan_state = &l1ts->chan_state[i];
			if (chan_state->dl_bursts) {
				talloc_free(chan_state->dl_bursts);
				chan_state->dl_bursts = NULL;
			}
			if (chan_state->ul_bursts) {
				talloc_free(chan_state->ul_bursts);
				chan_state->ul_bursts = NULL;
			}
		}
		/* clear lchan channel states */
		ts = &l1t->trx->ts[tn];
		for (i = 0; i < ARRAY_SIZE(ts->lchan); i++)
			lchan_set_state(&ts->lchan[i], LCHAN_S_NONE);
	}
}

/* close all logical channels and reset timeslots */
void trx_sched_reset(struct l1sched_trx *l1t)
{
	trx_sched_exit(l1t);
	trx_sched_init(l1t, l1t->trx);
}

struct msgb *_sched_dequeue_prim(struct l1sched_trx *l1t, int8_t tn, uint32_t fn,
				 enum trx_chan_type chan)
{
	struct msgb *msg, *msg2;
	struct osmo_phsap_prim *l1sap;
	uint32_t prim_fn;
	uint8_t chan_nr, link_id;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);

	/* get prim of current fn from queue */
	llist_for_each_entry_safe(msg, msg2, &l1ts->dl_prims, list) {
		l1sap = msgb_l1sap_prim(msg);
		if (l1sap->oph.operation != PRIM_OP_REQUEST) {
wrong_type:
			LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "Prim has wrong type.\n");
free_msg:
			/* unlink and free message */
			llist_del(&msg->list);
			msgb_free(msg);
			return NULL;
		}
		switch (l1sap->oph.primitive) {
		case PRIM_PH_DATA:
			chan_nr = l1sap->u.data.chan_nr;
			link_id = l1sap->u.data.link_id;
			prim_fn = ((l1sap->u.data.fn + GSM_HYPERFRAME - fn) % GSM_HYPERFRAME);
			break;
		case PRIM_TCH:
			chan_nr = l1sap->u.tch.chan_nr;
			link_id = 0;
			prim_fn = ((l1sap->u.tch.fn + GSM_HYPERFRAME - fn) % GSM_HYPERFRAME);
			break;
		default:
			goto wrong_type;
		}
		if (prim_fn > 100) {
			LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn,
			     "Prim %u is out of range (100), or channel %s with "
			     "type %s is already disabled. If this happens in "
			     "conjunction with PCU, increase 'rts-advance' by 5.\n",
			     prim_fn, get_lchan_by_chan_nr(l1t->trx, chan_nr)->name,
			     get_value_string(trx_chan_type_names, chan));
			/* unlink and free message */
			llist_del(&msg->list);
			msgb_free(msg);
			continue;
		}
		if (prim_fn > 0)
			continue;

		goto found_msg;
	}

	return NULL;

found_msg:
	if ((chan_nr ^ (trx_chan_desc[chan].chan_nr | tn))
	 || ((link_id & 0xc0) ^ trx_chan_desc[chan].link_id)) {
		LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "Prim has wrong chan_nr=0x%02x link_id=%02x, "
			"expecting chan_nr=0x%02x link_id=%02x.\n", chan_nr, link_id,
			trx_chan_desc[chan].chan_nr | tn, trx_chan_desc[chan].link_id);
		goto free_msg;
	}

	/* unlink and return message */
	llist_del(&msg->list);
	return msg;
}

int _sched_compose_ph_data_ind(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
			       enum trx_chan_type chan, uint8_t *l2,
			       uint8_t l2_len, float rssi,
			       int16_t ta_offs_256bits, int16_t link_qual_cb,
			       uint16_t ber10k,
			       enum osmo_ph_pres_info_type presence_info)
{
	struct msgb *msg;
	struct osmo_phsap_prim *l1sap;
	uint8_t chan_nr = trx_chan_desc[chan].chan_nr | tn;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);

	/* compose primitive */
	msg = l1sap_msgb_alloc(l2_len);
	l1sap = msgb_l1sap_prim(msg);
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_DATA,
		PRIM_OP_INDICATION, msg);
	l1sap->u.data.chan_nr = chan_nr;
	l1sap->u.data.link_id = trx_chan_desc[chan].link_id;
	l1sap->u.data.fn = fn;
	l1sap->u.data.rssi = (int8_t) (rssi);
	l1sap->u.data.ber10k = ber10k;
	l1sap->u.data.ta_offs_256bits = ta_offs_256bits;
	l1sap->u.data.lqual_cb = link_qual_cb;
	l1sap->u.data.pdch_presence_info = presence_info;
	msg->l2h = msgb_put(msg, l2_len);
	if (l2_len)
		memcpy(msg->l2h, l2, l2_len);

	if (L1SAP_IS_LINK_SACCH(trx_chan_desc[chan].link_id))
		l1ts->chan_state[chan].lost_frames = 0;

	/* forward primitive */
	l1sap_up(l1t->trx, l1sap);

	return 0;
}

int _sched_compose_tch_ind(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
		    enum trx_chan_type chan, uint8_t *tch, uint8_t tch_len)
{
	struct msgb *msg;
	struct osmo_phsap_prim *l1sap;
	struct gsm_bts_trx *trx = l1t->trx;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	uint8_t chan_nr = trx_chan_desc[chan].chan_nr | tn;
	struct gsm_lchan *lchan = &trx->ts[L1SAP_CHAN2TS(chan_nr)].lchan[l1sap_chan2ss(chan_nr)];

	/* compose primitive */
	msg = l1sap_msgb_alloc(tch_len);
	l1sap = msgb_l1sap_prim(msg);
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_TCH,
		PRIM_OP_INDICATION, msg);
	l1sap->u.tch.chan_nr = chan_nr;
	l1sap->u.tch.fn = fn;
	msg->l2h = msgb_put(msg, tch_len);
	if (tch_len)
		memcpy(msg->l2h, tch, tch_len);

	if (l1ts->chan_state[chan].lost_frames)
		l1ts->chan_state[chan].lost_frames--;

	LOGL1S(DL1P, LOGL_DEBUG, l1t, tn, -1, l1sap->u.data.fn,
	       "%s Rx -> RTP: %s\n",
	       gsm_lchan_name(lchan), osmo_hexdump(msgb_l2(msg), msgb_l2len(msg)));
	/* forward primitive */
	l1sap_up(l1t->trx, l1sap);

	return 0;
}



/*
 * data request (from upper layer)
 */

int trx_sched_ph_data_req(struct l1sched_trx *l1t, struct osmo_phsap_prim *l1sap)
{
	uint8_t tn = l1sap->u.data.chan_nr & 7;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);

	LOGL1S(DL1P, LOGL_INFO, l1t, tn, -1, l1sap->u.data.fn,
		"PH-DATA.req: chan_nr=0x%02x link_id=0x%02x\n",
		l1sap->u.data.chan_nr, l1sap->u.data.link_id);

	if (!l1sap->oph.msg)
		abort();

	/* ignore empty frame */
	if (!msgb_l2len(l1sap->oph.msg)) {
		msgb_free(l1sap->oph.msg);
		return 0;
	}

	msgb_enqueue(&l1ts->dl_prims, l1sap->oph.msg);

	return 0;
}

int trx_sched_tch_req(struct l1sched_trx *l1t, struct osmo_phsap_prim *l1sap)
{
	uint8_t tn = l1sap->u.tch.chan_nr & 7;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);

	LOGL1S(DL1P, LOGL_INFO, l1t, tn, -1, l1sap->u.tch.fn, "TCH.req: chan_nr=0x%02x\n",
		l1sap->u.tch.chan_nr);

	if (!l1sap->oph.msg)
		abort();

	/* ignore empty frame */
	if (!msgb_l2len(l1sap->oph.msg)) {
		msgb_free(l1sap->oph.msg);
		return 0;
	}

	msgb_enqueue(&l1ts->dl_prims, l1sap->oph.msg);

	return 0;
}


/* 
 * ready-to-send indication (to upper layer)
 */

/* RTS for data frame */
static int rts_data_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan)
{
	uint8_t chan_nr, link_id;
	struct msgb *msg;
	struct osmo_phsap_prim *l1sap;

	/* get data for RTS indication */
	chan_nr = trx_chan_desc[chan].chan_nr | tn;
	link_id = trx_chan_desc[chan].link_id;

	if (!chan_nr) {
		LOGL1S(DL1P, LOGL_FATAL, l1t, tn, chan, fn,
			"RTS func with non-existing chan_nr %d\n", chan_nr);
		return -ENODEV;
	}

	LOGL1S(DL1P, LOGL_INFO, l1t, tn, chan, fn,
		"PH-RTS.ind: chan_nr=0x%02x link_id=0x%02x\n", chan_nr, link_id);

	/* generate prim */
	msg = l1sap_msgb_alloc(200);
	if (!msg)
		return -ENOMEM;
	l1sap = msgb_l1sap_prim(msg);
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_RTS,
	                                PRIM_OP_INDICATION, msg);
	l1sap->u.data.chan_nr = chan_nr;
	l1sap->u.data.link_id = link_id;
	l1sap->u.data.fn = fn;

	return l1sap_up(l1t->trx, l1sap);
}

static int rts_tch_common(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, int facch)
{
	uint8_t chan_nr, link_id;
	struct msgb *msg;
	struct osmo_phsap_prim *l1sap;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	int rc = 0;

	/* get data for RTS indication */
	chan_nr = trx_chan_desc[chan].chan_nr | tn;
	link_id = trx_chan_desc[chan].link_id;

	if (!chan_nr) {
		LOGL1S(DL1P, LOGL_FATAL, l1t, tn, chan, fn,
			"RTS func with non-existing chan_nr %d\n", chan_nr);
		return -ENODEV;
	}

	LOGL1S(DL1P, LOGL_INFO, l1t, tn, chan, fn, "TCH RTS.ind: chan_nr=0x%02x\n", chan_nr);

	/* only send, if FACCH is selected */
	if (facch) {
		/* generate prim */
		msg = l1sap_msgb_alloc(200);
		if (!msg)
			return -ENOMEM;
		l1sap = msgb_l1sap_prim(msg);
		osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_RTS,
						PRIM_OP_INDICATION, msg);
		l1sap->u.data.chan_nr = chan_nr;
		l1sap->u.data.link_id = link_id;
		l1sap->u.data.fn = fn;

		rc = l1sap_up(l1t->trx, l1sap);
	}

	/* dont send, if TCH is in signalling only mode */
	if (l1ts->chan_state[chan].rsl_cmode != RSL_CMOD_SPD_SIGN) {
		/* generate prim */
		msg = l1sap_msgb_alloc(200);
		if (!msg)
			return -ENOMEM;
		l1sap = msgb_l1sap_prim(msg);
		osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_TCH_RTS,
						PRIM_OP_INDICATION, msg);
		l1sap->u.tch.chan_nr = chan_nr;
		l1sap->u.tch.fn = fn;

		return l1sap_up(l1t->trx, l1sap);
	}

	return rc;
}

/* RTS for full rate traffic frame */
static int rts_tchf_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan)
{
	/* TCH/F may include FACCH on every 4th burst */
	return rts_tch_common(l1t, tn, fn, chan, 1);
}


/* RTS for half rate traffic frame */
static int rts_tchh_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan)
{
	/* the FN 4/5, 13/14, 21/22 defines that FACCH may be included. */
	return rts_tch_common(l1t, tn, fn, chan, ((fn % 26) >> 2) & 1);
}

/* set multiframe scheduler to given pchan */
int trx_sched_set_pchan(struct l1sched_trx *l1t, uint8_t tn,
	enum gsm_phys_chan_config pchan)
{
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	int i;

	i = find_sched_mframe_idx(pchan, tn);
	if (i < 0) {
		LOGP(DL1C, LOGL_NOTICE, "Failed to configure multiframe "
			"trx=%d ts=%d\n", l1t->trx->nr, tn);
		return -ENOTSUP;
	}
	l1ts->mf_index = i;
	l1ts->mf_period = trx_sched_multiframes[i].period;
	l1ts->mf_frames = trx_sched_multiframes[i].frames;
	LOGP(DL1C, LOGL_NOTICE, "Configuring multiframe with %s trx=%d ts=%d\n",
		trx_sched_multiframes[i].name, l1t->trx->nr, tn);
	return 0;
}

/* setting all logical channels given attributes to active/inactive */
int trx_sched_set_lchan(struct l1sched_trx *l1t, uint8_t chan_nr, uint8_t link_id,
	int active)
{
	uint8_t tn = L1SAP_CHAN2TS(chan_nr);
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	uint8_t ss = l1sap_chan2ss(chan_nr);
	int i;
	int rc = -EINVAL;

	/* look for all matching chan_nr/link_id */
	for (i = 0; i < _TRX_CHAN_MAX; i++) {
		struct l1sched_chan_state *chan_state;
		chan_state = &l1ts->chan_state[i];
		/* skip if pchan type does not match pdch flag */
		if ((trx_sched_multiframes[l1ts->mf_index].pchan
							== GSM_PCHAN_PDCH)
						!= trx_chan_desc[i].pdch)
			continue;
		if (trx_chan_desc[i].chan_nr == (chan_nr & 0xf8)
		 && trx_chan_desc[i].link_id == link_id) {
			rc = 0;
			if (chan_state->active == active)
				continue;
			LOGP(DL1C, LOGL_NOTICE, "%s %s on trx=%d ts=%d\n",
				(active) ? "Activating" : "Deactivating",
				trx_chan_desc[i].name, l1t->trx->nr, tn);
			if (active)
				memset(chan_state, 0, sizeof(*chan_state));
			chan_state->active = active;
			/* free burst memory, to cleanly start with burst 0 */
			if (chan_state->dl_bursts) {
				talloc_free(chan_state->dl_bursts);
				chan_state->dl_bursts = NULL;
			}
			if (chan_state->ul_bursts) {
				talloc_free(chan_state->ul_bursts);
				chan_state->ul_bursts = NULL;
			}
			if (!active)
				chan_state->ho_rach_detect = 0;
		}
	}

	/* disable handover detection (on deactivation) */
	if (!active)
		_sched_act_rach_det(l1t, tn, ss, 0);

	return rc;
}

/* setting all logical channels given attributes to active/inactive */
int trx_sched_set_mode(struct l1sched_trx *l1t, uint8_t chan_nr, uint8_t rsl_cmode,
	uint8_t tch_mode, int codecs, uint8_t codec0, uint8_t codec1,
	uint8_t codec2, uint8_t codec3, uint8_t initial_id, uint8_t handover)
{
	uint8_t tn = L1SAP_CHAN2TS(chan_nr);
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	uint8_t ss = l1sap_chan2ss(chan_nr);
	int i;
	int rc = -EINVAL;
	struct l1sched_chan_state *chan_state;

	/* no mode for PDCH */
	if (trx_sched_multiframes[l1ts->mf_index].pchan == GSM_PCHAN_PDCH)
		return 0;

	/* look for all matching chan_nr/link_id */
	for (i = 0; i < _TRX_CHAN_MAX; i++) {
		if (trx_chan_desc[i].chan_nr == (chan_nr & 0xf8)
		 && trx_chan_desc[i].link_id == 0x00) {
			chan_state = &l1ts->chan_state[i];
			LOGP(DL1C, LOGL_NOTICE, "Set mode %u, %u, handover %u "
				"on %s of trx=%d ts=%d\n", rsl_cmode, tch_mode,
				handover, trx_chan_desc[i].name, l1t->trx->nr,
				tn);
			chan_state->rsl_cmode = rsl_cmode;
			chan_state->tch_mode = tch_mode;
			chan_state->ho_rach_detect = handover;
			if (rsl_cmode == RSL_CMOD_SPD_SPEECH
			 && tch_mode == GSM48_CMODE_SPEECH_AMR) {
				chan_state->codecs = codecs;
				chan_state->codec[0] = codec0;
				chan_state->codec[1] = codec1;
				chan_state->codec[2] = codec2;
				chan_state->codec[3] = codec3;
				chan_state->ul_ft = initial_id;
				chan_state->dl_ft = initial_id;
				chan_state->ul_cmr = initial_id;
				chan_state->dl_cmr = initial_id;
				chan_state->ber_sum = 0;
				chan_state->ber_num = 0;
			}
			rc = 0;
		}
	}

	/* command rach detection
	 * always enable handover, even if state is still set (due to loss
	 * of transceiver link).
	 * disable handover, if state is still set, since we might not know
	 * the actual state of transceiver (due to loss of link) */
	_sched_act_rach_det(l1t, tn, ss, handover);

	return rc;
}

/* setting cipher on logical channels */
int trx_sched_set_cipher(struct l1sched_trx *l1t, uint8_t chan_nr, int downlink,
	int algo, uint8_t *key, int key_len)
{
	uint8_t tn = L1SAP_CHAN2TS(chan_nr);
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	int i;
	int rc = -EINVAL;
	struct l1sched_chan_state *chan_state;

	/* no cipher for PDCH */
	if (trx_sched_multiframes[l1ts->mf_index].pchan == GSM_PCHAN_PDCH)
		return 0;

	/* no algorithm given means a5/0 */
	if (algo <= 0)
		algo = 0;
	else if (key_len != 8) {
		LOGP(DL1C, LOGL_ERROR, "Algo A5/%d not supported with given "
			"key len=%d\n", algo, key_len);
		return -ENOTSUP;
	}

	/* look for all matching chan_nr */
	for (i = 0; i < _TRX_CHAN_MAX; i++) {
		/* skip if pchan type */
		if (trx_chan_desc[i].pdch)
			continue;
		if (trx_chan_desc[i].chan_nr == (chan_nr & 0xf8)) {
			chan_state = &l1ts->chan_state[i];
			LOGP(DL1C, LOGL_NOTICE, "Set a5/%d %s for %s on trx=%d "
				"ts=%d\n", algo,
				(downlink) ? "downlink" : "uplink",
				trx_chan_desc[i].name, l1t->trx->nr, tn);
			if (downlink) {
				chan_state->dl_encr_algo = algo;
				memcpy(chan_state->dl_encr_key, key, key_len);
				chan_state->dl_encr_key_len = key_len;
			} else {
				chan_state->ul_encr_algo = algo;
				memcpy(chan_state->ul_encr_key, key, key_len);
				chan_state->ul_encr_key_len = key_len;
			}
			rc = 0;
		}
	}

	return rc;
}

/* process ready-to-send */
int _sched_rts(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn)
{
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	const struct trx_sched_frame *frame;
	uint8_t offset, period, bid;
	trx_sched_rts_func *func;
	enum trx_chan_type chan;

	/* no multiframe set */
	if (!l1ts->mf_index)
		return 0;

	/* get frame from multiframe */
	period = l1ts->mf_period;
	offset = fn % period;
	frame = l1ts->mf_frames + offset;

	chan = frame->dl_chan;
	bid = frame->dl_bid;
	func = trx_chan_desc[frame->dl_chan].rts_fn;

	/* only on bid == 0 */
	if (bid != 0)
		return 0;

	/* no RTS function */
	if (!func)
		return 0;

	/* check if channel is active */
	if (!trx_chan_desc[chan].auto_active
	 && !l1ts->chan_state[chan].active)
	 	return -EINVAL;

	return func(l1t, tn, fn, frame->dl_chan);
}

/* process downlink burst */
const ubit_t *_sched_dl_burst(struct l1sched_trx *l1t, uint8_t tn,
				uint32_t fn, uint16_t *nbits)
{
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	struct l1sched_chan_state *l1cs;
	const struct trx_sched_frame *frame;
	uint8_t offset, period, bid;
	trx_sched_dl_func *func;
	enum trx_chan_type chan;
	ubit_t *bits = NULL;

	if (!l1ts->mf_index)
		goto no_data;

	/* get frame from multiframe */
	period = l1ts->mf_period;
	offset = fn % period;
	frame = l1ts->mf_frames + offset;

	chan = frame->dl_chan;
	bid = frame->dl_bid;
	func = trx_chan_desc[chan].dl_fn;

	l1cs = &l1ts->chan_state[chan];

	/* check if channel is active */
	if (!trx_chan_desc[chan].auto_active && !l1cs->active) {
		if (nbits)
			*nbits = GSM_BURST_LEN;
		goto no_data;
	}

	/* get burst from function */
	bits = func(l1t, tn, fn, chan, bid, nbits);

	/* encrypt */
	if (bits && l1cs->dl_encr_algo) {
		ubit_t ks[114];
		int i;

		osmo_a5(l1cs->dl_encr_algo, l1cs->dl_encr_key, fn, ks, NULL);
		for (i = 0; i < 57; i++) {
			bits[i + 3] ^= ks[i];
			bits[i + 88] ^= ks[i + 57];
		}
	}

no_data:
	/* in case of C0, we need a dummy burst to maintain RF power */
	if (bits == NULL && l1t->trx == l1t->trx->bts->c0) {
#if 0
		if (chan != TRXC_IDLE) // hack
			LOGP(DL1C, LOGL_DEBUG, "No burst data for %s fn=%u ts=%u "
			     "burst=%d on C0, so filling with dummy burst\n",
			     trx_chan_desc[chan].name, fn, tn, bid);
#endif
		bits = (ubit_t *) dummy_burst;
	}

	return bits;
}

/* process uplink burst */
int trx_sched_ul_burst(struct l1sched_trx *l1t, uint8_t tn, uint32_t current_fn,
	sbit_t *bits, uint16_t nbits, int8_t rssi, int16_t toa256)
{
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	struct l1sched_chan_state *l1cs;
	const struct trx_sched_frame *frame;
	uint8_t offset, period, bid;
	trx_sched_ul_func *func;
	enum trx_chan_type chan;
	uint32_t fn, elapsed;

	if (!l1ts->mf_index)
		return -EINVAL;

	/* calculate how many frames have been elapsed */
	elapsed = (current_fn + GSM_HYPERFRAME - l1ts->mf_last_fn) % GSM_HYPERFRAME;

	/* start counting from last fn + 1, but only if not too many fn have
	 * been elapsed */
	if (elapsed < 10) {
		fn = (l1ts->mf_last_fn + 1) % GSM_HYPERFRAME;
	} else {
		LOGPFN(DL1P, LOGL_NOTICE, current_fn,
		       "Too many contiguous elapsed fn, dropping %u\n", elapsed);
		fn = current_fn;
	}

	while (42) {
		/* get frame from multiframe */
		period = l1ts->mf_period;
		offset = fn % period;
		frame = l1ts->mf_frames + offset;

		chan = frame->ul_chan;
		bid = frame->ul_bid;
		func = trx_chan_desc[chan].ul_fn;

		l1cs = &l1ts->chan_state[chan];

		/* check if channel is active */
		if (!trx_chan_desc[chan].auto_active && !l1cs->active)
			goto next_frame;

		/* omit bursts which have no handler, like IDLE bursts */
		if (!func)
			goto next_frame;

		/* put burst to function */
		if (fn == current_fn) {
			/* decrypt */
			if (bits && l1cs->ul_encr_algo) {
				ubit_t ks[114];
				int i;

				osmo_a5(l1cs->ul_encr_algo,
					l1cs->ul_encr_key,
					fn, NULL, ks);
				for (i = 0; i < 57; i++) {
					if (ks[i])
						bits[i + 3] = - bits[i + 3];
					if (ks[i + 57])
						bits[i + 88] = - bits[i + 88];
				}
			}

			func(l1t, tn, fn, chan, bid, bits, nbits, rssi, toa256);
		} else if (chan != TRXC_RACH && !l1cs->ho_rach_detect) {
			sbit_t spare[GSM_BURST_LEN];
			memset(spare, 0, GSM_BURST_LEN);
			/* We missed a couple of frame numbers (system overload?) and are now
			 * substituting some zero-filled bursts for those bursts we missed */
			LOGPFN(DL1P, LOGL_ERROR, fn, "Substituting all-zero burst (current_fn=%u, "
				"elapsed=%u\n", current_fn, elapsed);
			func(l1t, tn, fn, chan, bid, spare, GSM_BURST_LEN, -128, 0);
		}

next_frame:
		/* reached current fn */
		if (fn == current_fn)
			break;

		fn = (fn + 1) % GSM_HYPERFRAME;
	}

	l1ts->mf_last_fn = fn;

	return 0;
}

struct l1sched_ts *l1sched_trx_get_ts(struct l1sched_trx *l1t, uint8_t tn)
{
	OSMO_ASSERT(tn < ARRAY_SIZE(l1t->ts));
	return &l1t->ts[tn];
}
