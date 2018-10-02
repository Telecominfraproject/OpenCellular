#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include "lc15bts.h"

struct gsm_bts_trx;

int band_lc152osmo(GsmL1_FreqBand_t band);

int lc15bts_select_lc15_band(struct gsm_bts_trx *trx, uint16_t arfcn);

#endif
