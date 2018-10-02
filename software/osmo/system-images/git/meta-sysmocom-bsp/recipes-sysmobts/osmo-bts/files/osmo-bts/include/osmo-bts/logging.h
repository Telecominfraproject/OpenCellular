#ifndef _LOGGING_H
#define _LOGGING_H

#define DEBUG
#include <osmocom/core/logging.h>

enum {
	DRSL,
	DOML,
	DRLL,
	DRR,
	DMEAS,
	DPAG,
	DL1C,
	DL1P,
	DDSP,
	DPCU,
	DHO,
	DTRX,
	DLOOP,
	DABIS,
	DRTP,
	DSUM,
};

extern const struct log_info bts_log_info;

/* LOGP with gsm_time prefix */
#define LOGPGT(ss, lvl, gt, fmt, args...) \
	LOGP(ss, lvl, "%s " fmt, osmo_dump_gsmtime(gt), ## args)
#define DEBUGPGT(ss, gt, fmt, args...) \
	LOGP(ss, LOGL_DEBUG, "%s " fmt, osmo_dump_gsmtime(gt), ## args)

/* LOGP with frame number prefix */
#define LOGPFN(ss, lvl, fn, fmt, args...) \
	LOGP(ss, lvl, "%s " fmt, gsm_fn_as_gsmtime_str(fn), ## args)
#define DEBUGPFN(ss, fn, fmt, args...) \
	LOGP(ss, LOGL_DEBUG, "%s " fmt, gsm_fn_as_gsmtime_str(fn), ## args)

#endif /* _LOGGING_H */
