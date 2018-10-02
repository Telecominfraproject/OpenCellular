#ifndef _TRX_LOOPS_H
#define _TRX_LOOPS_H

/*
 * calibration of loops
 */

/* how much power levels do we raise/lower as maximum (1 level = 2 dB) */
#define MS_RAISE_MAX 4
#define MS_LOWER_MAX 2

/*
 * loops api
 */

int trx_loop_sacch_input(struct l1sched_trx *l1t, uint8_t chan_nr,
	struct l1sched_chan_state *chan_state, int8_t rssi, int16_t toa);

int trx_loop_sacch_clock(struct l1sched_trx *l1t, uint8_t chan_nr,
        struct l1sched_chan_state *chan_state);

int trx_loop_amr_input(struct l1sched_trx *l1t, uint8_t chan_nr,
        struct l1sched_chan_state *chan_state, float ber);

int trx_loop_amr_set(struct l1sched_chan_state *chan_state, int loop);

#endif /* _TRX_LOOPS_H */
