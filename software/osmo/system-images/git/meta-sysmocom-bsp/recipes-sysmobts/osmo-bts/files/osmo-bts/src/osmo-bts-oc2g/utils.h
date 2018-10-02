#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include "oc2gbts.h"

struct gsm_bts_trx;

int band_oc2g2osmo(GsmL1_FreqBand_t band);

int oc2gbts_select_oc2g_band(struct gsm_bts_trx *trx, uint16_t arfcn);

#endif
