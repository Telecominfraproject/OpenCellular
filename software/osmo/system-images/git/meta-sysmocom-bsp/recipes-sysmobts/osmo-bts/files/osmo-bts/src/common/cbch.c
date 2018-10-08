/* Cell Broadcast routines */

/* (C) 2014 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/linuxlist.h>
#include <osmocom/gsm/protocol/gsm_04_12.h>

#include <osmo-bts/bts.h>
#include <osmo-bts/cbch.h>
#include <osmo-bts/logging.h>

struct smscb_msg {
	struct llist_head list;		/* list in smscb_state.queue */

	uint8_t msg[GSM412_MSG_LEN];	/* message buffer */
	uint8_t next_seg;		/* next segment number */
	uint8_t num_segs;		/* total number of segments */
};

static int get_smscb_null_block(uint8_t *out)
{
	struct gsm412_block_type *block_type = (struct gsm412_block_type *) out;

	block_type->spare = 0;
	block_type->lpd = 1;
	block_type->seq_nr = GSM412_SEQ_NULL_MSG;
	block_type->lb = 0;
	memset(out+1, GSM_MACBLOCK_PADDING, GSM412_BLOCK_LEN);

	return 0;
}

/* get the next block of the current CB message */
static int get_smscb_block(struct gsm_bts *bts, uint8_t *out)
{
	int to_copy;
	struct gsm412_block_type *block_type;
	struct smscb_msg *msg = bts->smscb_state.cur_msg;

	if (!msg) {
		/* No message: Send NULL mesage */
		return get_smscb_null_block(out);
	}
	OSMO_ASSERT(msg->next_seg < 4);

	block_type = (struct gsm412_block_type *) out++;

	/* LPD is always 01 */
	block_type->spare = 0;
	block_type->lpd = 1;

	/* determine how much data to copy */
	to_copy = GSM412_MSG_LEN - (msg->next_seg * GSM412_BLOCK_LEN);
	if (to_copy > GSM412_BLOCK_LEN)
		to_copy = GSM412_BLOCK_LEN;
	OSMO_ASSERT(to_copy >= 0);

	/* copy data and increment index */
	memcpy(out, &msg->msg[msg->next_seg * GSM412_BLOCK_LEN], to_copy);

	/* set + increment sequence number */
	block_type->seq_nr = msg->next_seg++;

	/* determine if this is the last block */
	if (block_type->seq_nr + 1 == msg->num_segs)
		block_type->lb = 1;
	else
		block_type->lb = 0;

	if (block_type->lb == 1) {
		/* remove/release the message memory */
		talloc_free(bts->smscb_state.cur_msg);
		bts->smscb_state.cur_msg = NULL;
	}

	return block_type->lb;
}

static const uint8_t last_block_rsl2um[4] = {
	[RSL_CB_CMD_LASTBLOCK_4]	= 4,
	[RSL_CB_CMD_LASTBLOCK_1]	= 1,
	[RSL_CB_CMD_LASTBLOCK_2]	= 2,
	[RSL_CB_CMD_LASTBLOCK_3]	= 3,
};


/* incoming SMS broadcast command from RSL */
int bts_process_smscb_cmd(struct gsm_bts *bts,
			  struct rsl_ie_cb_cmd_type cmd_type,
			  uint8_t msg_len, const uint8_t *msg)
{
	struct smscb_msg *scm;

	if (msg_len > sizeof(scm->msg)) {
		LOGP(DLSMS, LOGL_ERROR,
		     "Cannot process SMSCB of %u bytes (max %zu)\n",
		     msg_len, sizeof(scm->msg));
		return -EINVAL;
	}

	scm = talloc_zero_size(bts, sizeof(*scm));
	if (!scm)
		return -1;

	/* initialize entire message with default padding */
	memset(scm->msg, GSM_MACBLOCK_PADDING, sizeof(scm->msg));
	/* next segment is first segment */
	scm->next_seg = 0;

	switch (cmd_type.command) {
	case RSL_CB_CMD_TYPE_NORMAL:
	case RSL_CB_CMD_TYPE_SCHEDULE:
	case RSL_CB_CMD_TYPE_NULL:
		scm->num_segs = last_block_rsl2um[cmd_type.last_block&3];
		memcpy(scm->msg, msg, msg_len);
		/* def_bcast is ignored */
		break;
	case RSL_CB_CMD_TYPE_DEFAULT:
		/* use def_bcast, ignore command  */
		/* def_bcast == 0: normal mess */
		break;
	}

	llist_add_tail(&scm->list, &bts->smscb_state.queue);
	/* FIXME: limit queue size and optionally send CBCH LOAD Information (overflow) via RSL */

	return 0;
}

static struct smscb_msg *select_next_smscb(struct gsm_bts *bts)
{
	struct smscb_msg *msg;

	msg = llist_first_entry_or_null(&bts->smscb_state.queue, struct smscb_msg, list);
	if (!msg) {
		/* FIXME: send CBCH LOAD Information (underflow) via RSL */
		return NULL;
	}

	llist_del(&msg->list);

	return msg;
}

/* call-back from bts model specific code when it wants to obtain a CBCH
 * block for a given gsm_time.  outbuf must have 23 bytes of space. */
int bts_cbch_get(struct gsm_bts *bts, uint8_t *outbuf, struct gsm_time *g_time)
{
	uint32_t fn = gsm_gsmtime2fn(g_time);
	/* According to 05.02 Section 6.5.4 */
	uint32_t tb = (fn / 51) % 8;
	int rc = 0;

	/* The multiframes used for the basic cell broadcast channel
	 * shall be those in * which TB = 0,1,2 and 3. The multiframes
	 * used for the extended cell broadcast channel shall be those
	 * in which TB = 4, 5, 6 and 7 */

	/* The SMSCB header shall be sent in the multiframe in which TB
	 * = 0 for the basic, and TB = 4 for the extended cell
	 * broadcast channel. */

	switch (tb) {
	case 0:
		/* select a new SMSCB message */
		bts->smscb_state.cur_msg = select_next_smscb(bts);
		rc = get_smscb_block(bts, outbuf);
		break;
	case 1: case 2: case 3:
		rc = get_smscb_block(bts, outbuf);
		break;
	case 4: case 5: case 6: case 7:
		/* always send NULL frame in extended CBCH for now */
		rc = get_smscb_null_block(outbuf);
		break;
	}

	return rc;
}
