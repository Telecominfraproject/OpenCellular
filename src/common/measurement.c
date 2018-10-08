
#include <stdint.h>
#include <errno.h>

#include <osmocom/gsm/gsm_utils.h>
#include <osmocom/core/utils.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/scheduler.h>
#include <osmo-bts/rsl.h>

/* Tables as per TS 45.008 Section 8.3 */
static const uint8_t ts45008_83_tch_f[] = { 52, 53, 54, 55, 56, 57, 58, 59 };
static const uint8_t ts45008_83_tch_hs0[] = { 0, 2, 4, 6, 52, 54, 56, 58 };
static const uint8_t ts45008_83_tch_hs1[] = { 14, 16, 18, 20, 66, 68, 70, 72 };

/* In cases where we less measurements than we expect we must assume that we
 * just did not receive the block because it was lost due to bad channel
 * conditions. We set up a dummy measurement result here that reflects the
 * worst possible result. In our* calculation we will use this dummy to replace
 * the missing measurements */
#define MEASUREMENT_DUMMY_BER 10000 /* 100% BER */
#define MEASUREMENT_DUMMY_IRSSI 109 /* noise floor in -dBm */
static const struct bts_ul_meas measurement_dummy = (struct bts_ul_meas) {
	.ber10k = MEASUREMENT_DUMMY_BER,
	.ta_offs_256bits = 0,
	.c_i = 0,
	.is_sub = 0,
	.inv_rssi = MEASUREMENT_DUMMY_IRSSI
};

/* find out if an array contains a given key as element */
#define ARRAY_CONTAINS(arr, val) array_contains(arr, ARRAY_SIZE(arr), val)
static bool array_contains(const uint8_t *arr, unsigned int len, uint8_t val) {
	int i;
	for (i = 0; i < len; i++) {
		if (arr[i] == val)
			return true;
	}
	return false;
}

/* Decide if a given frame number is part of the "-SUB" measurements (true) or not (false)
 * (this function is only used internally, it is public to call it from unit-tests) */
bool ts45008_83_is_sub(struct gsm_lchan *lchan, uint32_t fn, bool is_amr_sid_update)
{
	uint32_t fn104 = fn % 104;

	/* See TS 45.008 Sections 8.3 and 8.4 for a detailed descriptions of the rules
	 * implemented here. We only implement the logic for Voice, not CSD */

	switch (lchan->type) {
	case GSM_LCHAN_TCH_F:
		switch (lchan->tch_mode) {
		case GSM48_CMODE_SIGN:
		case GSM48_CMODE_SPEECH_V1:
		case GSM48_CMODE_SPEECH_EFR:
			if (trx_sched_is_sacch_fn(lchan->ts, fn, true))
				return true;
			if (ARRAY_CONTAINS(ts45008_83_tch_f, fn104))
				return true;
			break;
		case GSM48_CMODE_SPEECH_AMR:
			if (trx_sched_is_sacch_fn(lchan->ts, fn, true))
				return true;
			if (is_amr_sid_update)
				return true;
			break;
		default:
			LOGPFN(DMEAS, LOGL_ERROR, fn, "%s: Unsupported lchan->tch_mode %u\n",
				gsm_lchan_name(lchan), lchan->tch_mode);
			break;
		}
		break;
	case GSM_LCHAN_TCH_H:
		switch (lchan->tch_mode) {
		case GSM48_CMODE_SPEECH_V1:
			if (trx_sched_is_sacch_fn(lchan->ts, fn, true))
				return true;
			switch (lchan->nr) {
			case 0:
				if (ARRAY_CONTAINS(ts45008_83_tch_hs0, fn104))
					return true;
				break;
			case 1:
				if (ARRAY_CONTAINS(ts45008_83_tch_hs1, fn104))
					return true;
				break;
			default:
				OSMO_ASSERT(0);
			}
			break;
		case GSM48_CMODE_SPEECH_AMR:
			if (trx_sched_is_sacch_fn(lchan->ts, fn, true))
				return true;
			if (is_amr_sid_update)
				return true;
			break;
		case GSM48_CMODE_SIGN:
			/* No DTX allowed; SUB=FULL, therefore measurements at all frame numbers are
			 * SUB */
			return true;
		default:
			LOGPFN(DMEAS, LOGL_ERROR, fn, "%s: Unsupported lchan->tch_mode %u\n",
				gsm_lchan_name(lchan), lchan->tch_mode);
			break;
		}
		break;
	case GSM_LCHAN_SDCCH:
		/* No DTX allowed; SUB=FULL, therefore measurements at all frame numbers are SUB */
		return true;
	default:
		break;
	}
	return false;
}

/* Measurement reporting period and mapping of SACCH message block for TCHF
 * and TCHH chan As per in 3GPP TS 45.008, section 8.4.1.
 *
 *             Timeslot number (TN)        TDMA frame number (FN) modulo 104
 *             Half rate,    Half rate,     Reporting    SACCH
 * Full Rate   subch.0       subch.1        period       Message block
 * 0           0 and 1                      0 to 103     12,  38,  64,  90
 * 1                         0 and 1        13 to 12     25,  51,  77,  103
 * 2           2 and 3                      26 to 25     38,  64,  90,  12
 * 3                         2 and 3        39 to 38     51,  77,  103, 25
 * 4           4 and 5                      52 to 51     64,  90,  12,  38
 * 5                         4 and 5        65 to 64     77,  103, 25,  51
 * 6           6 and 7                      78 to 77     90,  12,  38,  64
 * 7                         6 and 7        91 to 90     103, 25,  51,  77
 *
 * Note: The array index of the following three lookup tables refes to a
 *       timeslot number. */

static const uint8_t tchf_meas_rep_fn104_by_ts[] = {
	[0] =	90,
	[1] =	103,
	[2] =	12,
	[3] =	25,
	[4] =	38,
	[5] =	51,
	[6] =	64,
	[7] =	77,
};
static const uint8_t tchh0_meas_rep_fn104_by_ts[] = {
	[0] =	90,
	[1] =	90,
	[2] =	12,
	[3] =	12,
	[4] =	38,
	[5] =	38,
	[6] =	64,
	[7] =	64,
};
static const uint8_t tchh1_meas_rep_fn104_by_ts[] = {
	[0] =	103,
	[1] =	103,
	[2] =	25,
	[3] =	25,
	[4] =	51,
	[5] =	51,
	[6] =	77,
	[7] =	77,
};

/* Measurement reporting period for SDCCH8 and SDCCH4 chan
 * As per in 3GPP TS 45.008, section 8.4.2.
 *
 * Logical Chan		TDMA frame number
 *			(FN) modulo 102
 *
 * SDCCH/8		12 to 11
 * SDCCH/4		37 to 36
 *
 *
 * Note: The array index of the following three lookup tables refes to a
 *       subslot number. */

/* FN of the first burst whose block completes before reaching fn%102=11 */
static const uint8_t sdcch8_meas_rep_fn102_by_ss[] = {
	[0] = 66,	/* 15(SDCCH), 47(SACCH), 66(SDCCH) */
	[1] = 70,	/* 19(SDCCH), 51(SACCH), 70(SDCCH) */
	[2] = 74,	/* 23(SDCCH), 55(SACCH), 74(SDCCH) */
	[3] = 78,	/* 27(SDCCH), 59(SACCH), 78(SDCCH) */
	[4] = 98,	/* 31(SDCCH), 98(SACCH), 82(SDCCH) */
	[5] = 0,	/* 35(SDCCH),  0(SACCH), 86(SDCCH) */
	[6] = 4,	/* 39(SDCCH),  4(SACCH), 90(SDCCH) */
	[7] = 8,	/* 43(SDCCH),  8(SACCH), 94(SDCCH) */
};

/* FN of the first burst whose block completes before reaching fn%102=37 */
static const uint8_t sdcch4_meas_rep_fn102_by_ss[] = {
	[0] = 88,	/* 37(SDCCH), 57(SACCH), 88(SDCCH) */
	[1] = 92,	/* 41(SDCCH), 61(SACCH), 92(SDCCH) */
	[2] = 6,	/*  6(SACCH), 47(SDCCH), 98(SDCCH) */
	[3] = 10	/* 10(SACCH),  0(SDCCH), 51(SDCCH) */
};

/* Note: The reporting of the measurement results is done via the SACCH channel.
 * The measurement interval is not aligned with the interval in which the
 * SACCH is transmitted. When we receive the measurement indication with the
 * SACCH block, the corresponding measurement interval will already have ended
 * and we will get the results late, but on spot with the beginning of the
 * next measurement interval.
 *
 * For example: We get a measurement indication on FN%104=38 in TS=2. Then we
 * will have to look at 3GPP TS 45.008, section 8.4.1 (or 3GPP TS 05.02 Clause 7
 * Table 1 of 9) what value we need to feed into the lookup tables in order to
 * detect the measurement period ending. In this example the "real" ending
 * was on FN%104=12. This is the value we have to look for in
 * tchf_meas_rep_fn104_by_ts to know that a measurement period has just ended. */

/* See also 3GPP TS 05.02 Clause 7 Table 1 of 9:
 * Mapping of logical channels onto physical channels (see subclauses 6.3, 6.4, 6.5) */
static uint8_t translate_tch_meas_rep_fn104(uint8_t fn_mod)
{
	switch (fn_mod) {
	case 25:
		return 103;
	case 38:
		return 12;
	case 51:
		return 25;
	case 64:
		return 38;
	case 77:
		return 51;
	case 90:
		return 64;
	case 103:
		return 77;
	case 12:
		return 90;
	}

	/* Invalid / not of interest */
	return 0;
}

/* determine if a measurement period ends at the given frame number
 * (this function is only used internally, it is public to call it from
 * unit-tests) */
int is_meas_complete(struct gsm_lchan *lchan, uint32_t fn)
{
	unsigned int fn_mod = -1;
	const uint8_t *tbl;
	int rc = 0;
	enum gsm_phys_chan_config pchan = ts_pchan(lchan->ts);

	if (lchan->ts->nr >= 8)
		return -EINVAL;
	if (pchan >= _GSM_PCHAN_MAX)
		return -EINVAL;

	switch (pchan) {
	case GSM_PCHAN_TCH_F:
		fn_mod = translate_tch_meas_rep_fn104(fn % 104);
		if (tchf_meas_rep_fn104_by_ts[lchan->ts->nr] == fn_mod)
			rc = 1;
		break;
	case GSM_PCHAN_TCH_H:
		fn_mod = translate_tch_meas_rep_fn104(fn % 104);
		if (lchan->nr == 0)
			tbl = tchh0_meas_rep_fn104_by_ts;
		else
			tbl = tchh1_meas_rep_fn104_by_ts;
		if (tbl[lchan->ts->nr] == fn_mod)
			rc = 1;
		break;
	case GSM_PCHAN_SDCCH8_SACCH8C:
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
		fn_mod = fn % 102;
		if (sdcch8_meas_rep_fn102_by_ss[lchan->nr] == fn_mod)
			rc = 1;
		break;
	case GSM_PCHAN_CCCH_SDCCH4:
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		fn_mod = fn % 102;
		if (sdcch4_meas_rep_fn102_by_ss[lchan->nr] == fn_mod)
			rc = 1;
		break;
	default:
		rc = 0;
		break;
	}

	if (rc == 1) {
		DEBUGP(DMEAS,
		       "%s meas period end fn:%u, fn_mod:%i, status:%d, pchan:%s\n",
		       gsm_lchan_name(lchan), fn, fn_mod, rc, gsm_pchan_name(pchan));
	}

	return rc;
}

/* determine the measurement interval modulus by a given lchan */
static uint8_t modulus_by_lchan(struct gsm_lchan *lchan)
{
	enum gsm_phys_chan_config pchan = ts_pchan(lchan->ts);

	switch (pchan) {
	case GSM_PCHAN_TCH_F:
	case GSM_PCHAN_TCH_H:
		return 104;
		break;
	case GSM_PCHAN_SDCCH8_SACCH8C:
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
	case GSM_PCHAN_CCCH_SDCCH4:
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		return 102;
		break;
	default:
		/* Invalid */
		return 1;
		break;
	}
}

/* receive a L1 uplink measurement from L1 (this function is only used
 * internally, it is public to call it from unit-tests)  */
int lchan_new_ul_meas(struct gsm_lchan *lchan, struct bts_ul_meas *ulm, uint32_t fn)
{
	uint32_t fn_mod = fn % modulus_by_lchan(lchan);

	if (lchan->state != LCHAN_S_ACTIVE) {
		LOGPFN(DMEAS, LOGL_NOTICE, fn,
		       "%s measurement during state: %s, num_ul_meas=%d, fn_mod=%u\n",
		       gsm_lchan_name(lchan), gsm_lchans_name(lchan->state),
		       lchan->meas.num_ul_meas, fn_mod);
	}

	if (lchan->meas.num_ul_meas >= ARRAY_SIZE(lchan->meas.uplink)) {
		LOGPFN(DMEAS, LOGL_NOTICE, fn,
		       "%s no space for uplink measurement, num_ul_meas=%d, fn_mod=%u\n",
		       gsm_lchan_name(lchan), lchan->meas.num_ul_meas, fn_mod);
		return -ENOSPC;
	}

	/* We expect the lower layers to mark AMR SID_UPDATE frames already as such.
	 * In this function, we only deal with the comon logic as per the TS 45.008 tables */
	if (!ulm->is_sub)
		ulm->is_sub = ts45008_83_is_sub(lchan, fn, false);

	DEBUGPFN(DMEAS, fn, "%s adding measurement (is_sub=%u), num_ul_meas=%d, fn_mod=%u\n",
		 gsm_lchan_name(lchan), ulm->is_sub, lchan->meas.num_ul_meas, fn_mod);

	memcpy(&lchan->meas.uplink[lchan->meas.num_ul_meas++], ulm,
		sizeof(*ulm));

	lchan->meas.last_fn = fn;

	return 0;
}

/* input: BER in steps of .01%, i.e. percent/100 */
static uint8_t ber10k_to_rxqual(uint32_t ber10k)
{
	/* Eight levels of Rx quality are defined and are mapped to the
	 * equivalent BER before channel decoding, as per in 3GPP TS 45.008,
	 * secton 8.2.4.
	 *
	 * RxQual:				BER Range:
	 * RXQUAL_0	     BER <  0,2 %       Assumed value = 0,14 %
	 * RXQUAL_1  0,2 % < BER <  0,4 %	Assumed value = 0,28 %
	 * RXQUAL_2  0,4 % < BER <  0,8 %	Assumed value = 0,57 %
	 * RXQUAL_3  0,8 % < BER <  1,6 %	Assumed value = 1,13 %
	 * RXQUAL_4  1,6 % < BER <  3,2 %	Assumed value = 2,26 %
	 * RXQUAL_5  3,2 % < BER <  6,4 %	Assumed value = 4,53 %
	 * RXQUAL_6  6,4 % < BER < 12,8 %	Assumed value = 9,05 %
	 * RXQUAL_7 12,8 % < BER		Assumed value = 18,10 % */

	if (ber10k < 20)
		return 0;
	if (ber10k < 40)
		return 1;
	if (ber10k < 80)
		return 2;
	if (ber10k < 160)
		return 3;
	if (ber10k < 320)
		return 4;
	if (ber10k < 640)
		return 5;
	if (ber10k < 1280)
		return 6;
	return 7;
}

/* Get the number of measurements that we expect for a specific lchan.
 * (This is a static number that is defined by the specific slot layout of
 * the channel) */
static unsigned int lchan_meas_num_expected(const struct gsm_lchan *lchan)
{
	enum gsm_phys_chan_config pchan = ts_pchan(lchan->ts);

	switch (pchan) {
	case GSM_PCHAN_TCH_F:
		/* 24 for TCH + 1 for SACCH */
		return 25;
	case GSM_PCHAN_TCH_H:
		/* 24 half-blocks for TCH + 1 for SACCH */
		return 25;
	case GSM_PCHAN_SDCCH8_SACCH8C:
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
		/* 2 for SDCCH + 1 for SACCH */
		return 3;
	case GSM_PCHAN_CCCH_SDCCH4:
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		/* 2 for SDCCH + 1 for SACCH */
		return 3;
	default:
		return lchan->meas.num_ul_meas;
	}
}

/* In DTX a subset of blocks must always be transmitted
 * See also: GSM 05.08, chapter 8.3 Aspects of discontinuous transmission (DTX) */
static unsigned int lchan_meas_sub_num_expected(const struct gsm_lchan *lchan)
{
	enum gsm_phys_chan_config pchan = ts_pchan(lchan->ts);

	/* AMR is using a more elaborated model with a dymanic amount of
	 * DTX blocks so this function is not applicable to determine the
	 * amount of expected SUB Measurements when AMR is used */
	OSMO_ASSERT(lchan->tch_mode != GSM48_CMODE_SPEECH_AMR)

	switch (pchan) {
	case GSM_PCHAN_TCH_F:
		/* 1 block SDCCH, 2 blocks TCH */
		return 3;
	case GSM_PCHAN_TCH_H:
		/* 1 block SDCCH, 4 half-blocks TCH */
		return 5;
	case GSM_PCHAN_SDCCH8_SACCH8C:
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
		/* no DTX here, all blocks must be present! */
		return 3;
	case GSM_PCHAN_CCCH_SDCCH4:
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		/* no DTX here, all blocks must be present! */
		return 3;
	default:
		return 0;
	}
}

/* if we clip the TOA value to 12 bits, i.e. toa256=3200,
 *  -> the maximum deviation can be 2*3200 = 6400
 *  -> the maximum squared deviation can be 6400^2 = 40960000
 *  -> the maximum sum of squared deviations can be 104*40960000 = 4259840000
 *     and hence fit into uint32_t
 *  -> once the value is divided by 104, it's again below 40960000
 *     leaving 6 MSBs of freedom, i.e. we could extend by 64, resulting in 2621440000
 *  -> as a result, the standard deviation could be communicated with up to six bits
 *     of fractional fixed-point number.
 */

/* compute Osmocom extended measurements for the given lchan */
static void lchan_meas_compute_extended(struct gsm_lchan *lchan)
{
	unsigned int num_ul_meas;
	unsigned int num_ul_meas_excess = 0;
        unsigned int num_ul_meas_expect;

	/* we assume that lchan_meas_check_compute() has already computed the mean value
	 * and we can compute the min/max/variance/stddev from this */
	int i;

	/* each measurement is an int32_t, so the squared difference value must fit in 32bits */
	/* the sum of the squared values (each up to 32bit) can very easily exceed 32 bits */
	u_int64_t sq_diff_sum = 0;

	/* In case we do not have any measurement values collected there is no
	 * computation possible. We just skip the whole computation here, the
	 * lchan->meas.flags will not get the LC_UL_M_F_OSMO_EXT_VALID flag set
	 * so no extended measurement results will be reported back via RSL.
	 * this is ok, since we have nothing to report anyway and apart of that
	 * we also just lost the signal (otherwise we would have at least some
	 * measurements). */
	if (!lchan->meas.num_ul_meas)
		return;

	/* initialize min/max values with their counterpart */
	lchan->meas.ext.toa256_min = INT16_MAX;
	lchan->meas.ext.toa256_max = INT16_MIN;

	/* Determine the number of measurement values we need to take into the
	 * computation. In this case we only compute over the measurements we
	 * have indeed received. Since this computation is about timing
	 * information it does not make sense to approach missing measurement
	 * samples the TOA with 0. This would bend the average towards 0. What
	 * counts is the average TOA of the properly received blocks so that
	 * the TA logic can make a proper decision. */
        num_ul_meas_expect = lchan_meas_num_expected(lchan);
	if (lchan->meas.num_ul_meas > num_ul_meas_expect) {
		num_ul_meas = num_ul_meas_expect;
		num_ul_meas_excess = lchan->meas.num_ul_meas - num_ul_meas_expect;
	}
	else
		num_ul_meas = lchan->meas.num_ul_meas;

	/* all computations are done on the relative arrival time of the burst, relative to the
	 * beginning of its slot. This is of course excluding the TA value that the MS has already
	 * compensated/pre-empted its transmission */

	/* step 1: compute the sum of the squared difference of each value to mean */
	for (i = 0; i < num_ul_meas; i++) {
		const struct bts_ul_meas *m;

		OSMO_ASSERT(i < lchan->meas.num_ul_meas);
		m = &lchan->meas.uplink[i+num_ul_meas_excess];

		int32_t diff = (int32_t)m->ta_offs_256bits - (int32_t)lchan->meas.ms_toa256;
		/* diff can now be any value of +65535 to -65535, so we can safely square it,
		 * but only in unsigned math.  As squaring looses the sign, we can simply drop
		 * it before squaring, too. */
		uint32_t diff_abs = labs(diff);
		uint32_t diff_squared = diff_abs * diff_abs;
		sq_diff_sum += diff_squared;

		/* also use this loop iteration to compute min/max values */
		if (m->ta_offs_256bits > lchan->meas.ext.toa256_max)
			lchan->meas.ext.toa256_max = m->ta_offs_256bits;
		if (m->ta_offs_256bits < lchan->meas.ext.toa256_min)
			lchan->meas.ext.toa256_min = m->ta_offs_256bits;
	}
	/* step 2: compute the variance (mean of sum of squared differences) */
	sq_diff_sum = sq_diff_sum / num_ul_meas;
	/* as the individual summed values can each not exceed 2^32, and we're
	 * dividing by the number of summands, the resulting value can also not exceed 2^32 */
	OSMO_ASSERT(sq_diff_sum <= UINT32_MAX);
	/* step 3: compute the standard deviation from the variance */
	lchan->meas.ext.toa256_std_dev = osmo_isqrt32(sq_diff_sum);
	lchan->meas.flags |= LC_UL_M_F_OSMO_EXT_VALID;
}

int lchan_meas_check_compute(struct gsm_lchan *lchan, uint32_t fn)
{
	struct gsm_meas_rep_unidir *mru;
	uint32_t ber_full_sum = 0;
	uint32_t irssi_full_sum = 0;
	uint32_t ber_sub_sum = 0;
	uint32_t irssi_sub_sum = 0;
	int32_t ta256b_sum = 0;
	unsigned int num_meas_sub = 0;
	unsigned int num_meas_sub_actual = 0;
	unsigned int num_meas_sub_subst = 0;
	unsigned int num_meas_sub_expect;
	unsigned int num_ul_meas;
	unsigned int num_ul_meas_actual = 0;
	unsigned int num_ul_meas_subst = 0;
	unsigned int num_ul_meas_expect;
	unsigned int num_ul_meas_excess = 0;
	int i;

	/* if measurement period is not complete, abort */
	if (!is_meas_complete(lchan, fn))
		return 0;

	LOGP(DMEAS, LOGL_DEBUG, "%s Calculating measurement results for physical channel:%s\n",
	     gsm_lchan_name(lchan), gsm_pchan_name(ts_pchan(lchan->ts)));

	/* Note: Some phys will send no measurement indication at all
	 * when a block is lost. Also in DTX mode blocks are left out
	 * intentionally to save energy. It is not necessarly an error
	 * when we get less measurements as we expect. */
	num_ul_meas_expect = lchan_meas_num_expected(lchan);

	if (lchan->tch_mode != GSM48_CMODE_SPEECH_AMR)
		num_meas_sub_expect = lchan_meas_sub_num_expected(lchan);
	else {
		/* FIXME: the amount of SUB Measurements is a dynamic parameter
		 * in AMR and can not be determined by using a lookup table.
		 * See also: OS#2978 */
		num_meas_sub_expect = 0;
	}

	if (lchan->meas.num_ul_meas > num_ul_meas_expect)
		num_ul_meas_excess = lchan->meas.num_ul_meas - num_ul_meas_expect;
	num_ul_meas = num_ul_meas_expect;

	LOGP(DMEAS, LOGL_DEBUG, "%s received %u UL measurements, expected %u\n", gsm_lchan_name(lchan),
	     lchan->meas.num_ul_meas, num_ul_meas_expect);
	if (num_ul_meas_excess)
		LOGP(DMEAS, LOGL_DEBUG, "%s received %u excess UL measurements\n", gsm_lchan_name(lchan),
		     num_ul_meas_excess);

	/* Measurement computation step 1: add up */
	for (i = 0; i < num_ul_meas; i++) {
		const struct bts_ul_meas *m;
		bool is_sub = false;

		/* Note: We will always compute over a full measurement,
		 * interval even when not enough measurement samples are in
		 * the buffer. As soon as we run out of measurement values
		 * we continue the calculation using dummy values. This works
		 * well for the BER, since there we can safely assume 100%
		 * since a missing measurement means that the data (block)
		 * is lost as well (some phys do not give us measurement
		 * reports for lost blocks or blocks that are spaced out for
		 * DTX). However, for RSSI and TA this does not work since
		 * there we would distort the calculation if we would replace
		 * them with a made up number. This means for those values we
		 * only compute over the data we have actually received. */

		if (i < lchan->meas.num_ul_meas) {
			m = &lchan->meas.uplink[i + num_ul_meas_excess];
			if (m->is_sub) {
				irssi_sub_sum += m->inv_rssi;
				num_meas_sub_actual++;
				is_sub = true;
			}
			irssi_full_sum += m->inv_rssi;
			ta256b_sum += m->ta_offs_256bits;

			num_ul_meas_actual++;
		} else {
			m = &measurement_dummy;
			if (num_ul_meas_expect - i <= num_meas_sub_expect - num_meas_sub) {
				num_meas_sub_subst++;
				is_sub = true;
			}

			num_ul_meas_subst++;
		}

		ber_full_sum += m->ber10k;
		if (is_sub) {
			num_meas_sub++;
			ber_sub_sum += m->ber10k;
		}
	}

	LOGP(DMEAS, LOGL_DEBUG, "%s received UL measurements contain %u SUB measurements, expected %u\n",
	     gsm_lchan_name(lchan), num_meas_sub_actual, num_meas_sub_expect);
	LOGP(DMEAS, LOGL_DEBUG, "%s replaced %u measurements with dummy values, from which %u were SUB measurements\n",
	     gsm_lchan_name(lchan), num_ul_meas_subst, num_meas_sub_subst);

	if (num_meas_sub != num_meas_sub_expect) {
		LOGP(DMEAS, LOGL_ERROR, "%s Incorrect number of SUB measurements detected!\n", gsm_lchan_name(lchan));
		/* Normally the logic above should make sure that there is
		 * always the exact amount of SUB measurements taken into
		 * account. If not then the logic that decides tags the received
		 * measurements as is_sub works incorrectly. Since the logic
		 * above only adds missing measurements during the calculation
		 * it can not remove excess SUB measurements or add missing SUB
		 * measurements when there is no more room in the interval. */
	}

	/* Measurement computation step 2: divide */
	ber_full_sum = ber_full_sum / num_ul_meas;

	if (!irssi_full_sum)
		ber_full_sum = MEASUREMENT_DUMMY_IRSSI;
	else
		irssi_full_sum = irssi_full_sum / num_ul_meas_actual;

	if (!num_ul_meas_actual)
		ta256b_sum = lchan->meas.ms_toa256;
	else
		ta256b_sum = ta256b_sum / num_ul_meas_actual;

	if (!num_meas_sub)
		ber_sub_sum = MEASUREMENT_DUMMY_BER;
	else
		ber_sub_sum = ber_sub_sum / num_meas_sub;

	if (!num_meas_sub_actual)
		irssi_sub_sum = MEASUREMENT_DUMMY_IRSSI;
	else
		irssi_sub_sum = irssi_sub_sum / num_meas_sub_actual;

	LOGP(DMEAS, LOGL_INFO, "%s Computed TA256(% 4d) BER-FULL(%2u.%02u%%), RSSI-FULL(-%3udBm), "
	     "BER-SUB(%2u.%02u%%), RSSI-SUB(-%3udBm)\n", gsm_lchan_name(lchan),
	     ta256b_sum, ber_full_sum / 100,
	     ber_full_sum % 100, irssi_full_sum, ber_sub_sum / 100, ber_sub_sum % 100, irssi_sub_sum);

	/* store results */
	mru = &lchan->meas.ul_res;
	mru->full.rx_lev = dbm2rxlev((int)irssi_full_sum * -1);
	mru->sub.rx_lev = dbm2rxlev((int)irssi_sub_sum * -1);
	mru->full.rx_qual = ber10k_to_rxqual(ber_full_sum);
	mru->sub.rx_qual = ber10k_to_rxqual(ber_sub_sum);
	lchan->meas.ms_toa256 = ta256b_sum;

	LOGP(DMEAS, LOGL_INFO, "%s UL MEAS RXLEV_FULL(%u), RXLEV_SUB(%u),"
	     "RXQUAL_FULL(%u), RXQUAL_SUB(%u), num_meas_sub(%u), num_ul_meas(%u) \n",
	     gsm_lchan_name(lchan),
	     mru->full.rx_lev, mru->sub.rx_lev, mru->full.rx_qual, mru->sub.rx_qual, num_meas_sub, num_ul_meas_expect);

	lchan->meas.flags |= LC_UL_M_F_RES_VALID;

	lchan_meas_compute_extended(lchan);

	lchan->meas.num_ul_meas = 0;

	/* return 1 to indicte that the computation has been done and the next
	 * interval begins. */
	return 1;
}

/* Process a single uplink measurement sample. This function is called from
 * l1sap.c every time a measurement indication is received. It collects the
 * measurement samples and automatically detects the end of the measurement
 * interval. */
int lchan_meas_process_measurement(struct gsm_lchan *lchan, struct bts_ul_meas *ulm, uint32_t fn)
{
	lchan_new_ul_meas(lchan, ulm, fn);
	return lchan_meas_check_compute(lchan, fn);
}

/* Reset all measurement related struct members to their initial values. This
 * function will be called every time an lchan is activated to ensure the
 * measurement process starts with a defined state. */
void lchan_meas_reset(struct gsm_lchan *lchan)
{
	memset(&lchan->meas, 0, sizeof(lchan->meas));
	lchan->meas.last_fn = LCHAN_FN_DUMMY;
}
