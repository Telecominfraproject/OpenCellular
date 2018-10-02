#pragma once

enum {
	HANDOVER_NONE = 0,
	HANDOVER_ENABLED,
	HANDOVER_WAIT_FRAME,
};

void handover_rach(struct gsm_lchan *lchan, uint8_t ra, uint8_t acc_delay);
void handover_frame(struct gsm_lchan *lchan);
void handover_reset(struct gsm_lchan *lchan);

