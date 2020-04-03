/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef DEVICE_UTILS_H_
#define DEVICE_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include "utils/types.h"

#define LTE_PMAX_MIN			(-30)
#define LTE_PMAX_MAX			(33)

extern bool get_lte_band_channel(uint16_t id, lte_direction_e dir, uint32_t *start_hz, uint32_t *end_hz, int32_t *spacing_hz);

extern uint32_t dl_hz_to_earfcn(uint16_t band_id, uint32_t fdl);
extern uint32_t ul_hz_to_earfcn(uint16_t band_id, uint32_t ful);

extern uint32_t dl_earfcn_to_hz(uint16_t band_id, uint32_t Ndl);
extern uint32_t ul_earfcn_to_hz(uint16_t band_id, uint32_t Nul);

extern bool validate_lte_cfg(uint16_t band_id, int bandwidth, int ref_sig_power, int earfcndl, int earfcnul, int pmax);


#endif // DEVICE_UTILS_H_

