#ifndef _SYSMOBTS_HW_MISC_H
#define _SYSMOBTS_HW_MISC_H

enum sysmobts_led {
	LED_NONE,
	LED_RF_ACTIVE,
	LED_ONLINE,
};

int sysmobts_led_set(enum sysmobts_led nr, int on);

#endif
