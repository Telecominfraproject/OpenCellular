#ifndef _OC2GBTS_CLOCK_H
#define _OC2GBTS_CLOCK_H

int oc2gbts_clock_err_open(void);
void oc2gbts_clock_err_close(void);
int oc2gbts_clock_err_reset(void);
int oc2gbts_clock_err_get(int *fault, int *error_ppt, 
		int *accuracy_ppq, int *interval_sec);

int oc2gbts_clock_dac_open(void);
void oc2gbts_clock_dac_close(void);
int oc2gbts_clock_dac_get(int *dac_value);
int oc2gbts_clock_dac_set(int dac_value);
int oc2gbts_clock_dac_save(void);

#endif
