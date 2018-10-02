#pragma once

struct gsm_bts;
struct osmo_fd;

/**
 * The default path lc15bts will listen for incoming
 * registrations for OML routing and sending.
 */
#define OML_ROUTER_PATH "/var/run/lc15bts_oml_router"


int oml_router_init(struct gsm_bts *bts, const char *path, struct osmo_fd *accept, struct osmo_fd *read);
