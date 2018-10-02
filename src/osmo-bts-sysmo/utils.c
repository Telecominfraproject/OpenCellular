/*
 * Helper utilities that are used in OML
 *
 * (C) 2011-2013 by Harald Welte <laforge@gnumonks.org>
 * (C) 2013 by Holger Hans Peter Freyther
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "utils.h"

#include <osmo-bts/bts.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>

#include "femtobts.h"
#include "l1_if.h"

int band_femto2osmo(GsmL1_FreqBand_t band)
{
	switch (band) {
	case GsmL1_FreqBand_850:
		return GSM_BAND_850;
	case GsmL1_FreqBand_900:
		return GSM_BAND_900;
	case GsmL1_FreqBand_1800:
		return GSM_BAND_1800;
	case GsmL1_FreqBand_1900:
		return GSM_BAND_1900;
	default:
		return -1;
	}
}

static int band_osmo2femto(struct gsm_bts_trx *trx, enum gsm_band osmo_band)
{
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);

	/* check if the TRX hardware actually supports the given band */
	if (!(fl1h->hw_info.band_support & osmo_band))
		return -1;

	/* if yes, convert from osmcoom style band definition to L1 band */
	switch (osmo_band) {
	case GSM_BAND_850:
		return GsmL1_FreqBand_850;
	case GSM_BAND_900:
		return GsmL1_FreqBand_900;
	case GSM_BAND_1800:
		return GsmL1_FreqBand_1800;
	case GSM_BAND_1900:
		return GsmL1_FreqBand_1900;
	default:
		return -1;
	}
}

/**
 * Select the band that matches the ARFCN. In general the ARFCNs
 * for GSM1800 and GSM1900 overlap and one needs to specify the
 * rightband. When moving between GSM900/GSM1800 and GSM850/1900
 * modifying the BTS configuration is a bit annoying. The auto-band
 * configuration allows to ease with this transition.
 */
int sysmobts_select_femto_band(struct gsm_bts_trx *trx, uint16_t arfcn)
{
	enum gsm_band band;
	struct gsm_bts *bts = trx->bts;

	if (!bts->auto_band)
		return band_osmo2femto(trx, bts->band);

	/*
	 * We need to check what will happen now.
	 */
	band = gsm_arfcn2band(arfcn);

	/* if we are already on the right band return */
	if (band == bts->band)
		return band_osmo2femto(trx, bts->band);

	/* Check if it is GSM1800/GSM1900 */
	if (band == GSM_BAND_1800 && bts->band == GSM_BAND_1900)
		return band_osmo2femto(trx, bts->band);

	/*
	 * Now to the actual autobauding. We just want DCS/DCS and
	 * PCS/PCS for PCS we check for 850/1800 though
	 */
	if ((band == GSM_BAND_900 && bts->band == GSM_BAND_1800)
		|| (band == GSM_BAND_1800 && bts->band == GSM_BAND_900)
		|| (band == GSM_BAND_850 && bts->band == GSM_BAND_1900))
		return band_osmo2femto(trx, band);
	if (band == GSM_BAND_1800 && bts->band == GSM_BAND_850)
		return band_osmo2femto(trx, GSM_BAND_1900);

	/* give up */
	return -1;
}
