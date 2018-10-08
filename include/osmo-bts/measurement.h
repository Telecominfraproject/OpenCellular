#ifndef OSMO_BTS_MEAS_H
#define OSMO_BTS_MEAS_H

#define MEAS_MAX_TIMING_ADVANCE 63
#define MEAS_MIN_TIMING_ADVANCE 0

int lchan_new_ul_meas(struct gsm_lchan *lchan, struct bts_ul_meas *ulm, uint32_t fn);

int lchan_meas_check_compute(struct gsm_lchan *lchan, uint32_t fn);

int lchan_meas_process_measurement(struct gsm_lchan *lchan, struct bts_ul_meas *ulm, uint32_t fn);

void lchan_meas_reset(struct gsm_lchan *lchan);

bool ts45008_83_is_sub(struct gsm_lchan *lchan, uint32_t fn, bool is_amr_sid_update);

int is_meas_complete(struct gsm_lchan *lchan, uint32_t fn);

#endif
