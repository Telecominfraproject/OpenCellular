#ifndef _LC15BTS_CLOCK_H
#define _LC15BTS_CLOCK_H

int lc15bts_clock_err_open(void);
void lc15bts_clock_err_close(void);
int lc15bts_clock_err_reset(void);
int lc15bts_clock_err_get(int *fault, int *error_ppt, 
		int *accuracy_ppq, int *interval_sec);

int lc15bts_clock_dac_open(void);
void lc15bts_clock_dac_close(void);
int lc15bts_clock_dac_get(int *dac_value);
int lc15bts_clock_dac_set(int dac_value);
int lc15bts_clock_dac_save(void);

#endif
