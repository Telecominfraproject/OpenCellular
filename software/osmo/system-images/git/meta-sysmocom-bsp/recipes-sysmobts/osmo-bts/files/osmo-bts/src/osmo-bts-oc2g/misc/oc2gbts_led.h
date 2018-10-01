#ifndef _OC2GBTS_LED_H
#define _OC2GBTS_LED_H

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>

#include <osmo-bts/logging.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/utils.h>

#include "oc2gbts_mgr.h"

/* public function prototypes */
void led_set(struct oc2gbts_mgr_instance *mgr, int pattern_id);

void select_led_pattern(struct oc2gbts_mgr_instance *mgr);

#endif
