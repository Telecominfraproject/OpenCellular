#ifndef _OC2GBTS_MISC_H
#define _OC2GBTS_MISC_H

#include <stdint.h>

void oc2gbts_check_temp(int no_rom_write);
void oc2gbts_check_power(int no_rom_write);
void oc2gbts_check_vswr(int no_rom_write);

int oc2gbts_update_hours(int no_rom_write);

enum oc2gbts_firmware_type {
	OC2GBTS_FW_DSP,
	_NUM_FW
};

#endif
