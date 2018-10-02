#ifndef _LC15BTS_LED_H
#define _LC15BTS_LED_H

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>

#include <osmo-bts/logging.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/utils.h>

#include "lc15bts_mgr.h"

/* public function prototypes */
void led_set(struct lc15bts_mgr_instance *mgr, int pattern_id);

void select_led_pattern(struct lc15bts_mgr_instance *mgr);

#endif
