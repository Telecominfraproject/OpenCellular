#ifndef TRX_IF_H
#define TRX_IF_H

extern int transceiver_available;

struct trx_l1h;

struct trx_ctrl_msg {
	struct llist_head	list;
	char 			cmd[28];
	char 			params[100];
	int			cmd_len;
	int			params_len;
	int			critical;
};

int trx_if_cmd_poweroff(struct trx_l1h *l1h);
int trx_if_cmd_poweron(struct trx_l1h *l1h);
int trx_if_cmd_settsc(struct trx_l1h *l1h, uint8_t tsc);
int trx_if_cmd_setbsic(struct trx_l1h *l1h, uint8_t bsic);
int trx_if_cmd_setrxgain(struct trx_l1h *l1h, int db);
int trx_if_cmd_setpower(struct trx_l1h *l1h, int db);
int trx_if_cmd_setmaxdly(struct trx_l1h *l1h, int dly);
int trx_if_cmd_setmaxdlynb(struct trx_l1h *l1h, int dly);
int trx_if_cmd_setslot(struct trx_l1h *l1h, uint8_t tn, uint8_t type);
int trx_if_cmd_rxtune(struct trx_l1h *l1h, uint16_t arfcn);
int trx_if_cmd_txtune(struct trx_l1h *l1h, uint16_t arfcn);
int trx_if_cmd_handover(struct trx_l1h *l1h, uint8_t tn, uint8_t ss);
int trx_if_cmd_nohandover(struct trx_l1h *l1h, uint8_t tn, uint8_t ss);
int trx_if_send_burst(struct trx_l1h *l1h, uint8_t tn, uint32_t fn, uint8_t pwr,
	const ubit_t *bits, uint16_t nbits);
int trx_if_powered(struct trx_l1h *l1h);

#endif /* TRX_IF_H */
