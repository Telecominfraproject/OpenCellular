#ifndef OSMO_BTS_MEAS_H
#define OSMO_BTS_MEAS_H

#define MEAS_MAX_TIMING_ADVANCE 63
#define MEAS_MIN_TIMING_ADVANCE 0

int lchan_new_ul_meas(struct gsm_lchan *lchan, struct bts_ul_meas *ulm, uint32_t fn);

int lchan_meas_check_compute(struct gsm_lchan *lchan, uint32_t fn);

#endif
