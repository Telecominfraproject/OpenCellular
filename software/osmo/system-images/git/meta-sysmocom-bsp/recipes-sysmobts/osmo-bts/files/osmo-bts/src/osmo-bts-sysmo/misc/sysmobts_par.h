#ifndef _SYSMOBTS_PAR_H
#define _SYSMOBTS_PAR_H

#include <osmocom/core/utils.h>

struct sysmobts_net_cfg;

enum sysmobts_par {
	SYSMOBTS_PAR_MAC,
	SYSMOBTS_PAR_CLK_FACTORY,
	SYSMOBTS_PAR_TEMP_DIG_MAX,
	SYSMOBTS_PAR_TEMP_RF_MAX,
	SYSMOBTS_PAR_SERNR,
	SYSMOBTS_PAR_HOURS,
	SYSMOBTS_PAR_BOOTS,
	SYSMOBTS_PAR_KEY,
	SYSMOBTS_PAR_MODEL_NR,
	SYSMOBTS_PAR_MODEL_FLAGS,
	SYSMOBTS_PAR_TRX_NR,
	_NUM_SYSMOBTS_PAR
};

extern const struct value_string sysmobts_par_names[_NUM_SYSMOBTS_PAR+1];

int sysmobts_par_get_int(enum sysmobts_par par, int *ret);
int sysmobts_par_set_int(enum sysmobts_par par, int val);
int sysmobts_par_get_buf(enum sysmobts_par par, uint8_t *buf,
			 unsigned int size);
int sysmobts_par_set_buf(enum sysmobts_par par, const uint8_t *buf,
			 unsigned int size);
int sysmobts_par_get_net(struct sysmobts_net_cfg *cfg);
int sysmobts_par_set_net(struct sysmobts_net_cfg *cfg);
int sysmobts_get_type(int *bts_type);
int sysmobts_get_trx(int *trx_number);
char *sysmobts_model(int bts_type, int trx_num);
int sysmobts_par_is_int(enum sysmobts_par par);

#endif
