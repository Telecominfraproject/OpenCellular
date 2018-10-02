/* libosmocore logging support */

/* (C) 2011 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2011 by Harald Welte <laforge@gnumonks.org>
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


#include <errno.h>

#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>
#include <osmocom/core/utils.h>

#include <osmo-bts/bts.h>
#include <osmo-bts/logging.h>

static struct log_info_cat bts_log_info_cat[] = {
	[DRSL] = {
		.name = "DRSL",
		.description = "A-bis Radio Siganlling Link (RSL)",
		.color = "\033[1;35m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DOML] =	{
		.name = "DOML",
		.description = "A-bis Network Management / O&M (NM/OML)",
		.color = "\033[1;36m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DRLL] = {
		.name = "DRLL",
		.description = "A-bis Radio Link Layer (RLL)",
		.color = "\033[1;31m",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
	[DRR] = {
		.name = "DRR",
		.description = "Layer3 Radio Resource (RR)",
		.color = "\033[1;34m",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
	[DMEAS] = {
		.name = "DMEAS",
		.description = "Radio Measurement Processing",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
	[DPAG]	= {
		.name = "DPAG",
		.description = "Paging Subsystem",
		.color = "\033[1;38m",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DL1C] = {
		.name = "DL1C",
		.description = "Layer 1 Control (MPH)",
		.loglevel = LOGL_INFO,
		.enabled = 1,
	},
	[DL1P] = {
		.name = "DL1P",
		.description = "Layer 1 Primitives (PH)",
		.loglevel = LOGL_INFO,
		.enabled = 0,
	},
	[DDSP] = {
		.name = "DDSP",
		.description = "DSP Trace Messages",
		.loglevel = LOGL_DEBUG,
		.enabled = 1,
	},
	[DABIS] = {
		.name = "DABIS",
		.description = "A-bis Intput Subsystem",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
	[DRTP] = {
		.name = "DRTP",
		.description = "Realtime Transfer Protocol",
		.loglevel = LOGL_NOTICE,
		.enabled = 1,
	},
	[DPCU] = {
		.name = "DPCU",
		.description = "PCU interface",
		.loglevel = LOGL_NOTICE,
		.enabled = 1,
	},
	[DHO] = {
		.name = "DHO",
		.description = "Handover",
		.color = "\033[0;37m",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
	[DTRX] = {
		.name = "DTRX",
		.description = "TRX interface",
		.color = "\033[1;33m",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
	[DLOOP] = {
		.name = "DLOOP",
		.description = "Control loops",
		.color = "\033[0;34m",
		.enabled = 1, .loglevel = LOGL_NOTICE,
	},
#if 0
	[DNS] = {
		.name = "DNS",
		.description = "GPRS Network Service (NS)",
		.enabled = 1, .loglevel = LOGL_INFO,
	},
	[DBSSGP] = {
		.name = "DBSSGP",
		.description = "GPRS BSS Gateway Protocol (BSSGP)",
		.enabled = 1, .loglevel = LOGL_DEBUG,
	},
	[DLLC] = {
		.name = "DLLC",
		.description = "GPRS Logical Link Control Protocol (LLC)",
		.enabled = 1, .loglevel = LOGL_DEBUG,
	},
#endif
	[DSUM] = {
		.name = "DSUM",
		.description = "DSUM",
		.loglevel = LOGL_NOTICE,
		.enabled = 1,
	},
};

const struct log_info bts_log_info = {
	.cat = bts_log_info_cat,
	.num_cat = ARRAY_SIZE(bts_log_info_cat),
};
