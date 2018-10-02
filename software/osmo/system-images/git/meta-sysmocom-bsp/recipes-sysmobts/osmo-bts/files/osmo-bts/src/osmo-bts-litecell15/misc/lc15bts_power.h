#ifndef _LC15BTS_POWER_H
#define _LC15BTS_POWER_H

enum lc15bts_power_source {
	LC15BTS_POWER_SUPPLY,
	LC15BTS_POWER_PA0,
	LC15BTS_POWER_PA1,
	_NUM_POWER_SOURCES
};

enum lc15bts_power_type {
	LC15BTS_POWER_POWER,
	LC15BTS_POWER_VOLTAGE,
	LC15BTS_POWER_CURRENT,
	_NUM_POWER_TYPES
};

int lc15bts_power_sensor_get(
	enum lc15bts_power_source source,
	enum lc15bts_power_type type,
	int *volt);

int lc15bts_power_set(
	enum lc15bts_power_source source,
	int en);

int lc15bts_power_get(
        enum lc15bts_power_source source);

enum lc15bts_vswr_sensor {
	LC15BTS_VSWR_TX0,
	LC15BTS_VSWR_TX1,
	_NUM_VSWR_SENSORS
};

int lc15bts_vswr_get(enum lc15bts_vswr_sensor sensor, int *vswr);

#endif
