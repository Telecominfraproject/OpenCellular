/* Control Interface for osmo-bts */

/* (C) 2014 by Harald Welte <laforge@gnumonks.org>
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

#include <osmocom/vty/command.h>
#include <osmocom/ctrl/control_if.h>
#include <osmocom/ctrl/ports.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/control_if.h>

extern vector ctrl_node_vec;

/*! \brief control interface lookup function for bsc/bts gsm_data
 * \param[in] data Private data passed to controlif_setup()
 * \param[in] vline Vector of the line holding the command string
 * \param[out] node_type type (CTRL_NODE_) that was determined
 * \param[out] node_data private dta of node that was determined
 * \param i Current index into vline, up to which it is parsed
 */
static int bts_ctrl_node_lookup(void *data, vector vline, int *node_type,
				void **node_data, int *i)
{
	struct gsm_bts *bts = data;
	struct gsm_bts_trx *trx = NULL;
	struct gsm_bts_trx_ts *ts = NULL;
	char *token = vector_slot(vline, *i);
	long num;

	/* TODO: We need to make sure that the following chars are digits
	 * and/or use strtol to check if number conversion was successful
	 * Right now something like net.bts_stats will not work */
	if (!strcmp(token, "trx")) {
		if (*node_type != CTRL_NODE_ROOT || !*node_data)
			goto err_missing;
		bts = *node_data;
		(*i)++;
		if (!ctrl_parse_get_num(vline, *i, &num))
			goto err_index;

		trx = gsm_bts_trx_num(bts, num);
		if (!trx)
			goto err_missing;
		*node_data = trx;
		*node_type = CTRL_NODE_TRX;
	} else if (!strcmp(token, "ts")) {
		if (*node_type != CTRL_NODE_TRX || !*node_data)
			goto err_missing;
		trx = *node_data;
		(*i)++;
		if (!ctrl_parse_get_num(vline, *i, &num))
			goto err_index;

		if ((num >= 0) && (num < TRX_NR_TS))
			ts = &trx->ts[num];
		if (!ts)
			goto err_missing;
		*node_data = ts;
		*node_type = CTRL_NODE_TS;
	} else
		return 0;

	return 1;
err_missing:
	return -ENODEV;
err_index:
	return -ERANGE;
}

struct ctrl_handle *bts_controlif_setup(struct gsm_bts *bts,
					const char *bind_addr, uint16_t port)
{
	struct ctrl_handle *hdl;
	int rc = 0;

	hdl = ctrl_interface_setup_dynip(bts, bind_addr, port,
					 bts_ctrl_node_lookup);
	if (!hdl)
		return NULL;

	rc = bts_ctrl_cmds_install(bts);
	if (rc) {
		/* FIXME: close control interface */
		return NULL;
	}

	rc = bts_model_ctrl_cmds_install(bts);
	if (rc) {
		/* FIXME: cleanup generic control commands */
		/* FIXME: close control interface */
		return NULL;
	}

	return hdl;
}
