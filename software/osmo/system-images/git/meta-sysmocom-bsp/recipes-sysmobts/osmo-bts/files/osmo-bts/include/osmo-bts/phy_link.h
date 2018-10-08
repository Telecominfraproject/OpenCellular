#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <osmocom/core/linuxlist.h>

#include <osmo-bts/scheduler.h>

#include <linux/if_packet.h>
#include "btsconfig.h"

struct gsm_bts_trx;
struct virt_um_inst;

enum phy_link_type {
	PHY_LINK_T_NONE,
	PHY_LINK_T_SYSMOBTS,
	PHY_LINK_T_OSMOTRX,
	PHY_LINK_T_VIRTUAL,
};

enum phy_link_state {
	PHY_LINK_SHUTDOWN,
	PHY_LINK_CONNECTING,
	PHY_LINK_CONNECTED,
};

/* A PHY link represents the connection to a given PHYsical layer
 * implementation.  That PHY link contains 1...N PHY instances, one for
 * each TRX */
struct phy_link {
	struct llist_head list;
	int num;
	enum phy_link_type type;
	enum phy_link_state state;
	struct llist_head instances;
	char *description;
	union {
		struct {
		} sysmobts;
		struct {
			char *local_ip;
			char *remote_ip;
			uint16_t base_port_local;
			uint16_t base_port_remote;
			struct osmo_fd trx_ofd_clk;
			bool trx_ta_loop;
			bool trx_ms_power_loop;
			int8_t trx_target_rssi;
			uint32_t clock_advance;
			uint32_t rts_advance;
			bool use_legacy_setbsic;
		} osmotrx;
		struct {
			char *mcast_dev;		/* Network device for multicast */
			char *bts_mcast_group;		/* BTS are listening to this group */
			uint16_t bts_mcast_port;
			char *ms_mcast_group;		/* MS are listening to this group */
			uint16_t ms_mcast_port;
			struct virt_um_inst *virt_um;
		} virt;
		struct {
			/* MAC address of the PHY */
			struct sockaddr_ll phy_addr;
			/* Network device name */
			char *netdev_name;

			/* configuration */
			uint32_t rf_port_index;
#if OCTPHY_USE_ANTENNA_ID == 1
			uint32_t rx_ant_id;
			uint32_t tx_ant_id;
#endif
			uint32_t rx_gain_db;
			bool tx_atten_flag;
			uint32_t tx_atten_db;
			bool over_sample_16x;
#if OCTPHY_MULTI_TRX == 1
			/* arfcn used by TRX with id 0 */
			uint16_t center_arfcn;
#endif

			struct octphy_hdl *hdl;
		} octphy;
	} u;
};

struct phy_instance {
	/* liked inside phy_link.linstances */
	struct llist_head list;
	int num;
	char *description;
	char version[MAX_VERSION_LENGTH];
	/* pointer to the PHY link to which we belong */
	struct phy_link *phy_link;

	/* back-pointer to the TRX to which we're associated */
	struct gsm_bts_trx *trx;

	union {
		struct {
			/* configuration */
			uint8_t clk_use_eeprom;
			uint32_t dsp_trace_f;
			int clk_cal;
			uint8_t clk_src;
			char *calib_path;

			struct femtol1_hdl *hdl;
		} sysmobts;
		struct {
			struct trx_l1h *hdl;
			bool sw_act_reported;
		} osmotrx;
		struct {
			struct l1sched_trx sched;
		} virt;
		struct {
			/* logical transceiver number within one PHY */
			uint32_t trx_id;
			/* trx lock state variable */
			int trx_locked;
		} octphy;
		struct {
			/* configuration */
			uint32_t dsp_trace_f;
			char *calib_path;
			int minTxPower;
			int maxTxPower;
			struct lc15l1_hdl *hdl;
			uint8_t max_cell_size;		/* 0:166 qbits*/
			uint8_t diversity_mode;		/* 0: SISO A, 1: SISO B, 2: MRC */
			uint8_t pedestal_mode;		/* 0: unused TS is OFF, 1: unused TS is in minimum Tx power */
			uint8_t dsp_alive_period;	/* DSP alive timer period  */
			uint8_t tx_pwr_adj_mode;	/* 0: no auto adjust power, 1: auto adjust power using RMS detector */
			uint8_t tx_pwr_red_8psk;	/* 8-PSK maximum Tx power reduction level in dB */
		} lc15;
                struct {
                        /* configuration */
                        uint32_t dsp_trace_f;
                        char *calib_path;
                        int minTxPower;
                        int maxTxPower;
                        struct oc2gl1_hdl *hdl;
                        uint8_t max_cell_size;          /* 0:166 qbits*/
                        uint8_t pedestal_mode;          /* 0: unused TS is OFF, 1: unused TS is in minimum Tx power */
                        uint8_t dsp_alive_period;       /* DSP alive timer period  */
                        uint8_t tx_pwr_adj_mode;        /* 0: no auto adjust power, 1: auto adjust power using RMS detector */
                        uint8_t tx_pwr_red_8psk;        /* 8-PSK maximum Tx power reduction level in dB */
                        uint8_t tx_c0_idle_pwr_red;     /* C0 idle slot Tx power reduction level in dB */
                } oc2g;
	} u;
};

struct phy_link *phy_link_by_num(int num);
struct phy_link *phy_link_create(void *ctx, int num);
void phy_link_destroy(struct phy_link *plink);
void phy_link_state_set(struct phy_link *plink, enum phy_link_state state);
int phy_links_open(void);

struct phy_instance *phy_instance_by_num(struct phy_link *plink, int num);
struct phy_instance *phy_instance_create(struct phy_link *plink, int num);
void phy_instance_link_to_trx(struct phy_instance *pinst, struct gsm_bts_trx *trx);
void phy_instance_destroy(struct phy_instance *pinst);
const char *phy_instance_name(struct phy_instance *pinst);

void phy_user_statechg_notif(struct phy_instance *pinst, enum phy_link_state link_state);

static inline struct phy_instance *trx_phy_instance(struct gsm_bts_trx *trx)
{
	OSMO_ASSERT(trx);
	return trx->role_bts.l1h;
}

int bts_model_phy_link_open(struct phy_link *plink);
