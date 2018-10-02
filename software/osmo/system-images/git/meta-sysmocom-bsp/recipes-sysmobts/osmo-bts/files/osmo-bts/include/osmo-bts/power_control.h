#pragma once

#include <stdint.h>
#include <osmo-bts/gsm_data.h>

int lchan_ms_pwr_ctrl(struct gsm_lchan *lchan,
		      const uint8_t ms_power, const int rxLevel);
