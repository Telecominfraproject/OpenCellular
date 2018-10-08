#define ULM(ber, ta, sub, neg_rssi) \
	{ .ber10k = (ber), .ta_offs_256bits = (ta), .c_i = 1.0, .is_sub = sub, .inv_rssi = (neg_rssi) }

struct meas_testcase {
	const char *name;
	/* input data */
	const struct bts_ul_meas *ulm;
	unsigned int ulm_count;
	uint32_t final_fn;
	uint8_t ts;
	enum gsm_phys_chan_config pchan;
	/* results */
	struct {
		int success;
		uint8_t rx_lev_full;
		uint8_t rx_qual_full;
		int16_t toa256_mean;
		int16_t toa256_min;
		int16_t toa256_max;
		uint16_t toa256_std_dev;
	} res;
};

static struct bts_ul_meas ulm1[] = {
	/* Note: The assumptions about the frame number and the subset
	 * allegiance is random since for the calculation only the amount
	 * is of relevance. This is true for all following testcases */
	ULM(0, 0, 0, 90),
	ULM(0, 256, 0, 90),
	ULM(0, -256, 0, 90),
};

static const struct meas_testcase mtc1 = {
	.name = "TOA256 Min-Max negative/positive",
	.ulm = ulm1,
	.ulm_count = ARRAY_SIZE(ulm1),
	.final_fn = 25,
	.ts = 1,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 110-90,
		.rx_qual_full = 0,
		.toa256_mean = 0,
		.toa256_max = 256,
		.toa256_min = -256,
		.toa256_std_dev = 209,
	},
};

static struct bts_ul_meas ulm2[] = {
	ULM(0, 256, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 1, 90),
	ULM(0, 256, 0, 90),
	ULM(0, 256, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 1, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 0, 90),
	ULM(0, 256, 1, 90),
	ULM(0, 256, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 0, 90),
	ULM(0, 256, 0, 90),
	ULM(0, 256, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 0, 90),
	ULM(0, 258, 0, 90),
	ULM(0, 254, 0, 90),
	ULM(0, 256, 0, 90),
	ULM(0, 256, 0, 90),
};

static const struct meas_testcase mtc2 = {
	.name = "TOA256 small jitter around 256",
	.ulm = ulm2,
	.ulm_count = ARRAY_SIZE(ulm2),
	.final_fn = 25,
	.ts = 1,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 110-90,
		.rx_qual_full = 7,
		.toa256_mean = 256,
		.toa256_max = 258,
		.toa256_min = 254,
		.toa256_std_dev = 1,
	},
};

static struct bts_ul_meas ulm3[] = {
	ULM(0, 0, 0, 90),
	ULM(0, 0, 0, 80),
	ULM(0, 0, 0, 80),
	ULM(0, 0, 0, 100),
	ULM(0, 0, 0, 100),
};

static const struct meas_testcase mtc3 = {
	.name = "RxLEv averaging",
	.ulm = ulm3,
	.ulm_count = ARRAY_SIZE(ulm3),
	.final_fn = 25,
	.ts = 1,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 110-90,
		.rx_qual_full = 0,
		.toa256_mean = 0,
		.toa256_max = 0,
		.toa256_min = 0,
		.toa256_std_dev = 0,
	},
};

static struct bts_ul_meas ulm4[] = {};

static const struct meas_testcase mtc4 = {
	.name = "Empty measurements",
	.ulm = ulm4,
	.ulm_count = ARRAY_SIZE(ulm4),
	.final_fn = 25,
	.ts = 1,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 63,
		.rx_qual_full = 3,
		.toa256_mean = 0,
		.toa256_max = 0,
		.toa256_min = 0,
		.toa256_std_dev = 0,
	},
};

static struct bts_ul_meas ulm5[] = {
	/* one 104 multiframe can at max contain 26 blocks (TCH/F),
	 * each of which can at maximum be 64 bits in advance (TA range) */
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
};

static const struct meas_testcase mtc5 = {
	.name = "TOA256 26 blocks with max TOA256",
	.ulm = ulm5,
	.ulm_count = ARRAY_SIZE(ulm5),
	.final_fn = 25,
	.ts = 1,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 110-90,
		.rx_qual_full = 0,
		.toa256_mean = 64*256,
		.toa256_max = 64*256,
		.toa256_min = 64*256,
		.toa256_std_dev = 0,
	},
};

/* This testcase models a good case as we can see it when all TCH
 * and SACCH blocks are received */
static struct bts_ul_meas ulm_tch_f_complete[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
};

static const struct meas_testcase mtc_tch_f_complete = {
	.name = "Complete TCH/F measurement period (26 measurements, 3 sub-frames)",
	.ulm = ulm_tch_f_complete,
	.ulm_count = ARRAY_SIZE(ulm_tch_f_complete),
	.final_fn = 38,
	.ts = 2,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 0,
		.toa256_mean = 64*256,
		.toa256_max = 64*256,
		.toa256_min = 64*256,
		.toa256_std_dev = 0,
	},
};

/* This testcase models an error case where two of 3 expected sub measurements
 * are lost. The calculation logic must detect this and replace those
 * measurements. Note that this example also lacks some blocks due to DTX,
 * which is normal. */
static struct bts_ul_meas ulm_tch_f_dtx_with_lost_subs[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
};

static const struct meas_testcase mtc_tch_f_dtx_with_lost_subs = {
	/* This testcase models a good case as we can see it when all TCH
	 * and SACCH blocks are received */
	.name = "Incomplete TCH/F measurement period (16 measurements, 1 sub-frame)",
	.ulm = ulm_tch_f_dtx_with_lost_subs,
	.ulm_count = ARRAY_SIZE(ulm_tch_f_dtx_with_lost_subs),
	.final_fn = 38,
	.ts = 2,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 7,
		.toa256_mean = 16384,
		.toa256_max = 16384,
		.toa256_min = 16384,
		.toa256_std_dev = 0,
	},
};

/* This testcase models a good-case with DTX. Some measurements are missing
 * because no block was transmitted, all sub measurements are there. */
static struct bts_ul_meas ulm_tch_f_dtx[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
};

static const struct meas_testcase mtc_tch_f_dtx = {
	.name = "Incomplete but normal TCH/F measurement period (16 measurements, 3 sub-frames)",
	.ulm = ulm_tch_f_dtx,
	.ulm_count = ARRAY_SIZE(ulm_tch_f_dtx),
	.final_fn = 38,
	.ts = 2,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 7,
		.toa256_mean = 16384,
		.toa256_max = 16384,
		.toa256_min = 16384,
		.toa256_std_dev = 0,
	},
};

/* This testcase models a good case as we can see it when all TCH
 * and SACCH blocks are received */
static struct bts_ul_meas ulm_tch_h_complete[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
};

static const struct meas_testcase mtc_tch_h_complete = {
	.name = "Complete TCH/H measurement period (26 measurements, 5 sub-frames)",
	.ulm = ulm_tch_h_complete,
	.ulm_count = ARRAY_SIZE(ulm_tch_h_complete),
	.final_fn = 38,
	.ts = 2,
	.pchan = GSM_PCHAN_TCH_H,
	.res = {
		.success = 1,
		.rx_lev_full = 110 - 90,
		.rx_qual_full = 0,
		.toa256_mean = 64*256,
		.toa256_max = 64*256,
		.toa256_min = 64*256,
		.toa256_std_dev = 0,
	},
};

static struct bts_ul_meas ulm_tch_h_dtx_with_lost_subs[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
};

static const struct meas_testcase mtc_tch_h_dtx_with_lost_subs = {
	.name = "Incomplete TCH/H measurement period (14 measurements, 3 sub-frames)",
	.ulm = ulm_tch_h_dtx_with_lost_subs,
	.ulm_count = ARRAY_SIZE(ulm_tch_h_dtx_with_lost_subs),
	.final_fn = 38,
	.ts = 2,
	.pchan = GSM_PCHAN_TCH_H,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 7,
		.toa256_mean = 16384,
		.toa256_max = 16384,
		.toa256_min = 16384,
		.toa256_std_dev = 0,
	},
};

/* This testcase models a good-case with DTX. Some measurements are missing
 * because no block was transmitted, all sub measurements are there. */
static struct bts_ul_meas ulm_tch_h_dtx[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
};

static const struct meas_testcase mtc_tch_h_dtx = {
	.name = "Incomplete but normal TCH/F measurement period (16 measurements, 5 sub-frames)",
	.ulm = ulm_tch_h_dtx,
	.ulm_count = ARRAY_SIZE(ulm_tch_h_dtx),
	.final_fn = 38,
	.ts = 2,
	.pchan = GSM_PCHAN_TCH_H,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 7,
		.toa256_mean = 16384,
		.toa256_max = 16384,
		.toa256_min = 16384,
		.toa256_std_dev = 0,
	},
};

/* This testcase assumes that too many measurements were collected. This can
 * happen when the measurement calculation for a previous cycle were not
 * executed. In this case the older part of the excess data must be discarded.
 * the calculation algorithm must make sure that the calculation only takes
 * place on the last measurement interval */
static struct bts_ul_meas ulm_overrun[] = {
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	/* All measurements above must be discarded */
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
	ULM(0, 64*256, 0, 90),
};

static const struct meas_testcase mtc_overrun = {
	.name = "TCH/F measurement period with too much measurement values (overrun)",
	.ulm = ulm_overrun,
	.ulm_count = ARRAY_SIZE(ulm_overrun),
	.final_fn = 25,
	.ts = 1,
	.pchan = GSM_PCHAN_TCH_F,
	.res = {
		.success = 1,
		.rx_lev_full = 110 - 90,
		.rx_qual_full = 0,
		.toa256_mean = 64*256,
		.toa256_max = 64*256,
		.toa256_min = 64*256,
		.toa256_std_dev = 0,
	},
};

/* Test SDCCH4 with all frames received */
static struct bts_ul_meas ulm_sdcch4_complete[] = {
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 1, 90),
};

static const struct meas_testcase mtc_sdcch4_complete = {
	.name = "Complete SDCCH4 measurement period (3 measurements)",
	.ulm = ulm_sdcch4_complete,
	.ulm_count = ARRAY_SIZE(ulm_sdcch4_complete),
	.final_fn = 88,
	.ts = 0,
	.pchan = GSM_PCHAN_CCCH_SDCCH4,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 0,
		.toa256_mean = 16384,
		.toa256_max = 16384,
		.toa256_min = 16384,
		.toa256_std_dev = 0,
	},
};

/* Test SDCCH8 with all frames received */
static struct bts_ul_meas ulm_sdcch8_complete[] = {
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 1, 90),
	ULM(0, 64*256, 1, 90),
};

static const struct meas_testcase mtc_sdcch8_complete = {
	.name = "Complete SDCCH8 measurement period (3 measurements)",
	.ulm = ulm_sdcch8_complete,
	.ulm_count = ARRAY_SIZE(ulm_sdcch8_complete),
	.final_fn = 66,
	.ts = 0,
	.pchan = GSM_PCHAN_SDCCH8_SACCH8C,
	.res = {
		.success = 1,
		.rx_lev_full = 20,
		.rx_qual_full = 0,
		.toa256_mean = 16384,
		.toa256_max = 16384,
		.toa256_min = 16384,
		.toa256_std_dev = 0,
	},
};
