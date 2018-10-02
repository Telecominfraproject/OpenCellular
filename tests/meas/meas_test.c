#include <stdio.h>
#include <stdint.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/measurement.h>

static struct gsm_bts *bts;
struct gsm_bts_trx *trx;

struct fn_sample {
	uint32_t fn;
	uint8_t ts;
	uint8_t ss;
	int rc;
};

#include "sysmobts_fr_samples.h"

void test_fn_sample(struct fn_sample *s, unsigned int len, uint8_t pchan, uint8_t tsmap)
{
	int rc;
	struct gsm_lchan *lchan;
	unsigned int i;
	unsigned int delta = 0;
	uint8_t tsmap_result = 0;
	uint32_t fn_prev = 0;
	struct gsm_time gsm_time;


	printf("\n\n");
	printf("===========================================================\n");

	for (i = 0; i < len; i++) {

		lchan = &trx->ts[s[i].ts].lchan[s[i].ss];
		trx->ts[s[i].ts].pchan = pchan;
		lchan->meas.num_ul_meas = 1;

		rc = lchan_meas_check_compute(lchan, s[i].fn);
		if (rc) {
			gsm_fn2gsmtime(&gsm_time, s[i].fn);
			fprintf(stdout, "Testing: ts[%i]->lchan[%i], fn=%u=>%s, fn%%104=%u, rc=%i, delta=%i\n", s[i].ts,
				s[i].ss, s[i].fn, osmo_dump_gsmtime(&gsm_time), s[i].fn % 104, rc, s[i].fn - fn_prev);
			fn_prev = s[i].fn;
			tsmap_result |= (1 << s[i].ts);
		} else
			delta++;

		/* If the test data set provides a return
		 * code, we check that as well */
		if (s[i].rc != -1)
			OSMO_ASSERT(s[i].rc == rc);
	}

	/* Make sure that we exactly trigger on the right frames
	 * timeslots must match exactlty to what we expect */
	OSMO_ASSERT(tsmap_result == tsmap);
}

int main(int argc, char **argv)
{
	void *tall_bts_ctx;

	tall_bts_ctx = talloc_named_const(NULL, 1, "OsmoBTS context");
	msgb_talloc_ctx_init(tall_bts_ctx, 0);

	osmo_init_logging2(tall_bts_ctx, &bts_log_info);
	osmo_stderr_target->categories[DMEAS].loglevel = LOGL_DEBUG;

	bts = gsm_bts_alloc(tall_bts_ctx, 0);
	if (!bts) {
		fprintf(stderr, "Failed to create BTS structure\n");
		exit(1);
	}
	trx = gsm_bts_trx_alloc(bts);
	if (!trx) {
		fprintf(stderr, "Failed to TRX structure\n");
		exit(1);
	}

	if (bts_init(bts) < 0) {
		fprintf(stderr, "unable to to open bts\n");
		exit(1);
	}

	printf("\n");
	printf("***********************\n");
	printf("*** FULL RATE TESTS ***\n");
	printf("***********************\n");

	/* Test full rate */
	test_fn_sample(test_fn_tch_f_ts_2_3, ARRAY_SIZE(test_fn_tch_f_ts_2_3), GSM_PCHAN_TCH_F, (1 << 2) | (1 << 3));
	test_fn_sample(test_fn_tch_f_ts_4_5, ARRAY_SIZE(test_fn_tch_f_ts_4_5), GSM_PCHAN_TCH_F, (1 << 4) | (1 << 5));
	test_fn_sample(test_fn_tch_f_ts_6_7, ARRAY_SIZE(test_fn_tch_f_ts_6_7), GSM_PCHAN_TCH_F, (1 << 6) | (1 << 7));

	printf("\n");
	printf("***********************\n");
	printf("*** HALF RATE TESTS ***\n");
	printf("***********************\n");

	/* Test half rate */
	test_fn_sample(test_fn_tch_h_ts_2_ss0_ss1, ARRAY_SIZE(test_fn_tch_h_ts_2_ss0_ss1), GSM_PCHAN_TCH_H, (1 << 2));
	test_fn_sample(test_fn_tch_h_ts_3_ss0_ss1, ARRAY_SIZE(test_fn_tch_h_ts_3_ss0_ss1), GSM_PCHAN_TCH_H, (1 << 3));
	test_fn_sample(test_fn_tch_h_ts_4_ss0_ss1, ARRAY_SIZE(test_fn_tch_h_ts_4_ss0_ss1), GSM_PCHAN_TCH_H, (1 << 4));
	test_fn_sample(test_fn_tch_h_ts_5_ss0_ss1, ARRAY_SIZE(test_fn_tch_h_ts_5_ss0_ss1), GSM_PCHAN_TCH_H, (1 << 5));
	test_fn_sample(test_fn_tch_h_ts_6_ss0_ss1, ARRAY_SIZE(test_fn_tch_h_ts_6_ss0_ss1), GSM_PCHAN_TCH_H, (1 << 6));
	test_fn_sample(test_fn_tch_h_ts_7_ss0_ss1, ARRAY_SIZE(test_fn_tch_h_ts_7_ss0_ss1), GSM_PCHAN_TCH_H, (1 << 7));

	printf("Success\n");

	return 0;
}

/* Stubs */
void bts_model_abis_close(struct gsm_bts *bts)
{
}

int bts_model_oml_estab(struct gsm_bts *bts)
{
	return 0;
}

int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap)
{
	return 0;
}

int bts_model_check_oml(struct gsm_bts *bts, uint8_t msg_type, struct tlv_parsed *old_attr, struct tlv_parsed *new_attr,
			void *obj)
{
	return 0;
}

int bts_model_apply_oml(struct gsm_bts *bts, struct msgb *msg, struct tlv_parsed *new_attr, int obj_kind, void *obj)
{
	return 0;
}

int bts_model_opstart(struct gsm_bts *bts, struct gsm_abis_mo *mo, void *obj)
{
	return 0;
}

int bts_model_chg_adm_state(struct gsm_bts *bts, struct gsm_abis_mo *mo, void *obj, uint8_t adm_state)
{
	return 0;
}

int bts_model_init(struct gsm_bts *bts)
{
	return 0;
}

int bts_model_trx_deact_rf(struct gsm_bts_trx *trx)
{
	return 0;
}

int bts_model_trx_close(struct gsm_bts_trx *trx)
{
	return 0;
}

void trx_get_hlayer1(void)
{
}

int bts_model_adjst_ms_pwr(struct gsm_lchan *lchan)
{
	return 0;
}

int bts_model_ts_disconnect(struct gsm_bts_trx_ts *ts)
{
	return 0;
}

int bts_model_ts_connect(struct gsm_bts_trx_ts *ts, enum gsm_phys_chan_config as_pchan)
{
	return 0;
}

int bts_model_lchan_deactivate(struct gsm_lchan *lchan)
{
	return 0;
}

int bts_model_lchan_deactivate_sacch(struct gsm_lchan *lchan)
{
	return 0;
}
