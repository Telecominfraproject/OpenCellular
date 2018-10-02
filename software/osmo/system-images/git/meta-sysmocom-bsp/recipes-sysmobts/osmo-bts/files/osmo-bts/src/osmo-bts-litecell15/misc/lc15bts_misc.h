#ifndef _LC15BTS_MISC_H
#define _LC15BTS_MISC_H

#include <stdint.h>

void lc15bts_check_temp(int no_rom_write);
void lc15bts_check_power(int no_rom_write);
void lc15bts_check_vswr(int no_rom_write);

int lc15bts_update_hours(int no_rom_write);

enum lc15bts_firmware_type {
	LC15BTS_FW_DSP0,
	LC15BTS_FW_DSP1,
	_NUM_FW
};

#endif
