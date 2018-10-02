#ifndef _OML_H
#define _OML_H

#include <osmocom/gsm/protocol/gsm_12_21.h>

struct gsm_bts;
struct gsm_abis_mo;
struct msgb;
struct gsm_lchan;


int oml_init(struct gsm_abis_mo *mo);
int down_oml(struct gsm_bts *bts, struct msgb *msg);

struct msgb *oml_msgb_alloc(void);
int oml_send_msg(struct msgb *msg, int is_mauf);
int oml_mo_send_msg(struct gsm_abis_mo *mo, struct msgb *msg, uint8_t msg_type);
int oml_mo_opstart_ack(struct gsm_abis_mo *mo);
int oml_mo_opstart_nack(struct gsm_abis_mo *mo, uint8_t nack_cause);
int oml_mo_statechg_ack(struct gsm_abis_mo *mo);
int oml_mo_statechg_nack(struct gsm_abis_mo *mo, uint8_t nack_cause);

/* Change the state and send STATE CHG REP */
int oml_mo_state_chg(struct gsm_abis_mo *mo, int op_state, int avail_state);

/* First initialization of MO, does _not_ generate state changes */
void oml_mo_state_init(struct gsm_abis_mo *mo, int op_state, int avail_state);

/* Update admin state and send ACK/NACK */
int oml_mo_rf_lock_chg(struct gsm_abis_mo *mo, uint8_t mute_state[8],
		       int success);

/* Transmit STATE CHG REP even if there was no state change */
int oml_tx_state_changed(struct gsm_abis_mo *mo);

int oml_mo_tx_sw_act_rep(struct gsm_abis_mo *mo);

int oml_fom_ack_nack(struct msgb *old_msg, uint8_t cause);

int oml_mo_fom_ack_nack(struct gsm_abis_mo *mo, uint8_t orig_msg_type,
			uint8_t cause);

/* Configure LAPDm T200 timers for this lchan according to OML */
int oml_set_lchan_t200(struct gsm_lchan *lchan);
extern const unsigned int oml_default_t200_ms[7];

/* Transmit failure event report */
void oml_fail_rep(uint16_t cause_value, const char *fmt, ...);

#endif // _OML_H */
