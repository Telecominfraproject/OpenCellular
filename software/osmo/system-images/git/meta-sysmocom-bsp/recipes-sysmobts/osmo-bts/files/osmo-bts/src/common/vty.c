/* OsmoBTS VTY interface */

/* (C) 2011-2014 by Harald Welte <laforge@gnumonks.org>
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

#include "btsconfig.h"

#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#include <osmocom/core/talloc.h>
#include <osmocom/gsm/abis_nm.h>
#include <osmocom/gsm/protocol/gsm_04_08.h>
#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/logging.h>
#include <osmocom/vty/misc.h>
#include <osmocom/vty/ports.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/utils.h>
#include <osmocom/trau/osmo_ortp.h>


#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/signal.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/pcuif_proto.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/l1sap.h>

#define VTY_STR	"Configure the VTY\n"

#define BTS_NR_STR "BTS Number\n"
#define TRX_NR_STR "TRX Number\n"
#define TS_NR_STR "Timeslot Number\n"
#define LCHAN_NR_STR "Logical Channel Number\n"
#define BTS_TRX_STR BTS_NR_STR TRX_NR_STR
#define BTS_TRX_TS_STR BTS_TRX_STR TS_NR_STR
#define BTS_TRX_TS_LCHAN_STR BTS_TRX_TS_STR LCHAN_NR_STR

int g_vty_port_num = OSMO_VTY_PORT_BTS;

struct phy_instance *vty_get_phy_instance(struct vty *vty, int phy_nr, int inst_nr)
{
	struct phy_link *plink = phy_link_by_num(phy_nr);
	struct phy_instance *pinst;

	if (!plink) {
		vty_out(vty, "Cannot find PHY link number %d%s",
			phy_nr, VTY_NEWLINE);
		return NULL;
	}

	pinst = phy_instance_by_num(plink, inst_nr);
	if (!pinst) {
		vty_out(vty, "Cannot find PHY instance number %d%s",
			inst_nr, VTY_NEWLINE);
		return NULL;
	}
	return pinst;
}

int bts_vty_go_parent(struct vty *vty)
{
	switch (vty->node) {
	case PHY_INST_NODE:
		vty->node = PHY_NODE;
		{
			struct phy_instance *pinst = vty->index;
			vty->index = pinst->phy_link;
		}
		break;
	case TRX_NODE:
		vty->node = BTS_NODE;
		{
			struct gsm_bts_trx *trx = vty->index;
			vty->index = trx->bts;
		}
		break;
	case PHY_NODE:
	default:
		vty->node = CONFIG_NODE;
	}
	return vty->node;
}

int bts_vty_is_config_node(struct vty *vty, int node)
{
	switch (node) {
	case TRX_NODE:
	case BTS_NODE:
	case PHY_NODE:
	case PHY_INST_NODE:
		return 1;
	default:
		return 0;
	}
}

gDEFUN(ournode_exit, ournode_exit_cmd, "exit",
	"Exit current node, go down to provious node")
{
	switch (vty->node) {
	case PHY_INST_NODE:
		vty->node = PHY_NODE;
		{
			struct phy_instance *pinst = vty->index;
			vty->index = pinst->phy_link;
		}
		break;
	case PHY_NODE:
		vty->node = CONFIG_NODE;
		vty->index = NULL;
		break;
	case TRX_NODE:
		vty->node = BTS_NODE;
		{
			struct gsm_bts_trx *trx = vty->index;
			vty->index = trx->bts;
		}
		break;
	default:
		break;
	}
	return CMD_SUCCESS;
}

gDEFUN(ournode_end, ournode_end_cmd, "end",
	"End current mode and change to enable mode")
{
	switch (vty->node) {
	default:
		vty_config_unlock(vty);
		vty->node = ENABLE_NODE;
		vty->index = NULL;
		vty->index_sub = NULL;
		break;
	}
	return CMD_SUCCESS;
}

static const char osmobts_copyright[] =
	"Copyright (C) 2010, 2011 by Harald Welte, Andreas Eversberg and On-Waves\r\n"
	"License AGPLv3+: GNU AGPL version 3 or later <http://gnu.org/licenses/agpl-3.0.html>\r\n"
	"This is free software: you are free to change and redistribute it.\r\n"
	 "There is NO WARRANTY, to the extent permitted by law.\r\n";

struct vty_app_info bts_vty_info = {
	.name		= "OsmoBTS",
	.version	= PACKAGE_VERSION,
	.copyright	= osmobts_copyright,
	.go_parent_cb	= bts_vty_go_parent,
	.is_config_node	= bts_vty_is_config_node,
};

extern struct gsm_network bts_gsmnet;

struct gsm_network *gsmnet_from_vty(struct vty *v)
{
	return &bts_gsmnet;
}

static struct cmd_node bts_node = {
	BTS_NODE,
	"%s(bts)# ",
	1,
};

static struct cmd_node trx_node = {
	TRX_NODE,
	"%s(trx)# ",
	1,
};

gDEFUN(cfg_bts_auto_band, cfg_bts_auto_band_cmd,
	"auto-band",
	"Automatically select band for ARFCN based on configured band\n")
{
	struct gsm_bts *bts = vty->index;

	bts->auto_band = 1;
	return CMD_SUCCESS;
}

gDEFUN(cfg_bts_no_auto_band, cfg_bts_no_auto_band_cmd,
	"no auto-band",
	NO_STR "Automatically select band for ARFCN based on configured band\n")
{
	struct gsm_bts *bts = vty->index;

	bts->auto_band = 0;
	return CMD_SUCCESS;
}


DEFUN(cfg_bts_trx, cfg_bts_trx_cmd,
	"trx <0-254>",
	"Select a TRX to configure\n" "TRX number\n")
{
	int trx_nr = atoi(argv[0]);
	struct gsm_bts *bts = vty->index;
	struct gsm_bts_trx *trx;

	trx = gsm_bts_trx_num(bts, trx_nr);
	if (!trx) {
		vty_out(vty, "Unknown TRX %u. Available TRX are: 0..%u%s",
			trx_nr, bts->num_trx - 1, VTY_NEWLINE);
		vty_out(vty, "Hint: Check if commandline option -t matches the"
			"number of available transceivers!%s", VTY_NEWLINE);
		return CMD_WARNING;
	}

	vty->index = trx;
	vty->index_sub = &trx->description;
	vty->node = TRX_NODE;

	return CMD_SUCCESS;
}

static void config_write_bts_single(struct vty *vty, struct gsm_bts *bts)
{
	struct gsm_bts_trx *trx;
	char buf_casecnvt[256];
	int i;

	vty_out(vty, "bts %u%s", bts->nr, VTY_NEWLINE);
	if (bts->description)
		vty_out(vty, " description %s%s", bts->description, VTY_NEWLINE);
	vty_out(vty, " band %s%s", gsm_band_name(bts->band), VTY_NEWLINE);
	if (bts->auto_band)
		vty_out(vty, " auto-band%s", VTY_NEWLINE);
	vty_out(vty, " ipa unit-id %u %u%s",
		bts->ip_access.site_id, bts->ip_access.bts_id, VTY_NEWLINE);
	vty_out(vty, " oml remote-ip %s%s", bts->bsc_oml_host, VTY_NEWLINE);
	vty_out(vty, " rtp jitter-buffer %u", bts->rtp_jitter_buf_ms);
	if (bts->rtp_jitter_adaptive)
		vty_out(vty, " adaptive");
	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, " rtp port-range %u %u%s", bts->rtp_port_range_start,
		bts->rtp_port_range_end, VTY_NEWLINE);
	vty_out(vty, " paging queue-size %u%s", paging_get_queue_max(bts->paging_state),
		VTY_NEWLINE);
	vty_out(vty, " paging lifetime %u%s", paging_get_lifetime(bts->paging_state),
		VTY_NEWLINE);
	vty_out(vty, " uplink-power-target %d%s", bts->ul_power_target, VTY_NEWLINE);
	if (bts->agch_queue.thresh_level != GSM_BTS_AGCH_QUEUE_THRESH_LEVEL_DEFAULT
		 || bts->agch_queue.low_level != GSM_BTS_AGCH_QUEUE_LOW_LEVEL_DEFAULT
		 || bts->agch_queue.high_level != GSM_BTS_AGCH_QUEUE_HIGH_LEVEL_DEFAULT)
		vty_out(vty, " agch-queue-mgmt threshold %d low %d high %d%s",
			bts->agch_queue.thresh_level, bts->agch_queue.low_level,
			bts->agch_queue.high_level, VTY_NEWLINE);

	for (i = 0; i < 32; i++) {
		if (gsmtap_sapi_mask & (1 << i)) {
			osmo_str2lower(buf_casecnvt, get_value_string(gsmtap_sapi_names, i));
			vty_out(vty, " gsmtap-sapi %s%s", buf_casecnvt, VTY_NEWLINE);
		}
	}
	if (gsmtap_sapi_acch) {
		osmo_str2lower(buf_casecnvt, get_value_string(gsmtap_sapi_names, GSMTAP_CHANNEL_ACCH));
		vty_out(vty, " gsmtap-sapi %s%s", buf_casecnvt, VTY_NEWLINE);
	}
	vty_out(vty, " min-qual-rach %.0f%s", bts->min_qual_rach * 10.0f,
		VTY_NEWLINE);
	vty_out(vty, " min-qual-norm %.0f%s", bts->min_qual_norm * 10.0f,
		VTY_NEWLINE);
	vty_out(vty, " max-ber10k-rach %u%s", bts->max_ber10k_rach,
		VTY_NEWLINE);
	if (strcmp(bts->pcu.sock_path, PCU_SOCK_DEFAULT))
		vty_out(vty, " pcu-socket %s%s", bts->pcu.sock_path, VTY_NEWLINE);
	if (bts->supp_meas_toa256)
		vty_out(vty, " supp-meas-info toa256%s", VTY_NEWLINE);

	bts_model_config_write_bts(vty, bts);

	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct trx_power_params *tpp = &trx->power_params;
		struct phy_instance *pinst = trx_phy_instance(trx);
		vty_out(vty, " trx %u%s", trx->nr, VTY_NEWLINE);

		if (trx->power_params.user_gain_mdB)
			vty_out(vty, "  user-gain %u mdB%s",
				tpp->user_gain_mdB, VTY_NEWLINE);
		vty_out(vty, "  power-ramp max-initial %d mdBm%s",
			tpp->ramp.max_initial_pout_mdBm, VTY_NEWLINE);
		vty_out(vty, "  power-ramp step-size %d mdB%s",
			tpp->ramp.step_size_mdB, VTY_NEWLINE);
		vty_out(vty, "  power-ramp step-interval %d%s",
			tpp->ramp.step_interval_sec, VTY_NEWLINE);
		vty_out(vty, "  ms-power-control %s%s",
			trx->ms_power_control == 0 ? "dsp" : "osmo",
			VTY_NEWLINE);
		vty_out(vty, "  phy %u instance %u%s", pinst->phy_link->num,
			pinst->num, VTY_NEWLINE);

		bts_model_config_write_trx(vty, trx);
	}
}

static int config_write_bts(struct vty *vty)
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_bts *bts;

	llist_for_each_entry(bts, &net->bts_list, list)
		config_write_bts_single(vty, bts);

	return CMD_SUCCESS;
}

static void config_write_phy_single(struct vty *vty, struct phy_link *plink)
{
	int i;

	vty_out(vty, "phy %u%s", plink->num, VTY_NEWLINE);
	bts_model_config_write_phy(vty, plink);

	for (i = 0; i < 255; i++) {
		struct phy_instance *pinst = phy_instance_by_num(plink, i);
		if (!pinst)
			break;
		vty_out(vty, " instance %u%s", pinst->num, VTY_NEWLINE);
		bts_model_config_write_phy_inst(vty, pinst);
	}
}

static int config_write_phy(struct vty *vty)
{
	int i;

	for (i = 0; i < 255; i++) {
		struct phy_link *plink = phy_link_by_num(i);
		if (!plink)
			break;
		config_write_phy_single(vty, plink);
	}

	return CMD_SUCCESS;
}

static int config_write_dummy(struct vty *vty)
{
	return CMD_SUCCESS;
}

DEFUN(cfg_vty_telnet_port, cfg_vty_telnet_port_cmd,
	"vty telnet-port <0-65535>",
	VTY_STR "Set the VTY telnet port\n"
	"TCP Port number\n")
{
	g_vty_port_num = atoi(argv[0]);
	return CMD_SUCCESS;
}

/* per-BTS configuration */
DEFUN(cfg_bts,
      cfg_bts_cmd,
      "bts BTS_NR",
      "Select a BTS to configure\n"
	"BTS Number\n")
{
	struct gsm_network *gsmnet = gsmnet_from_vty(vty);
	int bts_nr = atoi(argv[0]);
	struct gsm_bts *bts;

	if (bts_nr >= gsmnet->num_bts) {
		vty_out(vty, "%% Unknown BTS number %u (num %u)%s",
			bts_nr, gsmnet->num_bts, VTY_NEWLINE);
		return CMD_WARNING;
	} else
		bts = gsm_bts_num(gsmnet, bts_nr);

	vty->index = bts;
	vty->index_sub = &bts->description;
	vty->node = BTS_NODE;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_unit_id,
      cfg_bts_unit_id_cmd,
      "ipa unit-id <0-65534> <0-255>",
      "ip.access RSL commands\n"
      "Set the Unit ID of this BTS\n"
      "Site ID\n" "Unit ID\n")
{
	struct gsm_bts *bts = vty->index;
	int site_id = atoi(argv[0]);
	int bts_id = atoi(argv[1]);

	bts->ip_access.site_id = site_id;
	bts->ip_access.bts_id = bts_id;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_band,
      cfg_bts_band_cmd,
      "band (450|GSM450|480|GSM480|750|GSM750|810|GSM810|850|GSM850|900|GSM900|1800|DCS1800|1900|PCS1900)",
      "Set the frequency band of this BTS\n"
      "Alias for GSM450\n450Mhz\n"
      "Alias for GSM480\n480Mhz\n"
      "Alias for GSM750\n750Mhz\n"
      "Alias for GSM810\n810Mhz\n"
      "Alias for GSM850\n850Mhz\n"
      "Alias for GSM900\n900Mhz\n"
      "Alias for DCS1800\n1800Mhz\n"
      "Alias for PCS1900\n1900Mhz\n")
{
	struct gsm_bts *bts = vty->index;
	int band = gsm_band_parse(argv[0]);

	if (band < 0) {
		vty_out(vty, "%% BAND %d is not a valid GSM band%s",
			band, VTY_NEWLINE);
		return CMD_WARNING;
	}

	bts->band = band;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_oml_ip,
      cfg_bts_oml_ip_cmd,
      "oml remote-ip A.B.C.D",
      "OML Parameters\n" "OML IP Address\n" "OML IP Address\n")
{
	struct gsm_bts *bts = vty->index;

	if (bts->bsc_oml_host)
		talloc_free(bts->bsc_oml_host);

	bts->bsc_oml_host = talloc_strdup(bts, argv[0]);

	return CMD_SUCCESS;
}

#define RTP_STR "RTP parameters\n"

DEFUN_DEPRECATED(cfg_bts_rtp_bind_ip,
      cfg_bts_rtp_bind_ip_cmd,
      "rtp bind-ip A.B.C.D",
      RTP_STR "RTP local bind IP Address\n" "RTP local bind IP Address\n")
{
	vty_out(vty, "%% rtp bind-ip is now deprecated%s", VTY_NEWLINE);

	return CMD_WARNING;
}

DEFUN(cfg_bts_rtp_jitbuf,
	cfg_bts_rtp_jitbuf_cmd,
	"rtp jitter-buffer <0-10000> [adaptive]",
	RTP_STR "RTP jitter buffer\n" "jitter buffer in ms\n")
{
	struct gsm_bts *bts = vty->index;

	bts->rtp_jitter_buf_ms = atoi(argv[0]);
	if (argc > 1)
		bts->rtp_jitter_adaptive = true;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_rtp_port_range,
	cfg_bts_rtp_port_range_cmd,
	"rtp port-range <1-65534> <1-65534>",
      RTP_STR "Range of local ports to use for RTP/RTCP traffic\n")
{
	struct gsm_bts *bts = vty->index;
	unsigned int start;
	unsigned int end;

	start = atoi(argv[0]);
	end = atoi(argv[1]);

	if (end < start) {
		vty_out(vty, "range end port (%u) must be greater than the range start port (%u)!%s",
			end, start, VTY_NEWLINE);
		return CMD_WARNING;
	}

	if (start & 1) {
		vty_out(vty, "range must begin at an even port number! (%u not even)%s",
			start, VTY_NEWLINE);
		return CMD_WARNING;
	}

	if ((end & 1) == 0) {
		vty_out(vty, "range must end at an odd port number! (%u not odd)%s",
			end, VTY_NEWLINE);
		return CMD_WARNING;
	}

	bts->rtp_port_range_start = start;
	bts->rtp_port_range_end = end;
	bts->rtp_port_range_next = bts->rtp_port_range_start;

	return CMD_SUCCESS;
}

#define PAG_STR "Paging related parameters\n"

DEFUN(cfg_bts_paging_queue_size,
	cfg_bts_paging_queue_size_cmd,
	"paging queue-size <1-1024>",
	PAG_STR "Maximum length of BTS-internal paging queue\n"
	        "Maximum length of BTS-internal paging queue\n")
{
	struct gsm_bts *bts = vty->index;

	paging_set_queue_max(bts->paging_state, atoi(argv[0]));

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_paging_lifetime,
	cfg_bts_paging_lifetime_cmd,
	"paging lifetime <0-60>",
	PAG_STR "Maximum lifetime of a paging record\n"
	        "Maximum lifetime of a paging record (secods)\n")
{
	struct gsm_bts *bts = vty->index;

	paging_set_lifetime(bts->paging_state, atoi(argv[0]));

	return CMD_SUCCESS;
}

#define AGCH_QUEUE_STR "AGCH queue mgmt\n"

DEFUN(cfg_bts_agch_queue_mgmt_params,
	cfg_bts_agch_queue_mgmt_params_cmd,
	"agch-queue-mgmt threshold <0-100> low <0-100> high <0-100000>",
	AGCH_QUEUE_STR
	"Threshold to start cleanup\nin %% of the maximum queue length\n"
	"Low water mark for cleanup\nin %% of the maximum queue length\n"
	"High water mark for cleanup\nin %% of the maximum queue length\n")
{
	struct gsm_bts *bts = vty->index;

	bts->agch_queue.thresh_level = atoi(argv[0]);
	bts->agch_queue.low_level = atoi(argv[1]);
	bts->agch_queue.high_level = atoi(argv[2]);

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_agch_queue_mgmt_default,
	cfg_bts_agch_queue_mgmt_default_cmd,
	"agch-queue-mgmt default",
	AGCH_QUEUE_STR
	"Reset clean parameters to default values\n")
{
	struct gsm_bts *bts = vty->index;

	bts->agch_queue.thresh_level = GSM_BTS_AGCH_QUEUE_THRESH_LEVEL_DEFAULT;
	bts->agch_queue.low_level = GSM_BTS_AGCH_QUEUE_LOW_LEVEL_DEFAULT;
	bts->agch_queue.high_level = GSM_BTS_AGCH_QUEUE_HIGH_LEVEL_DEFAULT;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_ul_power_target, cfg_bts_ul_power_target_cmd,
	"uplink-power-target <-110-0>",
	"Set the nominal target Rx Level for uplink power control loop\n"
	"Target uplink Rx level in dBm\n")
{
	struct gsm_bts *bts = vty->index;

	bts->ul_power_target = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_min_qual_rach, cfg_bts_min_qual_rach_cmd,
	"min-qual-rach <-100-100>",
	"Set the minimum quality level of RACH burst to be accpeted\n"
	"C/I level in tenth of dB\n")
{
	struct gsm_bts *bts = vty->index;

	bts->min_qual_rach = strtof(argv[0], NULL) / 10.0f;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_min_qual_norm, cfg_bts_min_qual_norm_cmd,
	"min-qual-norm <-100-100>",
	"Set the minimum quality level of normal burst to be accpeted\n"
	"C/I level in tenth of dB\n")
{
	struct gsm_bts *bts = vty->index;

	bts->min_qual_norm = strtof(argv[0], NULL) / 10.0f;

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_max_ber_rach, cfg_bts_max_ber_rach_cmd,
	"max-ber10k-rach <0-10000>",
	"Set the maximum BER for valid RACH requests\n"
	"BER in 1/10000 units (0=no BER; 100=1% BER)\n")
{
	struct gsm_bts *bts = vty->index;

	bts->max_ber10k_rach = strtoul(argv[0], NULL, 10);

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_pcu_sock, cfg_bts_pcu_sock_cmd,
	"pcu-socket PATH",
	"Configure the PCU socket file/path name\n")
{
	struct gsm_bts *bts = vty->index;

	if (bts->pcu.sock_path) {
		/* FIXME: close the interface? */
		talloc_free(bts->pcu.sock_path);
	}
	bts->pcu.sock_path = talloc_strdup(bts, argv[0]);
	/* FIXME: re-open the interface? */

	return CMD_SUCCESS;
}

DEFUN(cfg_bts_supp_meas_toa256, cfg_bts_supp_meas_toa256_cmd,
	"supp-meas-info toa256",
	"Configure the RSL Supplementary Measurement Info\n"
	"Report the TOA in 1/256th symbol periods\n")
{
	struct gsm_bts *bts = vty->index;

	bts->supp_meas_toa256 = true;
	return CMD_SUCCESS;
}

DEFUN(cfg_bts_no_supp_meas_toa256, cfg_bts_no_supp_meas_toa256_cmd,
	"no supp-meas-info toa256",
	NO_STR "Configure the RSL Supplementary Measurement Info\n"
	"Report the TOA in 1/256th symbol periods\n")
{
	struct gsm_bts *bts = vty->index;

	bts->supp_meas_toa256 = false;
	return CMD_SUCCESS;
}


#define DB_DBM_STR 							\
	"Unit is dB (decibels)\n"					\
	"Unit is mdB (milli-decibels, or rather 1/10000 bel)\n"

static int parse_mdbm(const char *valstr, const char *unit)
{
	int val = atoi(valstr);

	if (!strcmp(unit, "dB") || !strcmp(unit, "dBm"))
		return val * 1000;
	else
		return val;
}

DEFUN(cfg_trx_user_gain,
	cfg_trx_user_gain_cmd,
	"user-gain <-100000-100000> (dB|mdB)",
	"Inform BTS about additional, user-provided gain or attenuation at TRX output\n"
	"Value of user-provided external gain(+)/attenuation(-)\n" DB_DBM_STR)
{
	struct gsm_bts_trx *trx = vty->index;

	trx->power_params.user_gain_mdB = parse_mdbm(argv[0], argv[1]);

	return CMD_SUCCESS;
}

#define PR_STR "Power-Ramp settings"
DEFUN(cfg_trx_pr_max_initial, cfg_trx_pr_max_initial_cmd,
	"power-ramp max-initial <0-100000> (dBm|mdBm)",
	PR_STR "Maximum initial power\n"
	"Value\n" DB_DBM_STR)
{
	struct gsm_bts_trx *trx = vty->index;

	trx->power_params.ramp.max_initial_pout_mdBm =
				parse_mdbm(argv[0], argv[1]);

	return CMD_SUCCESS;
}

DEFUN(cfg_trx_pr_step_size, cfg_trx_pr_step_size_cmd,
	"power-ramp step-size <1-100000> (dB|mdB)",
	PR_STR "Power increase by step\n"
	"Step size\n" DB_DBM_STR)
{
	struct gsm_bts_trx *trx = vty->index;

	trx->power_params.ramp.step_size_mdB =
				parse_mdbm(argv[0], argv[1]);
	return CMD_SUCCESS;
}

DEFUN(cfg_trx_pr_step_interval, cfg_trx_pr_step_interval_cmd,
	"power-ramp step-interval <1-100>",
	PR_STR "Power increase by step\n"
	"Step time in seconds\n")
{
	struct gsm_bts_trx *trx = vty->index;

	trx->power_params.ramp.step_interval_sec = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_trx_ms_power_control, cfg_trx_ms_power_control_cmd,
	"ms-power-control (dsp|osmo)",
	"Mobile Station Power Level Control (change requires restart)\n"
	"Handled by DSP\n" "Handled by OsmoBTS\n")
{
	struct gsm_bts_trx *trx = vty->index;

	trx->ms_power_control = argv[0][0] == 'd' ? 0 : 1;
	return CMD_SUCCESS;
}

DEFUN(cfg_trx_phy, cfg_trx_phy_cmd,
	"phy <0-255> instance <0-255>",
	"Configure PHY Link+Instance for this TRX\n"
	"PHY Link number\n" "PHY instance\n" "PHY Instance number")
{
	struct gsm_bts_trx *trx = vty->index;
	struct phy_link *plink = phy_link_by_num(atoi(argv[0]));
	struct phy_instance *pinst;

	if (!plink) {
		vty_out(vty, "phy%s does not exist%s",
			argv[0], VTY_NEWLINE);
		return CMD_WARNING;
	}

	pinst = phy_instance_by_num(plink, atoi(argv[1]));
	if (!pinst) {
		vty_out(vty, "phy%s instance %s does not exit%s",
			argv[0], argv[1], VTY_NEWLINE);
		return CMD_WARNING;
	}

	trx->role_bts.l1h = pinst;
	pinst->trx = trx;

	return CMD_SUCCESS;
}

/* ======================================================================
 * SHOW
 * ======================================================================*/

static void net_dump_nmstate(struct vty *vty, struct gsm_nm_state *nms)
{
	vty_out(vty,"Oper '%s', Admin '%s', Avail '%s'%s",
		abis_nm_opstate_name(nms->operational),
		get_value_string(abis_nm_adm_state_names, nms->administrative),
		abis_nm_avail_name(nms->availability), VTY_NEWLINE);
}

static unsigned int llist_length(struct llist_head *list)
{
	unsigned int len = 0;
	struct llist_head *pos;

	llist_for_each(pos, list)
		len++;

	return len;
}

static void bts_dump_vty_features(struct vty *vty, struct gsm_bts *bts)
{
	unsigned int i;
	bool no_features = true;
	vty_out(vty, "  Features:%s", VTY_NEWLINE);

	for (i = 0; i < _NUM_BTS_FEAT; i++) {
		if (gsm_bts_has_feature(bts, i)) {
			vty_out(vty, "    %03u ", i);
			vty_out(vty, "%-40s%s", get_value_string(gsm_bts_features_descs, i), VTY_NEWLINE);
			no_features = false;
		}
	}

	if (no_features)
		vty_out(vty, "    (not available)%s", VTY_NEWLINE);
}

static void bts_dump_vty(struct vty *vty, struct gsm_bts *bts)
{
	struct gsm_bts_trx *trx;

	vty_out(vty, "BTS %u is of %s type in band %s, has CI %u LAC %u, "
		"BSIC %u and %u TRX%s",
		bts->nr, "FIXME", gsm_band_name(bts->band),
		bts->cell_identity,
		bts->location_area_code, bts->bsic,
		bts->num_trx, VTY_NEWLINE);
	vty_out(vty, "  Description: %s%s",
		bts->description ? bts->description : "(null)", VTY_NEWLINE);
	vty_out(vty, "  Unit ID: %u/%u/0, OML Stream ID 0x%02x%s",
			bts->ip_access.site_id, bts->ip_access.bts_id,
			bts->oml_tei, VTY_NEWLINE);
	vty_out(vty, "  NM State: ");
	net_dump_nmstate(vty, &bts->mo.nm_state);
	vty_out(vty, "  Site Mgr NM State: ");
	net_dump_nmstate(vty, &bts->site_mgr.mo.nm_state);
	if (strnlen(bts->pcu_version, MAX_VERSION_LENGTH))
		vty_out(vty, "  PCU version %s connected%s",
			bts->pcu_version, VTY_NEWLINE);
	vty_out(vty, "  Paging: Queue size %u, occupied %u, lifetime %us%s",
		paging_get_queue_max(bts->paging_state), paging_queue_length(bts->paging_state),
		paging_get_lifetime(bts->paging_state), VTY_NEWLINE);
	vty_out(vty, "  AGCH: Queue limit %u, occupied %d, "
		"dropped %"PRIu64", merged %"PRIu64", rejected %"PRIu64", "
		"ag-res %"PRIu64", non-res %"PRIu64"%s",
		bts->agch_queue.max_length, bts->agch_queue.length,
		bts->agch_queue.dropped_msgs, bts->agch_queue.merged_msgs,
		bts->agch_queue.rejected_msgs, bts->agch_queue.agch_msgs,
		bts->agch_queue.pch_msgs,
		VTY_NEWLINE);
	vty_out(vty, "  CBCH backlog queue length: %u%s",
		llist_length(&bts->smscb_state.queue), VTY_NEWLINE);
	vty_out(vty, "  Paging: queue length %d, buffer space %d%s",
		paging_queue_length(bts->paging_state), paging_buffer_space(bts->paging_state),
		VTY_NEWLINE);
	vty_out(vty, "  OML Link state: %s.%s",
		bts->oml_link ? "connected" : "disconnected", VTY_NEWLINE);

	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct phy_instance *pinst = trx_phy_instance(trx);
		vty_out(vty, "  TRX %u%s", trx->nr, VTY_NEWLINE);
		if (pinst) {
			vty_out(vty, "    phy %d %s", pinst->num, pinst->version);
			if (pinst->description)
				vty_out(vty, " (%s)", pinst->description);
			vty_out(vty, "%s", VTY_NEWLINE);
		}
	}

	bts_dump_vty_features(vty, bts);
	vty_out_rate_ctr_group(vty, "  ", bts->ctrs);
}


DEFUN(show_bts, show_bts_cmd, "show bts <0-255>",
	SHOW_STR "Display information about a BTS\n"
		BTS_NR_STR)
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	int bts_nr;

	if (argc != 0) {
		/* use the BTS number that the user has specified */
		bts_nr = atoi(argv[0]);
		if (bts_nr >= net->num_bts) {
			vty_out(vty, "%% can't find BTS '%s'%s", argv[0],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		bts_dump_vty(vty, gsm_bts_num(net, bts_nr));
		return CMD_SUCCESS;
	}
	/* print all BTS's */
	for (bts_nr = 0; bts_nr < net->num_bts; bts_nr++)
		bts_dump_vty(vty, gsm_bts_num(net, bts_nr));

	return CMD_SUCCESS;
}

static void trx_dump_vty(struct vty *vty, struct gsm_bts_trx *trx)
{
	vty_out(vty, "TRX %u of BTS %u is on ARFCN %u%s",
		trx->nr, trx->bts->nr, trx->arfcn, VTY_NEWLINE);
	vty_out(vty, "Description: %s%s",
		trx->description ? trx->description : "(null)", VTY_NEWLINE);
	vty_out(vty, "  RF Nominal Power: %d dBm, reduced by %u dB, "
		"resulting BS power: %d dBm%s",
		trx->nominal_power, trx->max_power_red,
		trx->nominal_power - trx->max_power_red, VTY_NEWLINE);
	vty_out(vty, "  NM State: ");
	net_dump_nmstate(vty, &trx->mo.nm_state);
	vty_out(vty, "  RSL State: %s%s", trx->rsl_link? "connected" : "disconnected", VTY_NEWLINE);
	vty_out(vty, "  Baseband Transceiver NM State: ");
	net_dump_nmstate(vty, &trx->bb_transc.mo.nm_state);
	vty_out(vty, "  IPA stream ID: 0x%02x%s", trx->rsl_tei, VTY_NEWLINE);
}

static inline void print_all_trx(struct vty *vty, const struct gsm_bts *bts)
{
	uint8_t trx_nr;
	for (trx_nr = 0; trx_nr < bts->num_trx; trx_nr++)
		trx_dump_vty(vty, gsm_bts_trx_num(bts, trx_nr));
}

DEFUN(show_trx,
      show_trx_cmd,
      "show trx [<0-255>] [<0-255>]",
	SHOW_STR "Display information about a TRX\n"
	BTS_TRX_STR)
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_bts *bts = NULL;
	int bts_nr, trx_nr;

	if (argc >= 1) {
		/* use the BTS number that the user has specified */
		bts_nr = atoi(argv[0]);
		if (bts_nr >= net->num_bts) {
			vty_out(vty, "%% can't find BTS '%s'%s", argv[0],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		bts = gsm_bts_num(net, bts_nr);
	}
	if (argc >= 2) {
		trx_nr = atoi(argv[1]);
		if (trx_nr >= bts->num_trx) {
			vty_out(vty, "%% can't find TRX '%s'%s", argv[1],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		trx_dump_vty(vty, gsm_bts_trx_num(bts, trx_nr));
		return CMD_SUCCESS;
	}
	if (bts) {
		/* print all TRX in this BTS */
		print_all_trx(vty, bts);
		return CMD_SUCCESS;
	}

	for (bts_nr = 0; bts_nr < net->num_bts; bts_nr++)
		print_all_trx(vty, gsm_bts_num(net, bts_nr));

	return CMD_SUCCESS;
}


static void ts_dump_vty(struct vty *vty, struct gsm_bts_trx_ts *ts)
{
	vty_out(vty, "BTS %u, TRX %u, Timeslot %u, phys cfg %s, TSC %u",
		ts->trx->bts->nr, ts->trx->nr, ts->nr,
		gsm_pchan_name(ts->pchan), gsm_ts_tsc(ts));
	if (ts->pchan == GSM_PCHAN_TCH_F_PDCH)
		vty_out(vty, " (%s mode)",
			ts->flags & TS_F_PDCH_ACTIVE ? "PDCH" : "TCH/F");
	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "  NM State: ");
	net_dump_nmstate(vty, &ts->mo.nm_state);
}

DEFUN(show_ts,
      show_ts_cmd,
      "show timeslot [<0-255>] [<0-255>] [<0-7>]",
	SHOW_STR "Display information about a TS\n"
	BTS_TRX_TS_STR)
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_bts *bts = NULL;
	struct gsm_bts_trx *trx = NULL;
	struct gsm_bts_trx_ts *ts = NULL;
	int bts_nr, trx_nr, ts_nr;

	if (argc >= 1) {
		/* use the BTS number that the user has specified */
		bts_nr = atoi(argv[0]);
		if (bts_nr >= net->num_bts) {
			vty_out(vty, "%% can't find BTS '%s'%s", argv[0],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		bts = gsm_bts_num(net, bts_nr);
	}
	if (argc >= 2) {
		trx_nr = atoi(argv[1]);
		if (trx_nr >= bts->num_trx) {
			vty_out(vty, "%% can't find TRX '%s'%s", argv[1],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		trx = gsm_bts_trx_num(bts, trx_nr);
	}
	if (argc >= 3) {
		ts_nr = atoi(argv[2]);
		if (ts_nr >= TRX_NR_TS) {
			vty_out(vty, "%% can't find TS '%s'%s", argv[2],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		/* Fully Specified: print and exit */
		ts = &trx->ts[ts_nr];
		ts_dump_vty(vty, ts);
		return CMD_SUCCESS;
	}

	if (bts && trx) {
		/* Iterate over all TS in this TRX */
		for (ts_nr = 0; ts_nr < TRX_NR_TS; ts_nr++) {
			ts = &trx->ts[ts_nr];
			ts_dump_vty(vty, ts);
		}
	} else if (bts) {
		/* Iterate over all TRX in this BTS, TS in each TRX */
		for (trx_nr = 0; trx_nr < bts->num_trx; trx_nr++) {
			trx = gsm_bts_trx_num(bts, trx_nr);
			for (ts_nr = 0; ts_nr < TRX_NR_TS; ts_nr++) {
				ts = &trx->ts[ts_nr];
				ts_dump_vty(vty, ts);
			}
		}
	} else {
		/* Iterate over all BTS, TRX in each BTS, TS in each TRX */
		for (bts_nr = 0; bts_nr < net->num_bts; bts_nr++) {
			bts = gsm_bts_num(net, bts_nr);
			for (trx_nr = 0; trx_nr < bts->num_trx; trx_nr++) {
				trx = gsm_bts_trx_num(bts, trx_nr);
				for (ts_nr = 0; ts_nr < TRX_NR_TS; ts_nr++) {
					ts = &trx->ts[ts_nr];
					ts_dump_vty(vty, ts);
				}
			}
		}
	}

	return CMD_SUCCESS;
}

/* FIXME: move this to libosmogsm */
static const struct value_string gsm48_cmode_names[] = {
	{ GSM48_CMODE_SIGN,		"signalling" },
	{ GSM48_CMODE_SPEECH_V1,	"FR or HR" },
	{ GSM48_CMODE_SPEECH_EFR,	"EFR" },
	{ GSM48_CMODE_SPEECH_AMR,	"AMR" },
	{ GSM48_CMODE_DATA_14k5,	"CSD(14k5)" },
	{ GSM48_CMODE_DATA_12k0,	"CSD(12k0)" },
	{ GSM48_CMODE_DATA_6k0,		"CSD(6k0)" },
	{ GSM48_CMODE_DATA_3k6,		"CSD(3k6)" },
	{ 0, NULL }
};

/* call vty_out() to print a string like " as TCH/H" for dynamic timeslots.
 * Don't do anything if the ts is not dynamic. */
static void vty_out_dyn_ts_status(struct vty *vty, struct gsm_bts_trx_ts *ts)
{
	switch (ts->pchan) {
	case GSM_PCHAN_TCH_F_TCH_H_PDCH:
		if (ts->dyn.pchan_is == ts->dyn.pchan_want)
			vty_out(vty, " as %s",
				gsm_pchan_name(ts->dyn.pchan_is));
		else
			vty_out(vty, " switching %s -> %s",
				gsm_pchan_name(ts->dyn.pchan_is),
				gsm_pchan_name(ts->dyn.pchan_want));
		break;
	case GSM_PCHAN_TCH_F_PDCH:
		if ((ts->flags & TS_F_PDCH_PENDING_MASK) == 0)
			vty_out(vty, " as %s",
				(ts->flags & TS_F_PDCH_ACTIVE)? "PDCH"
							      : "TCH/F");
		else
			vty_out(vty, " switching %s -> %s",
				(ts->flags & TS_F_PDCH_ACTIVE)? "PDCH"
							      : "TCH/F",
				(ts->flags & TS_F_PDCH_ACT_PENDING)? "PDCH"
								   : "TCH/F");
		break;
	default:
		/* no dyn ts */
		break;
	}
}

static void lchan_dump_full_vty(struct vty *vty, struct gsm_lchan *lchan)
{
	struct in_addr ia;

	vty_out(vty, "BTS %u, TRX %u, Timeslot %u, Lchan %u: Type %s%s",
		lchan->ts->trx->bts->nr, lchan->ts->trx->nr, lchan->ts->nr,
		lchan->nr, gsm_lchant_name(lchan->type), VTY_NEWLINE);
	/* show dyn TS details, if applicable */
	switch (lchan->ts->pchan) {
	case GSM_PCHAN_TCH_F_TCH_H_PDCH:
		vty_out(vty, "  Osmocom Dyn TS:");
		vty_out_dyn_ts_status(vty, lchan->ts);
		vty_out(vty, VTY_NEWLINE);
		break;
	case GSM_PCHAN_TCH_F_PDCH:
		vty_out(vty, "  IPACC Dyn PDCH TS:");
		vty_out_dyn_ts_status(vty, lchan->ts);
		vty_out(vty, VTY_NEWLINE);
		break;
	default:
		/* no dyn ts */
		break;
	}
	vty_out(vty, "  State: %s%s%s%s",
		gsm_lchans_name(lchan->state),
		lchan->state == LCHAN_S_BROKEN ? " Error reason: " : "",
		lchan->state == LCHAN_S_BROKEN ? lchan->broken_reason : "",
		VTY_NEWLINE);
	vty_out(vty, "  BS Power: %d dBm, MS Power: %u dBm%s",
		lchan->ts->trx->nominal_power - lchan->ts->trx->max_power_red
		- lchan->bs_power*2,
		ms_pwr_dbm(lchan->ts->trx->bts->band, lchan->ms_power),
		VTY_NEWLINE);
	vty_out(vty, "  Channel Mode / Codec: %s%s",
		get_value_string(gsm48_cmode_names, lchan->tch_mode),
		VTY_NEWLINE);

	if (lchan->abis_ip.bound_ip) {
		ia.s_addr = htonl(lchan->abis_ip.bound_ip);
		vty_out(vty, "  Bound IP: %s Port %u RTP_TYPE2=%u CONN_ID=%u%s",
			inet_ntoa(ia), lchan->abis_ip.bound_port,
			lchan->abis_ip.rtp_payload2, lchan->abis_ip.conn_id,
			VTY_NEWLINE);
	}
	if (lchan->abis_ip.connect_ip) {
		ia.s_addr = htonl(lchan->abis_ip.connect_ip);
		vty_out(vty, "  Conn. IP: %s Port %u RTP_TYPE=%u SPEECH_MODE=0x%02u%s",
			inet_ntoa(ia), lchan->abis_ip.connect_port,
			lchan->abis_ip.rtp_payload, lchan->abis_ip.speech_mode,
			VTY_NEWLINE);
	}
#define LAPDM_ESTABLISHED(link, sapi_idx) \
		(link).datalink[sapi_idx].dl.state == LAPD_STATE_MF_EST
	vty_out(vty, "  LAPDm SAPIs: DCCH %c%c, SACCH %c%c%s",
		LAPDM_ESTABLISHED(lchan->lapdm_ch.lapdm_dcch, DL_SAPI0) ? '0' : '-',
		LAPDM_ESTABLISHED(lchan->lapdm_ch.lapdm_dcch, DL_SAPI3) ? '3' : '-',
		LAPDM_ESTABLISHED(lchan->lapdm_ch.lapdm_acch, DL_SAPI0) ? '0' : '-',
		LAPDM_ESTABLISHED(lchan->lapdm_ch.lapdm_acch, DL_SAPI3) ? '3' : '-',
		VTY_NEWLINE);
#undef LAPDM_ESTABLISHED
	vty_out(vty, "  Valid System Information: 0x%08x%s",
		lchan->si.valid, VTY_NEWLINE);
	/* TODO: L1 SAPI (but those are BTS speific :( */
	/* TODO: AMR bits */
	vty_out(vty, "  MS Timing Offset: %d, propagation delay: %d symbols %s",
		lchan->ms_t_offs, lchan->p_offs, VTY_NEWLINE);
	if (lchan->encr.alg_id) {
		vty_out(vty, "  Ciphering A5/%u State: %s, N(S)=%u%s",
			lchan->encr.alg_id-1, lchan_ciph_state_name(lchan->ciph_state),
			lchan->ciph_ns, VTY_NEWLINE);
	}
	if (lchan->loopback)
		vty_out(vty, "  RTP/PDCH Loopback Enabled%s", VTY_NEWLINE);
	vty_out(vty, "  Radio Link Failure Counter 'S': %d%s", lchan->s, VTY_NEWLINE);
	/* TODO: MS Power Control */
}

static void lchan_dump_short_vty(struct vty *vty, struct gsm_lchan *lchan)
{
	const struct gsm_meas_rep_unidir *mru = &lchan->meas.ul_res;

	vty_out(vty, "BTS %u, TRX %u, Timeslot %u %s",
		lchan->ts->trx->bts->nr, lchan->ts->trx->nr, lchan->ts->nr,
		gsm_pchan_name(lchan->ts->pchan));
	vty_out_dyn_ts_status(vty, lchan->ts);
	vty_out(vty, ", Lchan %u, Type %s, State %s - "
		"RXL-FULL-ul: %4d dBm%s",
		lchan->nr,
		gsm_lchant_name(lchan->type), gsm_lchans_name(lchan->state),
		rxlev2dbm(mru->full.rx_lev),
		VTY_NEWLINE);
}

static int dump_lchan_trx_ts(struct gsm_bts_trx_ts *ts, struct vty *vty,
			     void (*dump_cb)(struct vty *, struct gsm_lchan *))
{
	int lchan_nr;
	for (lchan_nr = 0; lchan_nr < TS_MAX_LCHAN; lchan_nr++) {
		struct gsm_lchan *lchan = &ts->lchan[lchan_nr];
		if (lchan->state == LCHAN_S_NONE)
			continue;
		dump_cb(vty, lchan);
	}

	return CMD_SUCCESS;
}

static int dump_lchan_trx(struct gsm_bts_trx *trx, struct vty *vty,
			  void (*dump_cb)(struct vty *, struct gsm_lchan *))
{
	int ts_nr;

	for (ts_nr = 0; ts_nr < TRX_NR_TS; ts_nr++) {
		struct gsm_bts_trx_ts *ts = &trx->ts[ts_nr];
		dump_lchan_trx_ts(ts, vty, dump_cb);
	}

	return CMD_SUCCESS;
}

static int dump_lchan_bts(struct gsm_bts *bts, struct vty *vty,
			  void (*dump_cb)(struct vty *, struct gsm_lchan *))
{
	int trx_nr;

	for (trx_nr = 0; trx_nr < bts->num_trx; trx_nr++) {
		struct gsm_bts_trx *trx = gsm_bts_trx_num(bts, trx_nr);
		dump_lchan_trx(trx, vty, dump_cb);
	}

	return CMD_SUCCESS;
}

static int lchan_summary(struct vty *vty, int argc, const char **argv,
			 void (*dump_cb)(struct vty *, struct gsm_lchan *))
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	struct gsm_lchan *lchan;
	int bts_nr, trx_nr, ts_nr, lchan_nr;

	if (argc >= 1) {
		/* use the BTS number that the user has specified */
		bts_nr = atoi(argv[0]);
		if (bts_nr >= net->num_bts) {
			vty_out(vty, "%% can't find BTS %s%s", argv[0],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		bts = gsm_bts_num(net, bts_nr);

		if (argc == 1)
			return dump_lchan_bts(bts, vty, dump_cb);
	}
	if (argc >= 2) {
		trx_nr = atoi(argv[1]);
		if (trx_nr >= bts->num_trx) {
			vty_out(vty, "%% can't find TRX %s%s", argv[1],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		trx = gsm_bts_trx_num(bts, trx_nr);

		if (argc == 2)
			return dump_lchan_trx(trx, vty, dump_cb);
	}
	if (argc >= 3) {
		ts_nr = atoi(argv[2]);
		if (ts_nr >= TRX_NR_TS) {
			vty_out(vty, "%% can't find TS %s%s", argv[2],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		ts = &trx->ts[ts_nr];

		if (argc == 3)
			return dump_lchan_trx_ts(ts, vty, dump_cb);
	}
	if (argc >= 4) {
		lchan_nr = atoi(argv[3]);
		if (lchan_nr >= TS_MAX_LCHAN) {
			vty_out(vty, "%% can't find LCHAN %s%s", argv[3],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		lchan = &ts->lchan[lchan_nr];
		dump_cb(vty, lchan);
		return CMD_SUCCESS;
	}

	for (bts_nr = 0; bts_nr < net->num_bts; bts_nr++) {
		bts = gsm_bts_num(net, bts_nr);
		dump_lchan_bts(bts, vty, dump_cb);
	}

	return CMD_SUCCESS;
}

DEFUN(show_lchan,
	show_lchan_cmd,
	"show lchan [<0-255>] [<0-255>] [<0-7>] [<0-7>]",
	SHOW_STR "Display information about a logical channel\n"
	BTS_TRX_TS_LCHAN_STR)
{
	return lchan_summary(vty, argc, argv, lchan_dump_full_vty);
}

DEFUN(show_lchan_summary,
	show_lchan_summary_cmd,
	"show lchan summary [<0-255>] [<0-255>] [<0-7>] [<0-7>]",
	SHOW_STR "Display information about a logical channel\n"
	"Short summary\n"
	BTS_TRX_TS_LCHAN_STR)
{
	return lchan_summary(vty, argc, argv, lchan_dump_short_vty);
}

static struct gsm_lchan *resolve_lchan(struct gsm_network *net,
					const char **argv, int idx)
{
	int bts_nr = atoi(argv[idx+0]);
	int trx_nr = atoi(argv[idx+1]);
	int ts_nr = atoi(argv[idx+2]);
	int lchan_nr = atoi(argv[idx+3]);
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;

	bts = gsm_bts_num(net, bts_nr);
	if (!bts)
		return NULL;

	trx = gsm_bts_trx_num(bts, trx_nr);
	if (!trx)
		return NULL;

	if (ts_nr >= ARRAY_SIZE(trx->ts))
		return NULL;
	ts = &trx->ts[ts_nr];

	if (lchan_nr >= ARRAY_SIZE(ts->lchan))
		return NULL;

	return &ts->lchan[lchan_nr];
}

#define BTS_T_T_L_STR			\
	"BTS related commands\n"	\
	"BTS number\n"			\
	"TRX related commands\n"	\
	"TRX number\n"			\
	"timeslot related commands\n"	\
	"timeslot number\n"		\
	"logical channel commands\n"	\
	"logical channel number\n"

DEFUN(cfg_trx_gsmtap_sapi, cfg_trx_gsmtap_sapi_cmd,
	"HIDDEN", "HIDDEN")
{
	int sapi;

	sapi = get_string_value(gsmtap_sapi_names, argv[0]);
	OSMO_ASSERT(sapi >= 0);

	if (sapi == GSMTAP_CHANNEL_ACCH)
		gsmtap_sapi_acch = 1;
	else
		gsmtap_sapi_mask |= (1 << sapi);

	return CMD_SUCCESS;
}

DEFUN(cfg_trx_no_gsmtap_sapi, cfg_trx_no_gsmtap_sapi_cmd,
	"HIDDEN", "HIDDEN")
{
	int sapi;

	sapi = get_string_value(gsmtap_sapi_names, argv[0]);
	OSMO_ASSERT(sapi >= 0);

	if (sapi == GSMTAP_CHANNEL_ACCH)
		gsmtap_sapi_acch = 0;
	else
		gsmtap_sapi_mask &= ~(1 << sapi);

	return CMD_SUCCESS;
}

static struct cmd_node phy_node = {
	PHY_NODE,
	"%s(phy)# ",
	1,
};

static struct cmd_node phy_inst_node = {
	PHY_INST_NODE,
	"%s(phy-inst)# ",
	1,
};

DEFUN(cfg_phy, cfg_phy_cmd,
	"phy <0-255>",
	"Select a PHY to configure\n" "PHY number\n")
{
	int phy_nr = atoi(argv[0]);
	struct phy_link *plink;

	plink = phy_link_by_num(phy_nr);
	if (!plink)
		plink = phy_link_create(tall_bts_ctx, phy_nr);
	if (!plink)
		return CMD_WARNING;

	vty->index = plink;
	vty->index_sub = &plink->description;
	vty->node = PHY_NODE;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_inst, cfg_phy_inst_cmd,
	"instance <0-255>",
	"Select a PHY instance to configure\n" "PHY Instance number\n")
{
	int inst_nr = atoi(argv[0]);
	struct phy_link *plink = vty->index;
	struct phy_instance *pinst;

	pinst = phy_instance_by_num(plink, inst_nr);
	if (!pinst) {
		pinst = phy_instance_create(plink, inst_nr);
		if (!pinst) {
			vty_out(vty, "Unable to create phy%u instance %u%s",
				plink->num, inst_nr, VTY_NEWLINE);
			return CMD_WARNING;
		}
	}

	vty->index = pinst;
	vty->index_sub = &pinst->description;
	vty->node = PHY_INST_NODE;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_no_inst, cfg_phy_no_inst_cmd,
	"no instance <0-255>"
	NO_STR "Select a PHY instance to remove\n", "PHY Instance number\n")
{
	int inst_nr = atoi(argv[0]);
	struct phy_link *plink = vty->index;
	struct phy_instance *pinst;

	pinst = phy_instance_by_num(plink, inst_nr);
	if (!pinst) {
		vty_out(vty, "No such instance %u%s", inst_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}

	phy_instance_destroy(pinst);

	return CMD_SUCCESS;
}

#if 0
DEFUN(cfg_phy_type, cfg_phy_type_cmd,
	"type (sysmobts|osmo-trx|virtual)",
	"configure the type of the PHY\n"
	"sysmocom sysmoBTS PHY\n"
	"OsmoTRX based PHY\n"
	"Virtual PHY (GSMTAP based)\n")
{
	struct phy_link *plink = vty->index;

	if (plink->state != PHY_LINK_SHUTDOWN) {
		vty_out(vty, "Cannot change type of active PHY%s", VTY_NEWLINE);
		return CMD_WARNING;
	}

	if (!strcmp(argv[0], "sysmobts"))
		plink->type = PHY_LINK_T_SYSMOBTS;
	else if (!strcmp(argv[0], "osmo-trx"))
		plink->type = PHY_LINK_T_OSMOTRX;
	else if (!strcmp(argv[0], "virtual"))
		plink->type = PHY_LINK_T_VIRTUAL;
}
#endif

DEFUN(bts_t_t_l_jitter_buf,
	bts_t_t_l_jitter_buf_cmd,
	"bts <0-0> trx <0-0> ts <0-7> lchan <0-1> rtp jitter-buffer <0-10000>",
	BTS_T_T_L_STR "RTP settings\n"
	"Jitter buffer\n" "Size of jitter buffer in (ms)\n")
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_lchan *lchan;
	int jitbuf_ms = atoi(argv[4]), rc;

	lchan = resolve_lchan(net, argv, 0);
	if (!lchan) {
		vty_out(vty, "%% can't find BTS%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	if (!lchan->abis_ip.rtp_socket) {
		vty_out(vty, "%% this channel has no active RTP stream%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	rc = osmo_rtp_socket_set_param(lchan->abis_ip.rtp_socket,
				  lchan->ts->trx->bts->rtp_jitter_adaptive ?
				  OSMO_RTP_P_JIT_ADAP : OSMO_RTP_P_JITBUF,
				  jitbuf_ms);
	if (rc < 0)
		vty_out(vty, "%% error setting jitter parameters: %s%s",
			strerror(-rc), VTY_NEWLINE);
	else
		vty_out(vty, "%% jitter parameters set: %d%s", rc, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(bts_t_t_l_loopback,
	bts_t_t_l_loopback_cmd,
	"bts <0-0> trx <0-0> ts <0-7> lchan <0-1> loopback",
	BTS_T_T_L_STR "Set loopback\n")
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_lchan *lchan;

	lchan = resolve_lchan(net, argv, 0);
	if (!lchan) {
		vty_out(vty, "%% can't find BTS%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	lchan->loopback = 1;

	return CMD_SUCCESS;
}

DEFUN(no_bts_t_t_l_loopback,
	no_bts_t_t_l_loopback_cmd,
	"no bts <0-0> trx <0-0> ts <0-7> lchan <0-1> loopback",
	NO_STR BTS_T_T_L_STR "Set loopback\n")
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_lchan *lchan;

	lchan = resolve_lchan(net, argv, 0);
	if (!lchan) {
		vty_out(vty, "%% can't find BTS%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	lchan->loopback = 0;

	return CMD_SUCCESS;
}

int bts_vty_init(struct gsm_bts *bts, const struct log_info *cat)
{
	cfg_trx_gsmtap_sapi_cmd.string = vty_cmd_string_from_valstr(bts, gsmtap_sapi_names,
						"gsmtap-sapi (",
						"|",")", VTY_DO_LOWER);
	cfg_trx_gsmtap_sapi_cmd.doc = vty_cmd_string_from_valstr(bts, gsmtap_sapi_names,
						"GSMTAP SAPI\n",
						"\n", "", 0);

	cfg_trx_no_gsmtap_sapi_cmd.string = vty_cmd_string_from_valstr(bts, gsmtap_sapi_names,
						"no gsmtap-sapi (",
						"|",")", VTY_DO_LOWER);
	cfg_trx_no_gsmtap_sapi_cmd.doc = vty_cmd_string_from_valstr(bts, gsmtap_sapi_names,
						NO_STR "GSMTAP SAPI\n",
						"\n", "", 0);

	install_element_ve(&show_bts_cmd);
	install_element_ve(&show_trx_cmd);
	install_element_ve(&show_ts_cmd);
	install_element_ve(&show_lchan_cmd);
	install_element_ve(&show_lchan_summary_cmd);

	logging_vty_add_cmds(cat);
	osmo_talloc_vty_add_cmds();

	install_node(&bts_node, config_write_bts);
	install_element(CONFIG_NODE, &cfg_bts_cmd);
	install_element(CONFIG_NODE, &cfg_vty_telnet_port_cmd);
	install_element(BTS_NODE, &cfg_bts_unit_id_cmd);
	install_element(BTS_NODE, &cfg_bts_oml_ip_cmd);
	install_element(BTS_NODE, &cfg_bts_rtp_bind_ip_cmd);
	install_element(BTS_NODE, &cfg_bts_rtp_jitbuf_cmd);
	install_element(BTS_NODE, &cfg_bts_rtp_port_range_cmd);
	install_element(BTS_NODE, &cfg_bts_band_cmd);
	install_element(BTS_NODE, &cfg_description_cmd);
	install_element(BTS_NODE, &cfg_no_description_cmd);
	install_element(BTS_NODE, &cfg_bts_paging_queue_size_cmd);
	install_element(BTS_NODE, &cfg_bts_paging_lifetime_cmd);
	install_element(BTS_NODE, &cfg_bts_agch_queue_mgmt_default_cmd);
	install_element(BTS_NODE, &cfg_bts_agch_queue_mgmt_params_cmd);
	install_element(BTS_NODE, &cfg_bts_ul_power_target_cmd);
	install_element(BTS_NODE, &cfg_bts_min_qual_rach_cmd);
	install_element(BTS_NODE, &cfg_bts_min_qual_norm_cmd);
	install_element(BTS_NODE, &cfg_bts_max_ber_rach_cmd);
	install_element(BTS_NODE, &cfg_bts_pcu_sock_cmd);
	install_element(BTS_NODE, &cfg_bts_supp_meas_toa256_cmd);
	install_element(BTS_NODE, &cfg_bts_no_supp_meas_toa256_cmd);

	install_element(BTS_NODE, &cfg_trx_gsmtap_sapi_cmd);
	install_element(BTS_NODE, &cfg_trx_no_gsmtap_sapi_cmd);

	/* add and link to TRX config node */
	install_element(BTS_NODE, &cfg_bts_trx_cmd);
	install_node(&trx_node, config_write_dummy);

	install_element(TRX_NODE, &cfg_trx_user_gain_cmd);
	install_element(TRX_NODE, &cfg_trx_pr_max_initial_cmd);
	install_element(TRX_NODE, &cfg_trx_pr_step_size_cmd);
	install_element(TRX_NODE, &cfg_trx_pr_step_interval_cmd);
	install_element(TRX_NODE, &cfg_trx_ms_power_control_cmd);
	install_element(TRX_NODE, &cfg_trx_phy_cmd);

	install_element(ENABLE_NODE, &bts_t_t_l_jitter_buf_cmd);
	install_element(ENABLE_NODE, &bts_t_t_l_loopback_cmd);
	install_element(ENABLE_NODE, &no_bts_t_t_l_loopback_cmd);

	install_element(CONFIG_NODE, &cfg_phy_cmd);
	install_node(&phy_node, config_write_phy);
	install_element(PHY_NODE, &cfg_phy_inst_cmd);
	install_element(PHY_NODE, &cfg_phy_no_inst_cmd);

	install_node(&phy_inst_node, config_write_dummy);

	return 0;
}
