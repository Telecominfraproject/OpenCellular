#ifndef _PCU_IF_H
#define _PCU_IF_H

extern int pcu_direct;

int pcu_tx_info_ind(void);
int pcu_tx_si13(const struct gsm_bts *bts, bool enable);
int pcu_tx_rts_req(struct gsm_bts_trx_ts *ts, uint8_t is_ptcch, uint32_t fn,
	uint16_t arfcn, uint8_t block_nr);
int pcu_tx_data_ind(struct gsm_bts_trx_ts *ts, uint8_t is_ptcch, uint32_t fn,
	uint16_t arfcn, uint8_t block_nr, uint8_t *data, uint8_t len,
		    int8_t rssi, uint16_t ber10k, int16_t bto, int16_t lqual);
int pcu_tx_rach_ind(struct gsm_bts *bts, int16_t qta, uint16_t ra, uint32_t fn,
	uint8_t is_11bit, enum ph_burst_type burst_type);
int pcu_tx_time_ind(uint32_t fn);
int pcu_tx_pag_req(const uint8_t *identity_lv, uint8_t chan_needed);
int pcu_tx_pch_data_cnf(uint32_t fn, uint8_t *data, uint8_t len);

int pcu_sock_init(const char *path);
void pcu_sock_exit(void);

bool pcu_connected(void);

#endif /* _PCU_IF_H */
