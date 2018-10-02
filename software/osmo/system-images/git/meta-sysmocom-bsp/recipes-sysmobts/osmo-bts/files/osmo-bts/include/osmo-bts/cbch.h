#pragma once

#include <osmocom/gsm/gsm_utils.h>
#include <osmocom/gsm/protocol/gsm_08_58.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/bts.h>

/* incoming SMS broadcast command from RSL */
int bts_process_smscb_cmd(struct gsm_bts *bts,
			  struct rsl_ie_cb_cmd_type cmd_type,
			  uint8_t msg_len, const uint8_t *msg);

/* call-back from bts model specific code when it wants to obtain a CBCH
 * block for a given gsm_time.  outbuf must have 23 bytes of space. */
int bts_cbch_get(struct gsm_bts *bts, uint8_t *outbuf, struct gsm_time *g_time);
