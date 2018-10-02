#include <stdint.h>
#include <errno.h>

#include <osmocom/core/logging.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/amr.h>

void amr_log_mr_conf(int ss, int logl, const char *pfx,
		     struct amr_multirate_conf *amr_mrc)
{
	int i;

	LOGP(ss, logl, "%s AMR MR Conf: num_modes=%u",
		pfx, amr_mrc->num_modes);

	for (i = 0; i < amr_mrc->num_modes; i++)
		LOGPC(ss, logl, ", mode[%u] = %u/%u/%u",
			i, amr_mrc->bts_mode[i].mode,
			amr_mrc->bts_mode[i].threshold,
			amr_mrc->bts_mode[i].hysteresis);
	LOGPC(ss, logl, "\n");
}

static inline int get_amr_mode_idx(const struct amr_multirate_conf *amr_mrc,
				   uint8_t cmi)
{
	unsigned int i;
	for (i = 0; i < amr_mrc->num_modes; i++) {
		if (amr_mrc->bts_mode[i].mode == cmi)
			return i;
	}
	return -EINVAL;
}

static inline uint8_t set_cmr_mode_idx(const struct amr_multirate_conf *amr_mrc,
				       uint8_t cmr)
{
	int rc;

	/* Codec Mode Request is in upper 4 bits of RTP payload header,
	 * and we simply copy the CMR into the CMC */
	if (cmr == 0xF) {
		/* FIXME: we need some state about the last codec mode */
		return 0;
	}

	rc = get_amr_mode_idx(amr_mrc, cmr);
	if (rc < 0) {
		/* FIXME: we need some state about the last codec mode */
		LOGP(DRTP, LOGL_INFO, "RTP->L1: overriding CMR %u\n", cmr);
		return 0;
	}
	return rc;
}

static inline uint8_t set_cmi_mode_idx(const struct amr_multirate_conf *amr_mrc,
				       uint8_t cmi)
{
	int rc = get_amr_mode_idx(amr_mrc, cmi);
	if (rc < 0) {
		LOGP(DRTP, LOGL_ERROR, "AMR CMI %u not part of AMR MR set\n",
		     cmi);
		return 0;
	}
	return rc;
}

void amr_set_mode_pref(uint8_t *data, const struct amr_multirate_conf *amr_mrc,
		      uint8_t cmi, uint8_t cmr)
{
	data[0] = set_cmi_mode_idx(amr_mrc, cmi);
	data[1] = set_cmr_mode_idx(amr_mrc, cmr);
}

/* parse a GSM 04.08 MultiRate Config IE (10.5.2.21aa) in a more
 * comfortable internal data structure */
int amr_parse_mr_conf(struct amr_multirate_conf *amr_mrc,
		      const uint8_t *mr_conf, unsigned int len)
{
	uint8_t mr_version = mr_conf[0] >> 5;
	uint8_t num_codecs = 0;
	int i, j = 0;

	if (mr_version != 1) {
		LOGP(DRSL, LOGL_ERROR, "AMR Multirate Version %u unknown\n",
			mr_version);
		goto ret_einval;
	}

	/* check number of active codecs */
	for (i = 0; i < 8; i++) {
		if (mr_conf[1] & (1 << i))
			num_codecs++;
	}

	/* check for minimum length */
	if (num_codecs == 0 ||
	    (num_codecs == 1 && len < 2) ||
	    (num_codecs == 2 && len < 4) ||
	    (num_codecs == 3 && len < 5) ||
	    (num_codecs == 4 && len < 6) ||
	    (num_codecs > 4)) {
		LOGP(DRSL, LOGL_ERROR, "AMR Multirate with %u modes len=%u "
		     "not possible\n", num_codecs, len);
		goto ret_einval;
	}

	/* copy the first two octets of the IE */
	amr_mrc->gsm48_ie[0] = mr_conf[0];
	amr_mrc->gsm48_ie[1] = mr_conf[1];

	amr_mrc->num_modes = num_codecs;

	for (i = 0; i < 8; i++) {
		if (mr_conf[1] & (1 << i)) {
			amr_mrc->bts_mode[j++].mode = i;
		}
	}

	if (num_codecs >= 2) {
		amr_mrc->bts_mode[0].threshold = mr_conf[1] & 0x3F;
		amr_mrc->bts_mode[0].hysteresis = mr_conf[2] >> 4;
	}
	if (num_codecs >= 3) {
		amr_mrc->bts_mode[1].threshold =
			((mr_conf[2] & 0xF) << 2) | (mr_conf[3] >> 6);
		amr_mrc->bts_mode[1].hysteresis = (mr_conf[3] >> 2) & 0xF;
	}
	if (num_codecs >= 4) {
		amr_mrc->bts_mode[2].threshold =
			((mr_conf[3] & 0x3) << 4) | (mr_conf[4] >> 4);
		amr_mrc->bts_mode[2].hysteresis = mr_conf[4] & 0xF;
	}

	return num_codecs;

ret_einval:
	return -EINVAL;
}


/*! \brief determine AMR initial codec mode for given logical channel 
 *  \returns integer between 0..3 for AMR codce mode 1..4 */
unsigned int amr_get_initial_mode(struct gsm_lchan *lchan)
{
	struct amr_multirate_conf *amr_mrc = &lchan->tch.amr_mr;
	struct gsm48_multi_rate_conf *mr_conf =
			(struct gsm48_multi_rate_conf *) amr_mrc->gsm48_ie;

	if (mr_conf->icmi) {
		/* initial mode given, coding in TS 05.09 3.4.1 */
		return mr_conf->smod;
	} else {
		/* implicit rule according to TS 05.09 Chapter 3.4.3 */
		switch (amr_mrc->num_modes) {
		case 2:
		case 3:
			/* return the most robust */
			return 0;
		case 4:
			/* return the second-most robust */
			return 1;
		case 1:
		default:
			/* return the only mode we have */
			return 0;
		}
	}
}
