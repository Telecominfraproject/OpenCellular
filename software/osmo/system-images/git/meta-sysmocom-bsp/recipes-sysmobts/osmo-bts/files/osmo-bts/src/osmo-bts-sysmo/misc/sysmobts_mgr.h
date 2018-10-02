#ifndef _SYSMOBTS_MGR_H
#define _SYSMOBTS_MGR_H

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/ctrl/control_if.h>
#include <osmocom/core/select.h>
#include <osmocom/core/timer.h>

#include <gps.h>

#include <stdint.h>

enum {
	DTEMP,
	DFW,
	DFIND,
	DCALIB,
};


enum {
#if 0
	TEMP_ACT_PWR_CONTRL	=	0x1,
#endif
	TEMP_ACT_SLAVE_OFF	=	0x4,
	TEMP_ACT_PA_OFF		=	0x8,
	TEMP_ACT_BTS_SRV_OFF	=	0x10,
};

/* actions only for normal state */
enum {
#if 0
	TEMP_ACT_NORM_PW_CONTRL	=	0x1,
#endif
	TEMP_ACT_NORM_SLAVE_ON	=	0x4,
	TEMP_ACT_NORM_PA_ON	=	0x8,
	TEMP_ACT_NORM_BTS_SRV_ON=	0x10,
};

enum sysmobts_temp_state {
	STATE_NORMAL,		/* Everything is fine */
	STATE_WARNING_HYST,	/* Go back to normal next? */
	STATE_WARNING,		/* We are above the warning threshold */
	STATE_CRITICAL,		/* We have an issue. Wait for below warning */
};

/**
 * Temperature Limits. We separate from a threshold
 * that will generate a warning and one that is so
 * severe that an action will be taken.
 */
struct sysmobts_temp_limit {
	int thresh_warn;
	int thresh_crit;
};

enum mgr_vty_node {
	MGR_NODE = _LAST_OSMOVTY_NODE + 1,

	ACT_NORM_NODE,
	ACT_WARN_NODE,
	ACT_CRIT_NODE,
	LIMIT_RF_NODE,
	LIMIT_DIGITAL_NODE,
	LIMIT_BOARD_NODE,
	LIMIT_PA_NODE,
};

struct sysmobts_mgr_instance {
	const char *config_file;

	struct sysmobts_temp_limit rf_limit;
	struct sysmobts_temp_limit digital_limit;

	/* Only available on sysmobts 2050 */
	struct sysmobts_temp_limit board_limit;
	struct sysmobts_temp_limit pa_limit;

	int action_norm;
	int action_warn;
	int action_crit;

	enum sysmobts_temp_state state;

	struct {
		int initial_calib_started;
		int is_up;
		struct osmo_timer_list recon_timer;
		struct ipa_client_conn *bts_conn;

		int state;
		struct osmo_timer_list timer;
		uint32_t last_seqno;

		/* gps structure to see if there is a fix */
		int gps_open;
		struct osmo_fd gpsfd;
		struct gps_data_t gpsdata;
		struct osmo_timer_list fix_timeout;

		/* Loop/Re-try control */
		int calib_from_loop;
		struct osmo_timer_list calib_timeout;
	} calib;
};

int sysmobts_mgr_vty_init(void);
int sysmobts_mgr_parse_config(struct sysmobts_mgr_instance *mgr);
int sysmobts_mgr_nl_init(void);
int sysmobts_mgr_temp_init(struct sysmobts_mgr_instance *mgr,
			   struct ctrl_connection *ctrl);
const char *sysmobts_mgr_temp_get_state(enum sysmobts_temp_state state);


int sysmobts_mgr_calib_init(struct sysmobts_mgr_instance *mgr);
int sysmobts_mgr_calib_run(struct sysmobts_mgr_instance *mgr);


extern void *tall_mgr_ctx;

#endif
