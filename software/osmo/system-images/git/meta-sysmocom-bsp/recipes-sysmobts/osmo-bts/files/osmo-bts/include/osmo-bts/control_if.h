#pragma once

int bts_ctrl_cmds_install(struct gsm_bts *bts);
struct ctrl_handle *bts_controlif_setup(struct gsm_bts *bts,
					const char *bind_addr, uint16_t port);
