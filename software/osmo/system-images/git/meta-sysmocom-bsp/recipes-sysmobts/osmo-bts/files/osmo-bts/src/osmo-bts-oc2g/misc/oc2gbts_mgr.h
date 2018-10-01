#ifndef _OC2GBTS_MGR_H
#define _OC2GBTS_MGR_H

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>

#include <osmocom/core/select.h>
#include <osmocom/core/timer.h>

#include <stdint.h>
#include <gps.h>

#define OC2GBTS_SENSOR_TIMER_DURATION			60
#define OC2GBTS_PREVENT_TIMER_DURATION			15 * 60
#define OC2GBTS_PREVENT_TIMER_SHORT_DURATION		5 * 60
#define OC2GBTS_PREVENT_TIMER_NONE			0
#define OC2GBTS_PREVENT_RETRY				INT_MAX - 1

enum BLINK_PATTERN {
	BLINK_PATTERN_POWER_ON = 0,	//hardware set
	BLINK_PATTERN_INIT,
	BLINK_PATTERN_NORMAL,
	BLINK_PATTERN_EXT_LINK_MALFUNC,
	BLINK_PATTERN_INT_PROC_MALFUNC,
	BLINK_PATTERN_SUPPLY_VOLT_LOW,
	BLINK_PATTERN_SUPPLY_VOLT_MIN,
	BLINK_PATTERN_VSWR_HIGH,
	BLINK_PATTERN_VSWR_MAX,
	BLINK_PATTERN_TEMP_HIGH,
	BLINK_PATTERN_TEMP_MAX,
	BLINK_PATTERN_SUPPLY_PWR_HIGH,
	BLINK_PATTERN_SUPPLY_PWR_MAX,
	BLINK_PATTERN_PA_PWR_HIGH,
	BLINK_PATTERN_PA_PWR_MAX,
	BLINK_PATTERN_GPS_FIX_LOST,
	BLINK_PATTERN_MAX_ITEM
};

#define BLINK_PATTERN_COMMAND {\
	"set red; sleep 5.0",\
	"set orange; sleep 5.0",\
	"set green; sleep 2.5; set off; sleep 2.5",\
	"set red; sleep 0.5; set off; sleep 0.5",\
	"set red; sleep 2.5; set off; sleep 2.5",\
	"set green; sleep 2.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5; set green; sleep 0.5; set off; sleep 0.5",\
	"set red; sleep 2.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5; set green; sleep 0.5; set off; sleep 0.5 ",\
	"set green; sleep 2.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5",\
	"set red; sleep 2.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5",\
	"set orange; sleep 2.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5 ",\
	"set red; sleep 2.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5",\
	"set green; sleep 2.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5",\
	"set red; sleep 2.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5",\
	"set green; sleep 2.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5",\
	"set red; sleep 2.5; set off; sleep 0.5; set red; sleep 0.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5",\
	"set green; sleep 2.5; set off; sleep 0.5; set green; sleep 0.5; set off; sleep 0.5; set orange; sleep 0.5; set off; sleep 0.5",\
}

enum {
	DTEMP,
	DFW,
	DFIND,
	DCALIB,
	DSWD,
};

// TODO NTQD: Define new actions like reducing output power, limit ARM core speed, shutdown second TRX/PA, ... 
enum {
#if 0
	SENSOR_ACT_PWR_CONTRL	=	0x1,
#endif
	SENSOR_ACT_PA_OFF	=	0x2,
	SENSOR_ACT_BTS_SRV_OFF	=	0x10,
};

/* actions only for normal state */
enum {
#if 0
	SENSOR_ACT_NORM_PW_CONTRL	=	0x1,
#endif
	SENSOR_ACT_NORM_PA_ON	=	0x2,
	SENSOR_ACT_NORM_BTS_SRV_ON=	0x10,
};

enum oc2gbts_sensor_state {
	STATE_NORMAL,		/* Everything is fine */
	STATE_WARNING_HYST,	/* Go back to normal next? */
	STATE_WARNING,		/* We are above the warning threshold */
	STATE_CRITICAL,		/* We have an issue. Wait for below warning */
};

enum oc2gbts_leds_name {
	OC2GBTS_LED_RED = 0,
	OC2GBTS_LED_GREEN,
	OC2GBTS_LED_ORANGE,
	OC2GBTS_LED_OFF,
	_OC2GBTS_LED_MAX
};

struct oc2gbts_led{
	char *name;
	char *fullname;
	char *path;
};

/**
 * Temperature Limits. We separate from a threshold
 * that will generate a warning and one that is so
 * severe that an action will be taken.
 */
struct oc2gbts_temp_limit {
	int thresh_warn_max;
	int thresh_crit_max;
	int thresh_warn_min;
};

struct oc2gbts_volt_limit {
	int thresh_warn_max;
	int thresh_crit_max;
	int thresh_warn_min;
	int thresh_crit_min;
};

struct oc2gbts_pwr_limit {
	int thresh_warn_max;
	int thresh_crit_max;
};

struct oc2gbts_vswr_limit {
	int thresh_warn_max;
	int thresh_crit_max;
};

struct oc2gbts_gps_fix_limit {
	int thresh_warn_max;
};

struct oc2gbts_sleep_time {
	int sleep_sec;
	int sleep_usec;
};

struct oc2gbts_led_timer {
	uint8_t idx;
	struct osmo_timer_list timer;
	struct oc2gbts_sleep_time param;
};

struct oc2gbts_led_timer_list {
	struct llist_head list;
	struct oc2gbts_led_timer led_timer;
};

struct oc2gbts_preventive_list {
	struct llist_head list;
	struct oc2gbts_sleep_time param;
	int action_flag;
};

enum mgr_vty_node {
	MGR_NODE = _LAST_OSMOVTY_NODE + 1,

	ACT_NORM_NODE,
	ACT_WARN_NODE,
	ACT_CRIT_NODE,
	LIMIT_SUPPLY_TEMP_NODE,
	LIMIT_SOC_NODE,
	LIMIT_FPGA_NODE,
	LIMIT_RMSDET_NODE,
	LIMIT_OCXO_NODE,
	LIMIT_TX_TEMP_NODE,
	LIMIT_PA_TEMP_NODE,
	LIMIT_SUPPLY_VOLT_NODE,
	LIMIT_VSWR_NODE,
	LIMIT_SUPPLY_PWR_NODE,
	LIMIT_PA_PWR_NODE,
	LIMIT_GPS_FIX_NODE,
};

enum mgr_vty_limit_type {
	MGR_LIMIT_TYPE_TEMP = 0,
	MGR_LIMIT_TYPE_VOLT,
	MGR_LIMIT_TYPE_VSWR,
	MGR_LIMIT_TYPE_PWR,
	_MGR_LIMIT_TYPE_MAX,
};

struct oc2gbts_mgr_instance {
	const char *config_file;

	struct {
		struct oc2gbts_temp_limit supply_temp_limit;
		struct oc2gbts_temp_limit soc_temp_limit;
		struct oc2gbts_temp_limit fpga_temp_limit;
		struct oc2gbts_temp_limit rmsdet_temp_limit;
		struct oc2gbts_temp_limit ocxo_temp_limit;
		struct oc2gbts_temp_limit tx_temp_limit;
		struct oc2gbts_temp_limit pa_temp_limit;
	} temp;

	struct {
		struct oc2gbts_volt_limit supply_volt_limit;
	} volt;

	struct {
		struct oc2gbts_pwr_limit supply_pwr_limit;
		struct oc2gbts_pwr_limit pa_pwr_limit;
	} pwr;

	struct {
		struct oc2gbts_vswr_limit vswr_limit;
		int last_vswr;
	} vswr;

	struct {
		struct oc2gbts_gps_fix_limit gps_fix_limit;
		int last_update;
		time_t last_gps_fix;
		time_t gps_fix_now;
		int gps_open;
		struct osmo_fd gpsfd;
		struct gps_data_t gpsdata;
		struct osmo_timer_list fix_timeout;
	} gps;

	struct {
		int action_norm;
		int action_warn;
		int action_crit;
		int action_comb;

		enum oc2gbts_sensor_state state;
	} state;

	struct {
		int state;
		int calib_from_loop;
		struct osmo_timer_list calib_timeout;
	} calib;

	struct {
		int state;
		int swd_from_loop;
		unsigned long long int swd_events;
		unsigned long long int swd_events_cache;
		unsigned long long int swd_eventmasks;
		int num_events;
		struct osmo_timer_list swd_timeout;
	} swd;

	struct {
		uint8_t	led_idx;
		uint8_t last_pattern_id;
		uint8_t active_timer;
		struct llist_head list;
	} oc2gbts_leds;

	struct {
		int is_up;
		uint32_t last_seqno;
		struct osmo_timer_list recon_timer;
		struct ipa_client_conn *bts_conn;
		uint32_t crit_flags;
		uint32_t warn_flags;
	} oc2gbts_ctrl;

	struct oc2gbts_alarms {
		int temp_high;
		int temp_max;
		int supply_low;
		int supply_min;
		int vswr_high;
		int vswr_max;
		int supply_pwr_high;
		int supply_pwr_max;
		int pa_pwr_high;
		int pa_pwr_max;
		int gps_fix_lost;
		struct llist_head list;
		struct osmo_timer_list preventive_timer;
		int preventive_duration;
		int preventive_retry;
	} alarms;

};

enum oc2gbts_mgr_fail_evt_rep_crit_sig {
	/* Critical alarms */
	S_MGR_TEMP_SUPPLY_CRIT_MAX_ALARM			= (1 << 0),
	S_MGR_TEMP_SOC_CRIT_MAX_ALARM				= (1 << 1),
	S_MGR_TEMP_FPGA_CRIT_MAX_ALARM				= (1 << 2),
	S_MGR_TEMP_RMS_DET_CRIT_MAX_ALARM			= (1 << 3),
	S_MGR_TEMP_OCXO_CRIT_MAX_ALARM				= (1 << 4),
	S_MGR_TEMP_TRX_CRIT_MAX_ALARM				= (1 << 5),
	S_MGR_TEMP_PA_CRIT_MAX_ALARM				= (1 << 6),
	S_MGR_SUPPLY_CRIT_MAX_ALARM					= (1 << 7),
	S_MGR_SUPPLY_CRIT_MIN_ALARM					= (1 << 8),
	S_MGR_VSWR_CRIT_MAX_ALARM					= (1 << 9),
	S_MGR_PWR_SUPPLY_CRIT_MAX_ALARM				= (1 << 10),
	S_MGR_PWR_PA_CRIT_MAX_ALARM					= (1 << 11),
	_S_MGR_CRIT_ALARM_MAX,
};

enum oc2gbts_mgr_fail_evt_rep_warn_sig {
	/* Warning alarms */
	S_MGR_TEMP_SUPPLY_WARN_MIN_ALARM			= (1 << 0),
	S_MGR_TEMP_SUPPLY_WARN_MAX_ALARM			= (1 << 1),
	S_MGR_TEMP_SOC_WARN_MIN_ALARM				= (1 << 2),
	S_MGR_TEMP_SOC_WARN_MAX_ALARM				= (1 << 3),
	S_MGR_TEMP_FPGA_WARN_MIN_ALARM				= (1 << 4),
	S_MGR_TEMP_FPGA_WARN_MAX_ALARM				= (1 << 5),
	S_MGR_TEMP_RMS_DET_WARN_MIN_ALARM			= (1 << 6),
	S_MGR_TEMP_RMS_DET_WARN_MAX_ALARM			= (1 << 7),
	S_MGR_TEMP_OCXO_WARN_MIN_ALARM				= (1 << 8),
	S_MGR_TEMP_OCXO_WARN_MAX_ALARM				= (1 << 9),
	S_MGR_TEMP_TRX_WARN_MIN_ALARM				= (1 << 10),
	S_MGR_TEMP_TRX_WARN_MAX_ALARM				= (1 << 11),
	S_MGR_TEMP_PA_WARN_MIN_ALARM				= (1 << 12),
	S_MGR_TEMP_PA_WARN_MAX_ALARM				= (1 << 13),
	S_MGR_SUPPLY_WARN_MIN_ALARM					= (1 << 14),
	S_MGR_SUPPLY_WARN_MAX_ALARM					= (1 << 15),
	S_MGR_VSWR_WARN_MAX_ALARM					= (1 << 16),
	S_MGR_PWR_SUPPLY_WARN_MAX_ALARM				= (1 << 17),
	S_MGR_PWR_PA_WARN_MAX_ALARM					= (1 << 18),
	S_MGR_GPS_FIX_WARN_ALARM					= (1 << 19),
	_S_MGR_WARN_ALARM_MAX,
};

enum oc2gbts_mgr_failure_event_causes {
	/* Critical causes */
	NM_EVT_CAUSE_CRIT_TEMP_SUPPLY_MAX_FAIL	= 0x4100,
	NM_EVT_CAUSE_CRIT_TEMP_FPGA_MAX_FAIL	= 0x4101,
	NM_EVT_CAUSE_CRIT_TEMP_SOC_MAX_FAIL		= 0x4102,
	NM_EVT_CAUSE_CRIT_TEMP_RMS_DET_MAX_FAIL	= 0x4103,
	NM_EVT_CAUSE_CRIT_TEMP_OCXO_MAX_FAIL	= 0x4104,
	NM_EVT_CAUSE_CRIT_TEMP_TRX_MAX_FAIL		= 0x4105,
	NM_EVT_CAUSE_CRIT_TEMP_PA_MAX_FAIL		= 0x4106,
	NM_EVT_CAUSE_CRIT_SUPPLY_MAX_FAIL		= 0x4107,
	NM_EVT_CAUSE_CRIT_SUPPLY_MIN_FAIL		= 0x4108,
	NM_EVT_CAUSE_CRIT_VSWR_MAX_FAIL			= 0x4109,
	NM_EVT_CAUSE_CRIT_PWR_SUPPLY_MAX_FAIL	= 0x410A,
	NM_EVT_CAUSE_CRIT_PWR_PA_MAX_FAIL		= 0x410B,
	/* Warning causes */
	NM_EVT_CAUSE_WARN_TEMP_SUPPLY_LOW_FAIL	= 0x4400,
	NM_EVT_CAUSE_WARN_TEMP_SUPPLY_HIGH_FAIL	= 0x4401,
	NM_EVT_CAUSE_WARN_TEMP_FPGA_LOW_FAIL	= 0x4402,
	NM_EVT_CAUSE_WARN_TEMP_FPGA_HIGH_FAIL	= 0x4403,
	NM_EVT_CAUSE_WARN_TEMP_SOC_LOW_FAIL		= 0x4404,
	NM_EVT_CAUSE_WARN_TEMP_SOC_HIGH_FAIL	= 0x4405,
	NM_EVT_CAUSE_WARN_TEMP_RMS_DET_LOW_FAIL	= 0x4406,
	NM_EVT_CAUSE_WARN_TEMP_RMS_DET_HIGH_FAIL= 0x4407,
	NM_EVT_CAUSE_WARN_TEMP_OCXO_LOW_FAIL	= 0x4408,
	NM_EVT_CAUSE_WARN_TEMP_OCXO_HIGH_FAIL	= 0x4409,
	NM_EVT_CAUSE_WARN_TEMP_TRX_LOW_FAIL		= 0x440A,
	NM_EVT_CAUSE_WARN_TEMP_TRX_HIGH_FAIL	= 0x440B,
	NM_EVT_CAUSE_WARN_TEMP_PA_LOW_FAIL		= 0x440C,
	NM_EVT_CAUSE_WARN_TEMP_PA_HIGH_FAIL		= 0x440D,
	NM_EVT_CAUSE_WARN_SUPPLY_LOW_FAIL		= 0x440E,
	NM_EVT_CAUSE_WARN_SUPPLY_HIGH_FAIL		= 0x440F,
	NM_EVT_CAUSE_WARN_VSWR_HIGH_FAIL		= 0x4410,
	NM_EVT_CAUSE_WARN_PWR_SUPPLY_HIGH_FAIL	= 0x4411,
	NM_EVT_CAUSE_WARN_PWR_PA_HIGH_FAIL		= 0x4412,
	NM_EVT_CAUSE_WARN_GPS_FIX_FAIL			= 0x4413,
};

/* This defines the list of notification events for systemd service watchdog.
   all these events must be notified in a certain service defined timeslot
   or the service (this app) would be restarted (only if related systemd service
   unit file has WatchdogSec!=0).
   WARNING: swd events must begin with event 0. Last events must be
   SWD_LAST (max 64 events in this list).
*/
enum mgr_swd_events {
	SWD_MAINLOOP = 0,
	SWD_CHECK_SENSOR,
	SWD_UPDATE_HOURS,
	SWD_CHECK_TEMP_SENSOR,
	SWD_CHECK_LED_CTRL,
	SWD_CHECK_CALIB,
	SWD_CHECK_BTS_CONNECTION,
	SWD_LAST
};

int oc2gbts_mgr_vty_init(void);
int oc2gbts_mgr_parse_config(struct oc2gbts_mgr_instance *mgr);
int oc2gbts_mgr_nl_init(void);
int oc2gbts_mgr_sensor_init(struct oc2gbts_mgr_instance *mgr);
const char *oc2gbts_mgr_sensor_get_state(enum oc2gbts_sensor_state state);

int oc2gbts_mgr_calib_init(struct oc2gbts_mgr_instance *mgr);
int oc2gbts_mgr_control_init(struct oc2gbts_mgr_instance *mgr);
int oc2gbts_mgr_calib_run(struct oc2gbts_mgr_instance *mgr);
void oc2gbts_mgr_dispatch_alarm(struct oc2gbts_mgr_instance *mgr, const int cause, const char *key, const char *text);
void handle_alert_actions(struct oc2gbts_mgr_instance *mgr);
void handle_ceased_actions(struct oc2gbts_mgr_instance *mgr);
void handle_warn_actions(struct oc2gbts_mgr_instance *mgr);
extern void *tall_mgr_ctx;

#endif
