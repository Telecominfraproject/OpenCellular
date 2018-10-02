#ifndef _BTS_H
#define _BTS_H

#include <osmocom/core/rate_ctr.h>
#include <osmo-bts/gsm_data.h>

enum bts_global_status {
	BTS_STATUS_RF_ACTIVE,
	BTS_STATUS_RF_MUTE,
	BTS_STATUS_LAST,
};

enum {
	BTS_CTR_PAGING_RCVD,
	BTS_CTR_PAGING_DROP,
	BTS_CTR_PAGING_SENT,
	BTS_CTR_RACH_RCVD,
	BTS_CTR_RACH_DROP,
	BTS_CTR_RACH_HO,
	BTS_CTR_RACH_CS,
	BTS_CTR_RACH_PS,
	BTS_CTR_AGCH_RCVD,
	BTS_CTR_AGCH_SENT,
	BTS_CTR_AGCH_DELETED,
};

extern void *tall_bts_ctx;

int bts_init(struct gsm_bts *bts);
void bts_shutdown(struct gsm_bts *bts, const char *reason);

struct gsm_bts *create_bts(uint8_t num_trx, char *id);
int create_ms(struct gsm_bts_trx *trx, int maskc, uint8_t *maskv_tx,
	uint8_t *maskv_rx);
void destroy_bts(struct gsm_bts *bts);
int work_bts(struct gsm_bts *bts);
int bts_link_estab(struct gsm_bts *bts);
int trx_link_estab(struct gsm_bts_trx *trx);
int trx_set_available(struct gsm_bts_trx *trx, int avail);
void bts_new_si(void *arg);
void bts_setup_slot(struct gsm_bts_trx_ts *slot, uint8_t comb);

int bts_agch_enqueue(struct gsm_bts *bts, struct msgb *msg);
struct msgb *bts_agch_dequeue(struct gsm_bts *bts);
int bts_agch_max_queue_length(int T, int bcch_conf);
int bts_ccch_copy_msg(struct gsm_bts *bts, uint8_t *out_buf, struct gsm_time *gt,
		      int is_ag_res);

uint8_t *bts_sysinfo_get(struct gsm_bts *bts, const struct gsm_time *g_time);
uint8_t *lchan_sacch_get(struct gsm_lchan *lchan);
int lchan_init_lapdm(struct gsm_lchan *lchan);

void load_timer_start(struct gsm_bts *bts);
uint8_t num_agch(struct gsm_bts_trx *trx, const char * arg);
void bts_update_status(enum bts_global_status which, int on);

int trx_ms_pwr_ctrl_is_osmo(struct gsm_bts_trx *trx);

struct gsm_time *get_time(struct gsm_bts *bts);

int bts_main(int argc, char **argv);

int bts_supports_cm(struct gsm_bts *bts, enum gsm_phys_chan_config pchan,
		    enum gsm48_chan_mode cm);

#endif /* _BTS_H */

