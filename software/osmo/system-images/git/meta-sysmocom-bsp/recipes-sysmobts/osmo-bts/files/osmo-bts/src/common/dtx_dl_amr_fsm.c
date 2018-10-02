/* DTX DL AMR FSM */

/* (C) 2016 by sysmocom s.f.m.c. GmbH
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
 *
 */

#include <osmo-bts/dtx_dl_amr_fsm.h>
#include <osmo-bts/logging.h>

void dtx_fsm_voice(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_VOICE:
	case E_FACCH:
		break;
	case E_SID_F:
		osmo_fsm_inst_state_chg(fi, ST_SID_F1, 0, 0);
		break;
	case E_SID_U:
		osmo_fsm_inst_state_chg(fi, ST_U_NOINH, 0, 0);
		break;
	case E_INHIB:
		osmo_fsm_inst_state_chg(fi, ST_F1_INH_V, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Inexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_sid_f1(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_SID_F:
/* FIXME: what shall we do if we get SID-FIRST _again_ (twice in a row)?
   Was observed during testing, let's just ignore it for now */
		break;
	case E_SID_U:
		osmo_fsm_inst_state_chg(fi, ST_U_NOINH, 0, 0);
		break;
	case E_FACCH:
		osmo_fsm_inst_state_chg(fi, ST_F1_INH_F, 0, 0);
		break;
	case E_FIRST:
		osmo_fsm_inst_state_chg(fi, ST_SID_F2, 0, 0);
		break;
	case E_ONSET:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_V, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_sid_f2(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_U_NOINH, 0, 0);
		break;
	case E_FACCH:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_F, 0, 0);
		break;
	case E_ONSET:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_V, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_f1_inh_v(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_F1_INH_V_REC, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_f1_inh_f(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_F1_INH_F_REC, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_u_inh_v(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_U_INH_V_REC, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_u_inh_f(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_U_INH_F_REC, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_f1_inh_v_rec(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_VOICE:
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_VOICE, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_f1_inh_f_rec(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_FACCH:
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_FACCH, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_u_inh_v_rec(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_VOICE:
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_VOICE, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_u_inh_f_rec(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_FACCH:
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_FACCH, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_u_noinh(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_FACCH:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_F, 0, 0);
		break;
	case E_VOICE:
		osmo_fsm_inst_state_chg(fi, ST_VOICE, 0, 0);
		break;
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_SID_U, 0, 0);
		break;
	case E_SID_U:
	case E_SID_F:
/* FIXME: what shall we do if we get SID-FIRST _after_ sending SID-UPDATE?
   Was observed during testing, let's just ignore it for now */
		break;
	case E_ONSET:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_V, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_sid_upd(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_FACCH:
		osmo_fsm_inst_state_chg(fi, ST_U_INH_F, 0, 0);
		break;
	case E_VOICE:
		osmo_fsm_inst_state_chg(fi, ST_VOICE, 0, 0);
		break;
	case E_INHIB:
		osmo_fsm_inst_state_chg(fi, ST_U_INH_V, 0, 0);
		break;
	case E_SID_U:
	case E_SID_F:
		osmo_fsm_inst_state_chg(fi, ST_U_NOINH, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_onset_v(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_V_REC, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_onset_f(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_ONSET_F_REC, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_onset_v_rec(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_VOICE, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_onset_f_rec(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_FACCH, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

void dtx_fsm_facch(struct osmo_fsm_inst *fi, uint32_t event, void *data)
{
	switch (event) {
	case E_SID_U:
	case E_SID_F:
	case E_FACCH:
		break;
	case E_VOICE:
		osmo_fsm_inst_state_chg(fi, ST_VOICE, 0, 0);
		break;
	case E_COMPL:
		osmo_fsm_inst_state_chg(fi, ST_SID_F1, 0, 0);
		break;
	default:
		LOGP(DL1P, LOGL_ERROR, "Unexpected event %d\n", event);
		OSMO_ASSERT(0);
		break;
	}
}

static struct osmo_fsm_state dtx_dl_amr_fsm_states[] = {
	/* default state for non-DTX and DTX when SPEECH is in progress */
	[ST_VOICE] = {
		.in_event_mask = X(E_SID_F) | X(E_SID_U) | X(E_VOICE) | X(E_FACCH) | X(E_INHIB),
		.out_state_mask = X(ST_SID_F1) | X(ST_U_NOINH) | X(ST_F1_INH_V),
		.name = "Voice",
		.action = dtx_fsm_voice,
	},
	/* SID-FIRST or SID-FIRST-P1 in case of AMR HR:
	   start of silence period (might be interrupted in case of AMR HR) */
	[ST_SID_F1]= {
		.in_event_mask = X(E_SID_F) | X(E_SID_U) | X(E_FACCH) | X(E_FIRST) | X(E_ONSET),
		.out_state_mask = X(ST_U_NOINH) | X(ST_ONSET_F) | X(ST_SID_F2) | X(ST_ONSET_V),
		.name = "SID-FIRST (P1)",
		.action = dtx_fsm_sid_f1,
	},
	/* SID-FIRST P2 (only for AMR HR):
	   actual start of silence period in case of AMR HR */
	[ST_SID_F2]= {
		.in_event_mask = X(E_COMPL) | X(E_FACCH) | X(E_ONSET),
		.out_state_mask = X(ST_U_NOINH) | X(ST_ONSET_F) | X(ST_ONSET_V),
		.name = "SID-FIRST (P2)",
		.action = dtx_fsm_sid_f2,
	},
	/* SID-FIRST Inhibited: incoming SPEECH (only for AMR HR) */
	[ST_F1_INH_V]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_F1_INH_V_REC),
		.name = "SID-FIRST (Inh, SPEECH)",
		.action = dtx_fsm_f1_inh_v,
	},
	/* SID-FIRST Inhibited: incoming FACCH frame (only for AMR HR) */
	[ST_F1_INH_F]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_F1_INH_F_REC),
		.name = "SID-FIRST (Inh, FACCH)",
		.action = dtx_fsm_f1_inh_f,
	},
	/* SID-UPDATE Inhibited: incoming SPEECH (only for AMR HR) */
	[ST_U_INH_V]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_U_INH_V_REC),
		.name = "SID-UPDATE (Inh, SPEECH)",
		.action = dtx_fsm_u_inh_v,
	},
	/* SID-UPDATE Inhibited: incoming FACCH frame (only for AMR HR) */
	[ST_U_INH_F]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_U_INH_F_REC),
		.name = "SID-UPDATE (Inh, FACCH)",
		.action = dtx_fsm_u_inh_f,
	},
	/* SID-UPDATE: Inhibited not allowed (only for AMR HR) */
	[ST_U_NOINH]= {
		.in_event_mask = X(E_FACCH) | X(E_VOICE) | X(E_COMPL) | X(E_SID_U) | X(E_SID_F) | X(E_ONSET),
		.out_state_mask = X(ST_ONSET_F) | X(ST_VOICE) | X(ST_SID_U) | X(ST_ONSET_V),
		.name = "SID-UPDATE (NoInh)",
		.action = dtx_fsm_u_noinh,
	},
	/* SID-FIRST Inhibition recursion in progress:
	   Inhibit itself was already sent, now have to send the voice that caused it */
	[ST_F1_INH_V_REC]= {
		.in_event_mask = X(E_COMPL) | X(E_VOICE),
		.out_state_mask = X(ST_VOICE),
		.name = "SID-FIRST (Inh, SPEECH, Rec)",
		.action = dtx_fsm_f1_inh_v_rec,
	},
	/* SID-FIRST Inhibition recursion in progress:
	   Inhibit itself was already sent, now have to send the data that caused it */
	[ST_F1_INH_F_REC]= {
		.in_event_mask = X(E_COMPL) | X(E_FACCH),
		.out_state_mask = X(ST_FACCH),
		.name = "SID-FIRST (Inh, FACCH, Rec)",
		.action = dtx_fsm_f1_inh_f_rec,
	},
	/* SID-UPDATE Inhibition recursion in progress:
	   Inhibit itself was already sent, now have to send the voice that caused it */
	[ST_U_INH_V_REC]= {
		.in_event_mask = X(E_COMPL) | X(E_VOICE),
		.out_state_mask = X(ST_VOICE),
		.name = "SID-UPDATE (Inh, SPEECH, Rec)",
		.action = dtx_fsm_u_inh_v_rec,
	},
	/* SID-UPDATE Inhibition recursion in progress:
	   Inhibit itself was already sent, now have to send the data that caused it */
	[ST_U_INH_F_REC]= {
		.in_event_mask = X(E_COMPL) | X(E_FACCH),
		.out_state_mask = X(ST_FACCH),
		.name = "SID-UPDATE (Inh, FACCH, Rec)",
		.action = dtx_fsm_u_inh_f_rec,
	},
	/* Silence period with periodic comfort noise data updates */
	[ST_SID_U]= {
		.in_event_mask = X(E_FACCH) | X(E_VOICE) | X(E_INHIB) | X(E_SID_U) | X(E_SID_F),
		.out_state_mask = X(ST_ONSET_F) | X(ST_VOICE) | X(ST_U_INH_V) | X(ST_U_INH_F) | X(ST_U_NOINH),
		.name = "SID-UPDATE (AMR/HR)",
		.action = dtx_fsm_sid_upd,
	},
	/* ONSET - end of silent period due to incoming SPEECH frame */
	[ST_ONSET_V]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_ONSET_V_REC),
		.name = "ONSET (SPEECH)",
		.action = dtx_fsm_onset_v,
	},
	/* ONSET - end of silent period due to incoming FACCH frame */
	[ST_ONSET_F]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_ONSET_F_REC),
		.name = "ONSET (FACCH)",
		.action = dtx_fsm_onset_f,
	},
	/* ONSET recursion in progress:
	   ONSET itself was already sent, now have to send the voice that caused it */
	[ST_ONSET_V_REC]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_VOICE),
		.name = "ONSET (SPEECH, Rec)",
		.action = dtx_fsm_onset_v_rec,
	},
	/* ONSET recursion in progress:
	   ONSET itself was already sent, now have to send the data that caused it */
	[ST_ONSET_F_REC]= {
		.in_event_mask = X(E_COMPL),
		.out_state_mask = X(ST_FACCH),
		.name = "ONSET (FACCH, Rec)",
		.action = dtx_fsm_onset_f_rec,
	},
	/* FACCH sending state */
	[ST_FACCH]= {
		.in_event_mask = X(E_FACCH) | X(E_VOICE) | X(E_COMPL) | X(E_SID_U) | X(E_SID_F),
		.out_state_mask = X(ST_VOICE) | X(ST_SID_F1),
		.name = "FACCH",
		.action = dtx_fsm_facch,
	},
};

const struct value_string dtx_dl_amr_fsm_event_names[] = {
	{ E_VOICE,	"Voice" },
	{ E_ONSET,	"ONSET" },
	{ E_FACCH,	"FACCH" },
	{ E_COMPL,	"Complete" },
	{ E_FIRST,	"FIRST P1->P2" },
	{ E_INHIB,	"Inhibit" },
	{ E_SID_F,	"SID-FIRST" },
	{ E_SID_U,	"SID-UPDATE" },
	{ 0, 		NULL }
};

struct osmo_fsm dtx_dl_amr_fsm = {
	.name = "DTX_DL_AMR_FSM",
	.states = dtx_dl_amr_fsm_states,
	.num_states = ARRAY_SIZE(dtx_dl_amr_fsm_states),
	.event_names = dtx_dl_amr_fsm_event_names,
	.log_subsys = DL1C,
};
