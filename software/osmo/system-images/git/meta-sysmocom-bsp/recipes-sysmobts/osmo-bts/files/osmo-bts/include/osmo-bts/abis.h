#ifndef _ABIS_H
#define _ABIS_H

#include <osmocom/core/select.h>
#include <osmocom/core/timer.h>

#include <osmo-bts/gsm_data.h>

#define	OML_RETRY_TIMER		5
#define	OML_PING_TIMER		20

enum {
	LINK_STATE_IDLE = 0,
	LINK_STATE_RETRYING,
	LINK_STATE_CONNECTING,
	LINK_STATE_CONNECT,
};

void abis_init(struct gsm_bts *bts);
struct e1inp_line *abis_open(struct gsm_bts *bts, char *dst_host,
			     char *model_name);


int abis_oml_sendmsg(struct msgb *msg);
int abis_bts_rsl_sendmsg(struct msgb *msg);

uint32_t get_signlink_remote_ip(struct e1inp_sign_link *link);

#endif /* _ABIS_H */
