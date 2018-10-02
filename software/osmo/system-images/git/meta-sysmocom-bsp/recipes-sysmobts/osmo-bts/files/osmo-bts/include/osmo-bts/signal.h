#ifndef OSMO_BTS_SIGNAL_H
#define OSMO_BTS_SIGNAL_H

#include <osmocom/core/signal.h>

enum sig_subsys {
	SS_GLOBAL,
	SS_FAIL,
};

enum signals_global {
	S_NEW_SYSINFO,
	S_NEW_OP_STATE,
	S_NEW_NSE_ATTR,
	S_NEW_CELL_ATTR,
	S_NEW_NSVC_ATTR,
};

#endif
