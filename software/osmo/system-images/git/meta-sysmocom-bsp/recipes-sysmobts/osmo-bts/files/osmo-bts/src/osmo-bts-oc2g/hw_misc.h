#ifndef _HW_MISC_H
#define _HW_MISC_H

enum oc2gbts_led_color {
	LED_OFF,
	LED_RED,
	LED_GREEN,
	LED_ORANGE,
};

int oc2gbts_led_set(enum oc2gbts_led_color c);

#endif
