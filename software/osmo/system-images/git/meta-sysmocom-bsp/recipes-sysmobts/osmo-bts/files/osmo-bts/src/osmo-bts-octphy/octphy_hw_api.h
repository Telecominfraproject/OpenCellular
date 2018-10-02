#pragma once

#include <stdint.h>
#include "l1_if.h"
#include <octphy/octvc1/hw/octvc1_hw_api.h>

static const struct value_string radio_std_vals[] = {
	{ cOCTVC1_RADIO_STANDARD_ENUM_GSM,	"GSM" },
	{ cOCTVC1_RADIO_STANDARD_ENUM_UMTS,	"UMTS" },
	{ cOCTVC1_RADIO_STANDARD_ENUM_LTE,	"LTE" },
	{ cOCTVC1_RADIO_STANDARD_ENUM_INVALID,	"INVALID" },
	{ 0, NULL }
};

static const struct value_string clocksync_state_vals[] = {
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_UNINITIALIZE,
							"Uninitialized" },
/* Note: Octasic renamed cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_UNUSED to
 * cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_IDLE. The following ifdef
 * statement ensures that older headers still work. */
#ifdef cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_UNUSED
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_UNUSED,	"Unused" },
#else
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_IDLE,	"Idle" },
#endif
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_NO_EXT_CLOCK,
							"No External Clock" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_LOCKED,	"Locked" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_UNLOCKED,"Unlocked" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_ERROR,	"Error" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_DISABLE,	"Disabled" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_STATE_ENUM_LOSS_EXT_CLOCK,
							"Loss of Ext Clock" },
	{ 0, NULL }
};

#if OCTPHY_USE_CLOCK_SYNC_MGR_STATS_DAC_STATE == 1
static const struct value_string clocksync_dac_vals[] = {
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_DAC_STATE_ENUM_UNUSED, "Unused" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_DAC_STATE_ENUM_MASTER, "Master" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_DAC_STATE_ENUM_SLAVE, "Slave" },
	{ cOCTVC1_HW_CLOCK_SYNC_MGR_DAC_STATE_ENUM_FREE_RUNNING, "Free_Run"},
	{ 0, NULL }
};
#endif

static const struct value_string usr_process_id[] = {
	{ cOCTVC1_USER_ID_PROCESS_ENUM_INVALID, "Invalid" },
	{ cOCTVC1_USER_ID_PROCESS_ENUM_MAIN_APP, "MainApp" },
	{ cOCTVC1_USER_ID_PROCESS_ENUM_MAIN_ROUTER, "MainRouter" },
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_DL_0, "DL"},
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_ULIM_0, "ULIM" },
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_ULOM_0, "ULOM" },
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_SCHED_0, "SCHED" },
#ifdef cOCTVC1_USER_ID_PROCESS_ENUM_GSM_DECOMB
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_DECOMB, "DECOMB"},
#endif
#ifdef cOCTVC1_USER_ID_PROCESS_ENUM_GSM_ULEQ
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_ULEQ, "ULEQ" },
#endif
#ifdef cOCTVC1_USER_ID_PROCESS_ENUM_GSM_TEST
	{ cOCTVC1_USER_ID_PROCESS_ENUM_GSM_TEST, "TEST"},
#endif
	{ 0, NULL }
};

typedef void octphy_hw_get_cb(struct msgb *resp, void *data);

struct octphy_hw_get_cb_data {
	octphy_hw_get_cb* cb;
	void *data;
};

int octphy_hw_get_pcb_info(struct octphy_hdl *fl1h);
int octphy_hw_get_rf_port_info(struct octphy_hdl *fl1h, uint32_t index);
int octphy_hw_get_rf_port_stats(struct octphy_hdl *fl1h, uint32_t index,
				struct octphy_hw_get_cb_data *cb_data);
int octphy_hw_get_rf_ant_rx_config(struct octphy_hdl *fl1h, uint32_t port_idx,
				   uint32_t ant_idx);
int octphy_hw_get_rf_ant_tx_config(struct octphy_hdl *fl1h, uint32_t port_idx,
				   uint32_t ant_idx);
int octphy_hw_get_clock_sync_info(struct octphy_hdl *fl1h);
int octphy_hw_get_clock_sync_stats(struct octphy_hdl *fl1h,
				   struct octphy_hw_get_cb_data *cb_data);
