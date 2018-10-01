/* Beginnings of an OML router */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     (C) 2014 by sysmocom s.f.m.c. GmbH
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

#include "oml_router.h"

#include <osmo-bts/bts.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/msg_utils.h>

#include <osmocom/core/socket.h>
#include <osmocom/core/select.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

static int oml_router_read_cb(struct osmo_fd *fd, unsigned int what)
{
	struct msgb *msg;
	int rc;

	msg = oml_msgb_alloc();
	if (!msg) {
		LOGP(DL1C, LOGL_ERROR, "Failed to allocate oml msgb.\n");
		return -1;
	}

	rc = recv(fd->fd, msg->tail, msg->data_len, 0);
	if (rc <= 0) {
		close(fd->fd);
		osmo_fd_unregister(fd);
		fd->fd = -1;
		goto err;
	}

	msg->l1h = msgb_put(msg, rc);
	rc = msg_verify_ipa_structure(msg);
	if (rc < 0) {
		LOGP(DL1C, LOGL_ERROR,
			"OML Router: Invalid IPA message rc(%d)\n", rc);
		goto err;
	}

	rc = msg_verify_oml_structure(msg);
	if (rc < 0) {
		LOGP(DL1C, LOGL_ERROR,
			"OML Router: Invalid OML message rc(%d)\n", rc);
		goto err;
	}

	/* todo dispatch message */

err:
	msgb_free(msg);
	return -1;
}

static int oml_router_accept_cb(struct osmo_fd *accept_fd, unsigned int what)
{
	int fd;
	struct osmo_fd *read_fd = (struct osmo_fd *) accept_fd->data;

	/* Accept only one connection at a time. De-register it */
	if (read_fd->fd > -1) {
		LOGP(DL1C, LOGL_NOTICE,
			"New OML router connection. Closing old one.\n");
		close(read_fd->fd);
		osmo_fd_unregister(read_fd);
		read_fd->fd = -1;
	}

	fd = accept(accept_fd->fd, NULL, NULL);
	if (fd < 0) {
		LOGP(DL1C, LOGL_ERROR, "Failed to accept. errno: %s.\n",
		     strerror(errno));
		return -1;
	}

	read_fd->fd = fd;
	if (osmo_fd_register(read_fd) != 0) {
		LOGP(DL1C, LOGL_ERROR, "Registering the read fd failed.\n");
		close(fd);
		read_fd->fd = -1;
		return -1;
	}

	return 0;
}

int oml_router_init(struct gsm_bts *bts, const char *path,
			struct osmo_fd *accept_fd, struct osmo_fd *read_fd)
{
	int rc;

	memset(accept_fd, 0, sizeof(*accept_fd));
	memset(read_fd, 0, sizeof(*read_fd));

	accept_fd->cb = oml_router_accept_cb;
	accept_fd->data = read_fd;

	read_fd->cb = oml_router_read_cb;
	read_fd->data = bts;
	read_fd->when = BSC_FD_READ;
	read_fd->fd = -1;

	rc = osmo_sock_unix_init_ofd(accept_fd, SOCK_SEQPACKET, 0,
					path,
					OSMO_SOCK_F_BIND | OSMO_SOCK_F_NONBLOCK);
	return rc;
}
