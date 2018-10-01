#ifndef _OC2GBTS_BTS_H_
#define _OC2GBTS_BTS_H_

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <osmo-bts/logging.h>

/* public function prototypes */
void check_bts_led_pattern(uint8_t *led);
int check_sensor_led_pattern( struct oc2gbts_mgr_instance *mgr, uint8_t *led);

#endif
