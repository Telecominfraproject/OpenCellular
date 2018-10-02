#ifndef SYSMOBTS_UTILS_H
#define SYSMOBTS_UTILS_H

#include <stdint.h>
#include "femtobts.h"

struct gsm_bts_trx;

int band_femto2osmo(GsmL1_FreqBand_t band);

int sysmobts_select_femto_band(struct gsm_bts_trx *trx, uint16_t arfcn);
#endif
