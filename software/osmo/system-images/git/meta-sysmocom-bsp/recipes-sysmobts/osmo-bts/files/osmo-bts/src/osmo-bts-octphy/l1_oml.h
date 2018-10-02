#pragma once

#include "l1_if.h"

/* channel control */
int l1if_rsl_chan_act(struct gsm_lchan *lchan);
int l1if_rsl_chan_rel(struct gsm_lchan *lchan);
int l1if_rsl_deact_sacch(struct gsm_lchan *lchan);
int l1if_rsl_mode_modify(struct gsm_lchan *lchan);

int l1if_set_ciphering(struct gsm_lchan *lchan, int dir_downlink);

uint32_t trx_get_hlayer1(struct gsm_bts_trx *trx);

int gsm_abis_mo_check_attr(const struct gsm_abis_mo *mo,
			   const uint8_t * attr_ids, unsigned int num_attr_ids);

int lchan_activate(struct gsm_lchan *lchan);
