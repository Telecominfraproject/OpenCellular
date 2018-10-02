#ifndef _HW_MISC_H
#define _HW_MISC_H

enum lc15bts_led_color {
	LED_OFF,
	LED_RED,
	LED_GREEN,
	LED_ORANGE,
};

int lc15bts_led_set(enum lc15bts_led_color c);

#endif
