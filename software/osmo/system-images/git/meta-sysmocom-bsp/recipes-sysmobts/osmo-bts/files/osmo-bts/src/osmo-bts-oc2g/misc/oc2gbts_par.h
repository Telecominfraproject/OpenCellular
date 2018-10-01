#ifndef _OC2GBTS_PAR_H
#define _OC2GBTS_PAR_H

#include <osmocom/core/utils.h>

#define FACTORY_ROM_PATH	"/mnt/rom/factory"
#define USER_ROM_PATH		"/var/run/oc2gbts-mgr"
#define UPTIME_TMP_PATH		"/tmp/uptime"

enum oc2gbts_par {
	OC2GBTS_PAR_TEMP_SUPPLY_MAX,
	OC2GBTS_PAR_TEMP_SOC_MAX,
	OC2GBTS_PAR_TEMP_FPGA_MAX,
	OC2GBTS_PAR_TEMP_RMSDET_MAX,
	OC2GBTS_PAR_TEMP_OCXO_MAX,
	OC2GBTS_PAR_TEMP_TX_MAX,
	OC2GBTS_PAR_TEMP_PA_MAX,
	OC2GBTS_PAR_VOLT_SUPPLY_MAX,
	OC2GBTS_PAR_PWR_SUPPLY_MAX,
	OC2GBTS_PAR_PWR_PA_MAX,
	OC2GBTS_PAR_VSWR_MAX,
	OC2GBTS_PAR_GPS_FIX,
	OC2GBTS_PAR_SERNR,
	OC2GBTS_PAR_HOURS,
	OC2GBTS_PAR_BOOTS,
	OC2GBTS_PAR_KEY,
	_NUM_OC2GBTS_PAR
};

extern const struct value_string oc2gbts_par_names[_NUM_OC2GBTS_PAR+1];

int oc2gbts_par_get_int(enum oc2gbts_par par, int *ret);
int oc2gbts_par_set_int(enum oc2gbts_par par, int val);
int oc2gbts_par_get_buf(enum oc2gbts_par par, uint8_t *buf,
			 unsigned int size);
int oc2gbts_par_set_buf(enum oc2gbts_par par, const uint8_t *buf,
			 unsigned int size);

int oc2gbts_par_is_int(enum oc2gbts_par par);
int oc2gbts_par_get_gps_fix(void *ctx, time_t *ret);
int oc2gbts_par_set_gps_fix(void *ctx, time_t val);

#endif
