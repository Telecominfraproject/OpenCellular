/* VTY interface for sysmoBTS */

/* (C) 2013 by Andreas Eversberg <jolly@eversberg.eu>
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
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

#include <arpa/inet.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/select.h>
#include <osmocom/core/bits.h>

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/misc.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/vty.h>
#include <osmo-bts/scheduler.h>

#include "l1_if.h"
#include "trx_if.h"
#include "loops.h"

#define OSMOTRX_STR	"OsmoTRX Transceiver configuration\n"

static struct gsm_bts *vty_bts;

DEFUN(show_transceiver, show_transceiver_cmd, "show transceiver",
	SHOW_STR "Display information about transceivers\n")
{
	struct gsm_bts *bts = vty_bts;
	struct gsm_bts_trx *trx;
	struct trx_l1h *l1h;

	if (!transceiver_available) {
		vty_out(vty, "transceiver is not connected%s", VTY_NEWLINE);
	} else {
		vty_out(vty, "transceiver is connected%s", VTY_NEWLINE);
	}

	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct phy_instance *pinst = trx_phy_instance(trx);
		l1h = pinst->u.osmotrx.hdl;
		vty_out(vty, "TRX %d%s", trx->nr, VTY_NEWLINE);
		vty_out(vty, " %s%s",
			(l1h->config.poweron) ? "poweron":"poweroff",
			VTY_NEWLINE);
		if (l1h->config.arfcn_valid)
			vty_out(vty, " arfcn  : %d%s%s",
				(l1h->config.arfcn & ~ARFCN_PCS),
				(l1h->config.arfcn & ARFCN_PCS) ? " (PCS)" : "",
				VTY_NEWLINE);
		else
			vty_out(vty, " arfcn  : undefined%s", VTY_NEWLINE);
		if (l1h->config.tsc_valid)
			vty_out(vty, " tsc    : %d%s", l1h->config.tsc,
				VTY_NEWLINE);
		else
			vty_out(vty, " tsc    : undefined%s", VTY_NEWLINE);
		if (l1h->config.bsic_valid)
			vty_out(vty, " bsic   : %d%s", l1h->config.bsic,
				VTY_NEWLINE);
		else
			vty_out(vty, " bisc   : undefined%s", VTY_NEWLINE);
	}

	return CMD_SUCCESS;
}


static void show_phy_inst_single(struct vty *vty, struct phy_instance *pinst)
{
	uint8_t tn;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	vty_out(vty, "PHY Instance %s%s",
		phy_instance_name(pinst), VTY_NEWLINE);

	if (l1h->config.rxgain_valid)
		vty_out(vty, " rx-gain        : %d dB%s",
			l1h->config.rxgain, VTY_NEWLINE);
	else
		vty_out(vty, " rx-gain        : undefined%s", VTY_NEWLINE);
	if (l1h->config.power_valid)
		vty_out(vty, " tx-attenuation : %d dB%s",
			l1h->config.power, VTY_NEWLINE);
	else
		vty_out(vty, " tx-attenuation : undefined%s", VTY_NEWLINE);
	if (l1h->config.maxdly_valid)
		vty_out(vty, " maxdly : %d%s", l1h->config.maxdly,
			VTY_NEWLINE);
	else
		vty_out(vty, " maxdly : undefined%s", VTY_NEWLINE);
	if (l1h->config.maxdlynb_valid)
		vty_out(vty, " maxdlynb : %d%s", l1h->config.maxdlynb,
			VTY_NEWLINE);
	else
		vty_out(vty, " maxdlynb : undefined%s", VTY_NEWLINE);
	for (tn = 0; tn < TRX_NR_TS; tn++) {
		if (!((1 << tn) & l1h->config.slotmask))
			vty_out(vty, " slot #%d: unsupported%s", tn,
				VTY_NEWLINE);
		else if (l1h->config.slottype_valid[tn])
			vty_out(vty, " slot #%d: type %d%s", tn,
				l1h->config.slottype[tn],
				VTY_NEWLINE);
		else
			vty_out(vty, " slot #%d: undefined%s", tn,
				VTY_NEWLINE);
	}
}

static void show_phy_single(struct vty *vty, struct phy_link *plink)
{
	struct phy_instance *pinst;

	vty_out(vty, "PHY %u%s", plink->num, VTY_NEWLINE);

	llist_for_each_entry(pinst, &plink->instances, list)
		show_phy_inst_single(vty, pinst);
}

DEFUN(show_phy, show_phy_cmd, "show phy",
	SHOW_STR  "Display information about the available PHYs")
{
	int i;

	for (i = 0; i < 255; i++) {
		struct phy_link *plink = phy_link_by_num(i);
		if (!plink)
			break;
		show_phy_single(vty, plink);
	}

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_ms_power_loop, cfg_phy_ms_power_loop_cmd,
	"osmotrx ms-power-loop <-127-127>", OSMOTRX_STR
	"Enable MS power control loop\nTarget RSSI value (transceiver specific, "
	"should be 6dB or more above noise floor)\n")
{
	struct phy_link *plink = vty->index;

	plink->u.osmotrx.trx_target_rssi = atoi(argv[0]);
	plink->u.osmotrx.trx_ms_power_loop = true;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_no_ms_power_loop, cfg_phy_no_ms_power_loop_cmd,
	"no osmotrx ms-power-loop",
	NO_STR OSMOTRX_STR "Disable MS power control loop\n")
{
	struct phy_link *plink = vty->index;

	plink->u.osmotrx.trx_ms_power_loop = false;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_timing_advance_loop, cfg_phy_timing_advance_loop_cmd,
	"osmotrx timing-advance-loop", OSMOTRX_STR
	"Enable timing advance control loop\n")
{
	struct phy_link *plink = vty->index;

	plink->u.osmotrx.trx_ta_loop = true;

	return CMD_SUCCESS;
}
DEFUN(cfg_phy_no_timing_advance_loop, cfg_phy_no_timing_advance_loop_cmd,
	"no osmotrx timing-advance-loop",
	NO_STR OSMOTRX_STR "Disable timing advance control loop\n")
{
	struct phy_link *plink = vty->index;

	plink->u.osmotrx.trx_ta_loop = false;

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_maxdly, cfg_phyinst_maxdly_cmd,
	"osmotrx maxdly <0-31>",
	OSMOTRX_STR
	"Set the maximum acceptable delay of an Access Burst (in GSM symbols)."
	" Access Burst is the first burst a mobile transmits in order to establish"
	" a connection and it is used to estimate Timing Advance (TA) which is"
	" then applied to Normal Bursts to compensate for signal delay due to"
	" distance. So changing this setting effectively changes maximum range of"
	" the cell, because if we receive an Access Burst with a delay higher than"
	" this value, it will be ignored and connection is dropped.\n"
	"GSM symbols (approx. 1.1km per symbol)\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.maxdly = atoi(argv[0]);
	l1h->config.maxdly_valid = 1;
	l1h->config.maxdly_sent = 0;
	l1if_provision_transceiver_trx(l1h);

	return CMD_SUCCESS;
}


DEFUN(cfg_phyinst_maxdlynb, cfg_phyinst_maxdlynb_cmd,
	"osmotrx maxdlynb <0-31>",
	OSMOTRX_STR
	"Set the maximum acceptable delay of a Normal Burst (in GSM symbols)."
	" USE FOR TESTING ONLY, DON'T CHANGE IN PRODUCTION USE!"
	" During normal operation, Normal Bursts delay are controled by a Timing"
	" Advance control loop and thus Normal Bursts arrive to a BTS with no more"
	" than a couple GSM symbols, which is already taken into account in osmo-trx."
	" So changing this setting will have no effect in production installations"
	" except increasing osmo-trx CPU load. This setting is only useful when"
	" testing with a transmitter which can't precisely synchronize to the BTS"
	" downlink signal, like e.g. R&S CMD57.\n"
	"GSM symbols (approx. 1.1km per symbol)\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.maxdlynb = atoi(argv[0]);
	l1h->config.maxdlynb_valid = 1;
	l1h->config.maxdlynb_sent = 0;
	l1if_provision_transceiver_trx(l1h);

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_slotmask, cfg_phyinst_slotmask_cmd,
	"slotmask (1|0) (1|0) (1|0) (1|0) (1|0) (1|0) (1|0) (1|0)",
	"Set the supported slots\n"
	"TS0 supported\nTS0 unsupported\nTS1 supported\nTS1 unsupported\n"
	"TS2 supported\nTS2 unsupported\nTS3 supported\nTS3 unsupported\n"
	"TS4 supported\nTS4 unsupported\nTS5 supported\nTS5 unsupported\n"
	"TS6 supported\nTS6 unsupported\nTS7 supported\nTS7 unsupported\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	uint8_t tn;

	l1h->config.slotmask = 0;
	for (tn = 0; tn < TRX_NR_TS; tn++)
		if (argv[tn][0] == '1')
			l1h->config.slotmask |= (1 << tn);

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_power_on, cfg_phyinst_power_on_cmd,
	"osmotrx power (on|off)",
	OSMOTRX_STR
	"Change TRX state\n"
	"Turn it ON or OFF\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	if (strcmp(argv[0], "on"))
		vty_out(vty, "OFF: %d%s", trx_if_cmd_poweroff(l1h), VTY_NEWLINE);
	else {
		vty_out(vty, "ON: %d%s", trx_if_cmd_poweron(l1h), VTY_NEWLINE);
	}

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_fn_advance, cfg_phy_fn_advance_cmd,
	"osmotrx fn-advance <0-30>",
	OSMOTRX_STR
	"Set the number of frames to be transmitted to transceiver in advance "
	"of current FN\n"
	"Advance in frames\n")
{
	struct phy_link *plink = vty->index;

	plink->u.osmotrx.clock_advance = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_rts_advance, cfg_phy_rts_advance_cmd,
	"osmotrx rts-advance <0-30>",
	OSMOTRX_STR
	"Set the number of frames to be requested (PCU) in advance of current "
	"FN. Do not change this, unless you have a good reason!\n"
	"Advance in frames\n")
{
	struct phy_link *plink = vty->index;

	plink->u.osmotrx.rts_advance = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_rxgain, cfg_phyinst_rxgain_cmd,
	"osmotrx rx-gain <0-50>",
	OSMOTRX_STR
	"Set the receiver gain in dB\n"
	"Gain in dB\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.rxgain = atoi(argv[0]);
	l1h->config.rxgain_valid = 1;
	l1h->config.rxgain_sent = 0;
	l1if_provision_transceiver_trx(l1h);

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_tx_atten, cfg_phyinst_tx_atten_cmd,
	"osmotrx tx-attenuation <0-50>",
	OSMOTRX_STR
	"Set the transmitter attenuation\n"
	"Fixed attenuation in dB, overriding OML\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.power = atoi(argv[0]);
	l1h->config.power_oml = 0;
	l1h->config.power_valid = 1;
	l1h->config.power_sent = 0;
	l1if_provision_transceiver_trx(l1h);

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_tx_atten_oml, cfg_phyinst_tx_atten_oml_cmd,
	"osmotrx tx-attenuation oml",
	OSMOTRX_STR
	"Set the transmitter attenuation\n"
	"Use NM_ATT_RF_MAXPOWR_R (max power reduction) from BSC via OML\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.power_oml = 1;
	l1h->config.power_valid = 1;
	l1h->config.power_sent = 0;
	l1if_provision_transceiver_trx(l1h);

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_no_rxgain, cfg_phyinst_no_rxgain_cmd,
	"no osmotrx rx-gain",
	NO_STR OSMOTRX_STR "Unset the receiver gain in dB\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.rxgain_valid = 0;

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_no_tx_atten, cfg_phyinst_no_tx_atten_cmd,
	"no osmotrx tx-attenuation",
	NO_STR OSMOTRX_STR "Unset the transmitter attenuation\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.power_valid = 0;

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_no_maxdly, cfg_phyinst_no_maxdly_cmd,
	"no osmotrx maxdly",
	NO_STR OSMOTRX_STR
	"Unset the maximum delay of GSM symbols\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.maxdly_valid = 0;

	return CMD_SUCCESS;
}

DEFUN(cfg_phyinst_no_maxdlynb, cfg_phyinst_no_maxdlynb_cmd,
	"no osmotrx maxdlynb",
	NO_STR OSMOTRX_STR
	"Unset the maximum delay of GSM symbols\n")
{
	struct phy_instance *pinst = vty->index;
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	l1h->config.maxdlynb_valid = 0;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_transc_ip, cfg_phy_transc_ip_cmd,
	"osmotrx ip HOST",
	OSMOTRX_STR
	"Set local and remote IP address\n"
	"IP address (for both OsmoBtsTrx and OsmoTRX)\n")
{
	struct phy_link *plink = vty->index;

	osmo_talloc_replace_string(plink, &plink->u.osmotrx.local_ip, argv[0]);
	osmo_talloc_replace_string(plink, &plink->u.osmotrx.remote_ip, argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_osmotrx_ip, cfg_phy_osmotrx_ip_cmd,
	"osmotrx ip (local|remote) A.B.C.D",
	OSMOTRX_STR
	"Set IP address\n" "Local IP address (BTS)\n"
	"Remote IP address (OsmoTRX)\n" "IP address\n")
{
	struct phy_link *plink = vty->index;

	if (!strcmp(argv[0], "local"))
		osmo_talloc_replace_string(plink, &plink->u.osmotrx.local_ip, argv[1]);
	else if (!strcmp(argv[0], "remote"))
		osmo_talloc_replace_string(plink, &plink->u.osmotrx.remote_ip, argv[1]);
	else
		return CMD_WARNING;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_base_port, cfg_phy_base_port_cmd,
	"osmotrx base-port (local|remote) <0-65535>",
	OSMOTRX_STR "Set base UDP port number\n" "Local UDP port\n"
	"Remote UDP port\n" "UDP base port number\n")
{
	struct phy_link *plink = vty->index;

	if (!strcmp(argv[0], "local"))
		plink->u.osmotrx.base_port_local = atoi(argv[1]);
	else
		plink->u.osmotrx.base_port_remote = atoi(argv[1]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_setbsic, cfg_phy_setbsic_cmd,
	"osmotrx legacy-setbsic", OSMOTRX_STR
	"Use SETBSIC to configure transceiver (use ONLY with OpenBTS Transceiver!)\n")
{
	struct phy_link *plink = vty->index;
	plink->u.osmotrx.use_legacy_setbsic = true;

	vty_out(vty, "%% You have enabled SETBSIC, which is not supported by OsmoTRX "
		"but only useful if you want to interface with legacy OpenBTS Transceivers%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_no_setbsic, cfg_phy_no_setbsic_cmd,
	"no osmotrx legacy-setbsic",
	NO_STR OSMOTRX_STR "Disable Legacy SETBSIC to configure transceiver\n")
{
	struct phy_link *plink = vty->index;
	plink->u.osmotrx.use_legacy_setbsic = false;

	return CMD_SUCCESS;
}

void bts_model_config_write_phy(struct vty *vty, struct phy_link *plink)
{
	if (plink->u.osmotrx.local_ip)
		vty_out(vty, " osmotrx ip local %s%s",
			plink->u.osmotrx.local_ip, VTY_NEWLINE);
	if (plink->u.osmotrx.remote_ip)
		vty_out(vty, " osmotrx ip remote %s%s",
			plink->u.osmotrx.remote_ip, VTY_NEWLINE);

	if (plink->u.osmotrx.trx_ms_power_loop)
		vty_out(vty, " osmotrx ms-power-loop %d%s", plink->u.osmotrx.trx_target_rssi, VTY_NEWLINE);
	else
		vty_out(vty, " no osmotrx ms-power-loop%s", VTY_NEWLINE);
	vty_out(vty, " %sosmotrx timing-advance-loop%s", (plink->u.osmotrx.trx_ta_loop) ? "" : "no ", VTY_NEWLINE);

	if (plink->u.osmotrx.base_port_local)
		vty_out(vty, " osmotrx base-port local %"PRIu16"%s",
			plink->u.osmotrx.base_port_local, VTY_NEWLINE);
	if (plink->u.osmotrx.base_port_remote)
		vty_out(vty, " osmotrx base-port remote %"PRIu16"%s",
			plink->u.osmotrx.base_port_remote, VTY_NEWLINE);

	vty_out(vty, " osmotrx fn-advance %d%s",
		plink->u.osmotrx.clock_advance, VTY_NEWLINE);
	vty_out(vty, " osmotrx rts-advance %d%s",
		plink->u.osmotrx.rts_advance, VTY_NEWLINE);

	if (plink->u.osmotrx.use_legacy_setbsic)
		vty_out(vty, " osmotrx legacy-setbsic%s", VTY_NEWLINE);
}

void bts_model_config_write_phy_inst(struct vty *vty, struct phy_instance *pinst)
{
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	if (l1h->config.rxgain_valid)
		vty_out(vty, "  osmotrx rx-gain %d%s",
			l1h->config.rxgain, VTY_NEWLINE);
	if (l1h->config.power_valid) {
		if (l1h->config.power_oml)
			vty_out(vty, "  osmotrx tx-attenuation oml%s", VTY_NEWLINE);
		else
			vty_out(vty, "  osmotrx tx-attenuation %d%s",
				l1h->config.power, VTY_NEWLINE);
	}
	if (l1h->config.maxdly_valid)
		vty_out(vty, "  osmotrx maxdly %d%s", l1h->config.maxdly, VTY_NEWLINE);
	if (l1h->config.maxdlynb_valid)
		vty_out(vty, "  osmotrx maxdlynb %d%s", l1h->config.maxdlynb, VTY_NEWLINE);
	if (l1h->config.slotmask != 0xff)
		vty_out(vty, "  slotmask %d %d %d %d %d %d %d %d%s",
			l1h->config.slotmask & 1,
			(l1h->config.slotmask >> 1) & 1,
			(l1h->config.slotmask >> 2) & 1,
			(l1h->config.slotmask >> 3) & 1,
			(l1h->config.slotmask >> 4) & 1,
			(l1h->config.slotmask >> 5) & 1,
			(l1h->config.slotmask >> 6) & 1,
			l1h->config.slotmask >> 7,
			VTY_NEWLINE);
}

void bts_model_config_write_bts(struct vty *vty, struct gsm_bts *bts)
{
}

void bts_model_config_write_trx(struct vty *vty, struct gsm_bts_trx *trx)
{
}

int bts_model_vty_init(struct gsm_bts *bts)
{
	vty_bts = bts;

	install_element_ve(&show_transceiver_cmd);
	install_element_ve(&show_phy_cmd);

	install_element(PHY_NODE, &cfg_phy_ms_power_loop_cmd);
	install_element(PHY_NODE, &cfg_phy_no_ms_power_loop_cmd);
	install_element(PHY_NODE, &cfg_phy_timing_advance_loop_cmd);
	install_element(PHY_NODE, &cfg_phy_no_timing_advance_loop_cmd);
	install_element(PHY_NODE, &cfg_phy_base_port_cmd);
	install_element(PHY_NODE, &cfg_phy_fn_advance_cmd);
	install_element(PHY_NODE, &cfg_phy_rts_advance_cmd);
	install_element(PHY_NODE, &cfg_phy_transc_ip_cmd);
	install_element(PHY_NODE, &cfg_phy_osmotrx_ip_cmd);
	install_element(PHY_NODE, &cfg_phy_setbsic_cmd);
	install_element(PHY_NODE, &cfg_phy_no_setbsic_cmd);

	install_element(PHY_INST_NODE, &cfg_phyinst_rxgain_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_tx_atten_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_tx_atten_oml_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_no_rxgain_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_no_tx_atten_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_slotmask_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_power_on_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_maxdly_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_no_maxdly_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_maxdlynb_cmd);
	install_element(PHY_INST_NODE, &cfg_phyinst_no_maxdlynb_cmd);

	return 0;
}

int bts_model_ctrl_cmds_install(struct gsm_bts *bts)
{
	return 0;
}
