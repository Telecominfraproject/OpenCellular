#include <stdio.h>
#include <stdint.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>
#include <osmocom/gsm/gsm_utils.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/rsl.h>

static struct gsm_bts *bts;
struct gsm_bts_trx *trx;

struct fn_sample {
	uint32_t fn;
	uint8_t ts;
	uint8_t ss;
	int rc;
};

#include "sysmobts_fr_samples.h"
#include "meas_testcases.h"


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

static void reset_lchan_meas(struct gsm_lchan *lchan)
{
	lchan->state = LCHAN_S_ACTIVE;
	memset(&lchan->meas, 0, sizeof(lchan->meas));
}

static void test_meas_compute(const struct meas_testcase *mtc)
{
	struct gsm_lchan *lchan;
	unsigned int i;
	unsigned int fn = 0;

	printf("\n\n");
	printf("===========================================================\n");
	printf("Measurement Compute Test: %s\n", mtc->name);

	lchan = &trx->ts[mtc->ts].lchan[0];
	lchan->ts->pchan = mtc->pchan;
	reset_lchan_meas(lchan);

	/* feed uplink measurements into the code */
	for (i = 0; i < mtc->ulm_count; i++) {
		lchan_new_ul_meas(lchan, (struct bts_ul_meas *) &mtc->ulm[i], fn);
		fn += 1;
	}

	/* compute the results */
	OSMO_ASSERT(lchan_meas_check_compute(lchan, mtc->final_fn) == mtc->res.success);
	if (!mtc->res.success) {
		OSMO_ASSERT(!(lchan->meas.flags & LC_UL_M_F_RES_VALID));
	} else {
		OSMO_ASSERT(lchan->meas.flags & (LC_UL_M_F_RES_VALID|LC_UL_M_F_OSMO_EXT_VALID));
		printf("number of measurements: %u\n",  mtc->ulm_count);
		printf("parameter                | actual | expected\n");
		printf("meas.ext.toa256_min      | %6d | %6d\n",
			lchan->meas.ext.toa256_min, mtc->res.toa256_min);
		printf("meas.ext.toa256_max      | %6d | %6d\n",
			lchan->meas.ext.toa256_max, mtc->res.toa256_max);
		printf("meas.ms_toa256           | %6d | %6d\n",
			lchan->meas.ms_toa256, mtc->res.toa256_mean);
		printf("meas.ext.toa256_std_dev  | %6u | %6u\n",
			lchan->meas.ext.toa256_std_dev, mtc->res.toa256_std_dev);
		printf("meas.ul_res.full.rx_lev  | %6u | %6u\n",
			lchan->meas.ul_res.full.rx_lev, mtc->res.rx_lev_full);
		printf("meas.ul_res.full.rx_qual | %6u | %6u\n",
			lchan->meas.ul_res.full.rx_qual, mtc->res.rx_qual_full);

		if ((lchan->meas.ext.toa256_min != mtc->res.toa256_min) ||
		    (lchan->meas.ext.toa256_max != mtc->res.toa256_max) ||
		    (lchan->meas.ms_toa256 != mtc->res.toa256_mean) ||
		    (lchan->meas.ext.toa256_std_dev != mtc->res.toa256_std_dev) ||
		    (lchan->meas.ul_res.full.rx_lev != mtc->res.rx_lev_full)) {
			fprintf(stderr, "%s: Unexpected measurement result!\n", mtc->name);
			OSMO_ASSERT(false);
		}
	}

}

static void test_is_meas_complete_single(struct gsm_lchan *lchan,
					 uint32_t fn_end, uint8_t intv_len)
{
	unsigned int i;
	unsigned int k;
	int rc;
	uint32_t offset;

	/* Walk through multiple measurement intervals and make sure that the
	 * interval end is detected only in the expected location */
	for (k = 0; k < 100; k++) {
		offset = intv_len * k;
		for (i = 0; i < intv_len; i++) {
			rc = is_meas_complete(lchan, i + offset);
			if (rc)
				OSMO_ASSERT(i + offset == fn_end + offset);
		}
	}
}

static void test_is_meas_complete(void)
{
	struct gsm_lchan *lchan;
	printf("\n\n");
	printf("===========================================================\n");
	printf("Testing is_meas_complete()\n");

	/* Test interval end detection on TCH/F TS0-TS7 */
	lchan = &trx->ts[0].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 12, 104);

	lchan = &trx->ts[1].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 25, 104);

	lchan = &trx->ts[2].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 38, 104);

	lchan = &trx->ts[3].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 51, 104);

	lchan = &trx->ts[4].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 64, 104);

	lchan = &trx->ts[5].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 77, 104);

	lchan = &trx->ts[6].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 90, 104);

	lchan = &trx->ts[7].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	test_is_meas_complete_single(lchan, 103, 104);

	/* Test interval end detection on TCH/H TS0-TS7 */
	lchan = &trx->ts[0].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 12, 104);

	lchan = &trx->ts[1].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 12, 104);

	lchan = &trx->ts[0].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 25, 104);

	lchan = &trx->ts[1].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 25, 104);

	lchan = &trx->ts[2].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 38, 104);

	lchan = &trx->ts[3].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 38, 104);

	lchan = &trx->ts[2].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 51, 104);

	lchan = &trx->ts[3].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 51, 104);

	lchan = &trx->ts[4].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 64, 104);

	lchan = &trx->ts[5].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 64, 104);

	lchan = &trx->ts[4].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 77, 104);

	lchan = &trx->ts[5].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 77, 104);

	lchan = &trx->ts[6].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 90, 104);

	lchan = &trx->ts[7].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 90, 104);

	lchan = &trx->ts[6].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 103, 104);

	lchan = &trx->ts[7].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_TCH_H;
	test_is_meas_complete_single(lchan, 103, 104);

	/* Test interval end detection on SDCCH/8 SS0-SS7 */
	lchan = &trx->ts[0].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 66, 102);

	lchan = &trx->ts[0].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 70, 102);

	lchan = &trx->ts[0].lchan[2];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 74, 102);

	lchan = &trx->ts[0].lchan[3];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 78, 102);

	lchan = &trx->ts[0].lchan[4];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 98, 102);

	lchan = &trx->ts[0].lchan[5];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 0, 102);

	lchan = &trx->ts[0].lchan[6];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 4, 102);

	lchan = &trx->ts[0].lchan[7];
	lchan->ts->pchan = GSM_PCHAN_SDCCH8_SACCH8C;
	test_is_meas_complete_single(lchan, 8, 102);

	/* Test interval end detection on SDCCH/4 SS0-SS3 */
	lchan = &trx->ts[0].lchan[0];
	lchan->ts->pchan = GSM_PCHAN_CCCH_SDCCH4;
	test_is_meas_complete_single(lchan, 88, 102);

	lchan = &trx->ts[0].lchan[1];
	lchan->ts->pchan = GSM_PCHAN_CCCH_SDCCH4;
	test_is_meas_complete_single(lchan, 92, 102);

	lchan = &trx->ts[0].lchan[2];
	lchan->ts->pchan = GSM_PCHAN_CCCH_SDCCH4;
	test_is_meas_complete_single(lchan, 6, 102);

	lchan = &trx->ts[0].lchan[3];
	lchan->ts->pchan = GSM_PCHAN_CCCH_SDCCH4;
	test_is_meas_complete_single(lchan, 10, 102);
}

/* This tests the robustness of lchan_meas_process_measurement(). This is the
 * function that is called from l1_sap.c each time a measurement indication is
 * received. The process must still go on when measurement indications (blocks)
 * are lost or otherwise spaced out. Even the complete absence of the
 * measurement indications from the SACCH which are used to detect the interval
 * end must not keep the interval from beeing processed. */
void test_lchan_meas_process_measurement(bool no_sacch, bool dropouts)
{
	struct gsm_lchan *lchan = &trx->ts[2].lchan[0];
	unsigned int i;
	unsigned int k = 0;
	unsigned int fn = 0;
	unsigned int fn104;
	struct bts_ul_meas ulm;
	int rc;

	printf("\n\n");
	printf("===========================================================\n");
	printf("Testing lchan_meas_process_measurement()\n");
	if (no_sacch)
		printf(" * SACCH blocks not generated.\n");
	if (dropouts)
		printf
		    (" * Simulate dropouts by leaving out every 4th measurement\n");

	ulm.ber10k = 0;
	ulm.ta_offs_256bits = 256;
	ulm.c_i = 0;
	ulm.is_sub = 0;
	ulm.inv_rssi = 90;

	lchan->ts->pchan = GSM_PCHAN_TCH_F;
	reset_lchan_meas(lchan);

	/* feed uplink measurements into the code */
	for (i = 0; i < 100; i++) {

		fn104 = fn % 104;
		ulm.is_sub = 0;

		if (fn104 >= 52 && fn104 <= 59) {
			ulm.is_sub = 1;
		}

		if (dropouts == false || i % 4) {
			if (ulm.is_sub == 1)
				printf("(now adding SUB measurement sample %u)\n", fn);
			rc = lchan_meas_process_measurement(lchan, &ulm, fn);
			OSMO_ASSERT(rc == 0);
		} else if (ulm.is_sub == 1)
			printf("(leaving out SUB measurement sample for frame number %u)\n", fn);
		else
			printf("(leaving out measurement sample for frame number %u)\n", fn);

		fn += 4;
		if (k == 2) {
			fn++;
			k = 0;
		} else
			k++;

		if (fn % 104 == 39 && no_sacch == false) {
			printf("(now adding SUB measurement sample for SACCH block at frame number %u)\n", fn);
			ulm.is_sub = 1;
			rc = lchan_meas_process_measurement(lchan, &ulm, fn - 1);
			OSMO_ASSERT(rc);
		} else if (fn % 104 == 39 && no_sacch == true)
			printf("(leaving out SUB measurement sample for SACCH block at frame number %u)\n", fn);
	}
}

static bool test_ts45008_83_is_sub_is_sacch(uint32_t fn)
{
	if (fn % 104 == 12)
		return true;
	if (fn % 104 == 25)
		return true;
	if (fn % 104 == 38)
		return true;
	if (fn % 104 == 51)
		return true;
	if (fn % 104 == 64)
		return true;
	if (fn % 104 == 77)
		return true;
	if (fn % 104 == 90)
		return true;
	if (fn % 104 == 103)
		return true;

	return false;
}

static bool test_ts45008_83_is_sub_is_sub(uint32_t fn, uint8_t ss)
{
	fn = fn % 104;

	if (fn >= 52 && fn <= 59)
		return true;

	if (ss == 0) {
		if (fn == 0)
			return true;
		if (fn == 2)
			return true;
		if (fn == 4)
			return true;
		if (fn == 6)
			return true;
		if (fn == 52)
			return true;
		if (fn == 54)
			return true;
		if (fn == 56)
			return true;
		if (fn == 58)
			return true;
	} else if (ss == 1) {
		if (fn == 14)
			return true;
		if (fn == 16)
			return true;
		if (fn == 18)
			return true;
		if (fn == 20)
			return true;
		if (fn == 66)
			return true;
		if (fn == 68)
			return true;
		if (fn == 70)
			return true;
		if (fn == 72)
			return true;
	} else
		OSMO_ASSERT(false);

	return false;
}

static void test_ts45008_83_is_sub_single(uint8_t ts, uint8_t ss, bool fr)
{
	struct gsm_lchan *lchan;
	bool rc;
	unsigned int i;

	lchan = &trx->ts[ts].lchan[ss];

	printf("Checking: ");

	if (fr) {
		printf("TCH/F");
		lchan->type = GSM_LCHAN_TCH_F;
		lchan->ts->pchan = GSM_PCHAN_TCH_F;
		lchan->tch_mode = GSM48_CMODE_SPEECH_V1;
	} else {
		printf("TCH/H");
		lchan->type = GSM_LCHAN_TCH_H;
		lchan->ts->pchan = GSM_PCHAN_TCH_H;
		lchan->tch_mode = GSM48_CMODE_SPEECH_V1;
	}

	printf(" TS=%u ", ts);
	printf("SS=%u", ss);

	/* Walk trough the first 100 intervals and check for unexpected
	 * results (false positive and false negative) */
	for (i = 0; i < 104 * 100; i++) {
		rc = ts45008_83_is_sub(lchan, i, false);
		if (rc) {
			if (!test_ts45008_83_is_sub_is_sacch(i)
			    && !test_ts45008_83_is_sub_is_sub(i, ss)) {
				printf("==> Unexpected SUB frame at fn=%u", i);
				OSMO_ASSERT(false);
			}
		} else {
			if (test_ts45008_83_is_sub_is_sacch(i)
			    && test_ts45008_83_is_sub_is_sub(i, ss)) {
				printf("==> Unexpected non-SUB frame at fn=%u",
				       i);
				OSMO_ASSERT(false);
			}
		}
	}
	printf("\n");
}

static void test_ts45008_83_is_sub(void)
{
	unsigned int i;

	printf("\n\n");
	printf("===========================================================\n");
	printf("Testing ts45008_83_is_sub()\n");

	for (i = 0; i < 7; i++)
		test_ts45008_83_is_sub_single(i, 0, true);
	for (i = 0; i < 7; i++)
		test_ts45008_83_is_sub_single(i, 0, false);
	for (i = 0; i < 7; i++)
		test_ts45008_83_is_sub_single(i, 1, false);
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

	test_meas_compute(&mtc1);
	test_meas_compute(&mtc2);
	test_meas_compute(&mtc3);
	test_meas_compute(&mtc4);
	test_meas_compute(&mtc5);
	test_meas_compute(&mtc_tch_f_complete);
	test_meas_compute(&mtc_tch_f_dtx_with_lost_subs);
	test_meas_compute(&mtc_tch_f_dtx);
	test_meas_compute(&mtc_tch_h_complete);
	test_meas_compute(&mtc_tch_h_dtx_with_lost_subs);
	test_meas_compute(&mtc_tch_h_dtx);
	test_meas_compute(&mtc_overrun);
	test_meas_compute(&mtc_sdcch4_complete);
	test_meas_compute(&mtc_sdcch8_complete);

	printf("\n");
	printf("***************************************************\n");
	printf("*** MEASUREMENT INTERVAL ENDING DETECTION TESTS ***\n");
	printf("***************************************************\n");

	test_is_meas_complete();
	test_lchan_meas_process_measurement(false, false);
	test_lchan_meas_process_measurement(true, false);
	test_lchan_meas_process_measurement(false, true);
	test_lchan_meas_process_measurement(true, true);
	test_ts45008_83_is_sub();

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
