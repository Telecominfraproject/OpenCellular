#ifndef _SYSMOBTS_MISC_H
#define _SYSMOBTS_MISC_H

#include <stdint.h>

enum sysmobts_temp_sensor {
	SYSMOBTS_TEMP_DIGITAL = 1,
	SYSMOBTS_TEMP_RF = 2,
};

enum sysmobts_temp_type {
	SYSMOBTS_TEMP_INPUT,
	SYSMOBTS_TEMP_LOWEST,
	SYSMOBTS_TEMP_HIGHEST,
	_NUM_TEMP_TYPES
};

int sysmobts_temp_get(enum sysmobts_temp_sensor sensor,
		      enum sysmobts_temp_type type);

void sysmobts_check_temp(int no_eeprom_write);

int sysmobts_update_hours(int no_epprom_write);

enum sysmobts_firmware_type {
	SYSMOBTS_FW_FPGA,
	SYSMOBTS_FW_DSP,
	_NUM_FW
};

int sysmobts_firmware_reload(enum sysmobts_firmware_type type);


int sysmobts_bts_type();
int sysmobts_trx_number();
int is_sbts2050(void);
int is_sbts2050_trx(int);
int is_sbts2050_master(void);

struct sbts2050_power_status {
	float main_supply_current;

	int master_enabled;
	float master_voltage;
	float master_current;

	int slave_enabled;
	float slave_voltage;
	float slave_current;

	int pa_enabled;
	float pa_voltage;
	float pa_current;

	float pa_bias_voltage;
};

int sbts2050_uc_check_temp(int *temp_pa, int *temp_board);
int sbts2050_uc_set_power(int pmaster, int pslave, int ppa);
int sbts2050_uc_get_status(struct sbts2050_power_status *status);
int sbts2050_uc_set_pa_power(int on_off);
int sbts2050_uc_set_slave_power(int on_off);
void sbts2050_uc_initialize();

#endif
