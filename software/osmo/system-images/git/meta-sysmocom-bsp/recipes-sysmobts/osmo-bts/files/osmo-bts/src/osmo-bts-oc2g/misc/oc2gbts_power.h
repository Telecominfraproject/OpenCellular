#ifndef _OC2GBTS_POWER_H
#define _OC2GBTS_POWER_H

enum oc2gbts_power_source {
	OC2GBTS_POWER_SUPPLY,
	OC2GBTS_POWER_PA,
	_NUM_POWER_SOURCES
};

enum oc2gbts_power_type {
	OC2GBTS_POWER_POWER,
	OC2GBTS_POWER_VOLTAGE,
	OC2GBTS_POWER_CURRENT,
	_NUM_POWER_TYPES
};

int oc2gbts_power_sensor_get(
	enum oc2gbts_power_source source,
	enum oc2gbts_power_type type,
	int *volt);

int oc2gbts_power_set(
	enum oc2gbts_power_source source,
	int en);

int oc2gbts_power_get(
		enum oc2gbts_power_source source);

enum oc2gbts_vswr_sensor {
	OC2GBTS_VSWR,
	_NUM_VSWR_SENSORS
};

int oc2gbts_vswr_get(enum oc2gbts_vswr_sensor sensor, int *vswr);

#endif
