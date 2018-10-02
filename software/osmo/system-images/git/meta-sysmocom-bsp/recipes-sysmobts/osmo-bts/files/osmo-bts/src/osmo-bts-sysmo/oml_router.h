#pragma once

struct gsm_bts;
struct osmo_fd;

/**
 * The default path sysmobts will listen for incoming
 * registrations for OML routing and sending.
 */
#define OML_ROUTER_PATH "/var/run/sysmobts_oml_router"


int oml_router_init(struct gsm_bts *bts, const char *path, struct osmo_fd *accept, struct osmo_fd *read);
