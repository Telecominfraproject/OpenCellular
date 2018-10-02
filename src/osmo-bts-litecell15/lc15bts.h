#ifndef LC15BTS_H
#define LC15BTS_H

#include <stdlib.h>
#include <osmocom/core/utils.h>

#include <nrw/litecell15/litecell15.h>
#include <nrw/litecell15/gsml1const.h>

/*
 * Depending on the firmware version either GsmL1_Prim_t or Litecell15_Prim_t
 * is the bigger struct. For earlier firmware versions the GsmL1_Prim_t was the
 * bigger struct.
 */
#define LC15BTS_PRIM_SIZE \
	(OSMO_MAX(sizeof(Litecell15_Prim_t), sizeof(GsmL1_Prim_t)) + 128)

enum l1prim_type {
	L1P_T_INVALID, /* this must be 0 to detect uninitialized elements */
	L1P_T_REQ,
	L1P_T_CONF,
	L1P_T_IND,
};

enum l1prim_type lc15bts_get_l1prim_type(GsmL1_PrimId_t id);
const struct value_string lc15bts_l1prim_names[GsmL1_PrimId_NUM+1];
GsmL1_PrimId_t lc15bts_get_l1prim_conf(GsmL1_PrimId_t id);

enum l1prim_type lc15bts_get_sysprim_type(Litecell15_PrimId_t id);
const struct value_string lc15bts_sysprim_names[Litecell15_PrimId_NUM+1];
Litecell15_PrimId_t lc15bts_get_sysprim_conf(Litecell15_PrimId_t id);

const struct value_string lc15bts_l1sapi_names[GsmL1_Sapi_NUM+1];
const struct value_string lc15bts_l1status_names[GSML1_STATUS_NUM+1];

const struct value_string lc15bts_tracef_names[29];
const struct value_string lc15bts_tracef_docs[29];

const struct value_string lc15bts_tch_pl_names[15];

const struct value_string lc15bts_clksrc_names[10];

const struct value_string lc15bts_dir_names[6];

enum pdch_cs {
	PDCH_CS_1,
	PDCH_CS_2,
	PDCH_CS_3,
	PDCH_CS_4,
	PDCH_MCS_1,
	PDCH_MCS_2,
	PDCH_MCS_3,
	PDCH_MCS_4,
	PDCH_MCS_5,
	PDCH_MCS_6,
	PDCH_MCS_7,
	PDCH_MCS_8,
	PDCH_MCS_9,
	_NUM_PDCH_CS
};

const uint8_t pdch_msu_size[_NUM_PDCH_CS];

#endif /* LC15BTS_H */
