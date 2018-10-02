#ifndef BTS_MODEL_H
#define BTS_MODEL_H

#include <stdint.h>

#include <osmocom/gsm/tlv.h>
#include <osmocom/gsm/gsm_utils.h>

#include <osmo-bts/gsm_data.h>

struct phy_link;
struct phy_instance;

/* BTS model specific functions needed by the common code */

int bts_model_init(struct gsm_bts *bts);

int bts_model_check_oml(struct gsm_bts *bts, uint8_t msg_type,
			struct tlv_parsed *old_attr, struct tlv_parsed *new_attr,
			void *obj);

int bts_model_apply_oml(struct gsm_bts *bts, struct msgb *msg,
			struct tlv_parsed *new_attr, int obj_kind, void *obj);

int bts_model_opstart(struct gsm_bts *bts, struct gsm_abis_mo *mo,
		      void *obj);

int bts_model_chg_adm_state(struct gsm_bts *bts, struct gsm_abis_mo *mo,
			    void *obj, uint8_t adm_state);

int bts_model_trx_deact_rf(struct gsm_bts_trx *trx);
int bts_model_trx_close(struct gsm_bts_trx *trx);

int bts_model_vty_init(struct gsm_bts *bts);

void bts_model_config_write_bts(struct vty *vty, struct gsm_bts *bts);
void bts_model_config_write_trx(struct vty *vty, struct gsm_bts_trx *trx);
void bts_model_config_write_phy(struct vty *vty, struct phy_link *plink);
void bts_model_config_write_phy_inst(struct vty *vty, struct phy_instance *pinst);

int bts_model_oml_estab(struct gsm_bts *bts);

int bts_model_change_power(struct gsm_bts_trx *trx, int p_trxout_mdBm);
int bts_model_adjst_ms_pwr(struct gsm_lchan *lchan);

int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap);

int bts_model_lchan_deactivate(struct gsm_lchan *lchan);
int bts_model_lchan_deactivate_sacch(struct gsm_lchan *lchan);

void bts_model_abis_close(struct gsm_bts *bts);

int bts_model_ctrl_cmds_install(struct gsm_bts *bts);

int bts_model_handle_options(int argc, char **argv);
void bts_model_print_help();

void bts_model_phy_link_set_defaults(struct phy_link *plink);
void bts_model_phy_instance_set_defaults(struct phy_instance *pinst);

int bts_model_ts_disconnect(struct gsm_bts_trx_ts *ts);
int bts_model_ts_connect(struct gsm_bts_trx_ts *ts, enum gsm_phys_chan_config as_pchan);

#endif
