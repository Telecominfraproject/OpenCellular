/* Interface handler for Sysmocom L1 (real hardware) */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
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

#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/select.h>
#include <osmocom/core/write_queue.h>
#include <osmocom/gsm/gsm_utils.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1prim.h>
#include <sysmocom/femtobts/gsml1const.h>
#include <sysmocom/femtobts/gsml1types.h>

#include "femtobts.h"
#include "l1_if.h"
#include "l1_transp.h"


#ifdef HW_SYSMOBTS_V1
#define DEV_SYS_DSP2ARM_NAME	"/dev/msgq/femtobts_dsp2arm"
#define DEV_SYS_ARM2DSP_NAME	"/dev/msgq/femtobts_arm2dsp"
#define DEV_L1_DSP2ARM_NAME	"/dev/msgq/gsml1_dsp2arm"
#define DEV_L1_ARM2DSP_NAME	"/dev/msgq/gsml1_arm2dsp"
#else
#define DEV_SYS_DSP2ARM_NAME	"/dev/msgq/superfemto_dsp2arm"
#define DEV_SYS_ARM2DSP_NAME	"/dev/msgq/superfemto_arm2dsp"
#define DEV_L1_DSP2ARM_NAME	"/dev/msgq/gsml1_sig_dsp2arm"
#define DEV_L1_ARM2DSP_NAME	"/dev/msgq/gsml1_sig_arm2dsp"

#define DEV_TCH_DSP2ARM_NAME	"/dev/msgq/gsml1_tch_dsp2arm"
#define DEV_TCH_ARM2DSP_NAME	"/dev/msgq/gsml1_tch_arm2dsp"
#define DEV_PDTCH_DSP2ARM_NAME	"/dev/msgq/gsml1_pdtch_dsp2arm"
#define DEV_PDTCH_ARM2DSP_NAME	"/dev/msgq/gsml1_pdtch_arm2dsp"
#endif

static const char *rd_devnames[] = {
	[MQ_SYS_READ]	= DEV_SYS_DSP2ARM_NAME,
	[MQ_L1_READ]	= DEV_L1_DSP2ARM_NAME,
#ifndef HW_SYSMOBTS_V1
	[MQ_TCH_READ]	= DEV_TCH_DSP2ARM_NAME,
	[MQ_PDTCH_READ]	= DEV_PDTCH_DSP2ARM_NAME,
#endif
};

static const char *wr_devnames[] = {
	[MQ_SYS_WRITE]	= DEV_SYS_ARM2DSP_NAME,
	[MQ_L1_WRITE]	= DEV_L1_ARM2DSP_NAME,
#ifndef HW_SYSMOBTS_V1
	[MQ_TCH_WRITE]	= DEV_TCH_ARM2DSP_NAME,
	[MQ_PDTCH_WRITE]= DEV_PDTCH_ARM2DSP_NAME,
#endif
};

/*
 * Make sure that all structs we read fit into the SYSMOBTS_PRIM_SIZE
 */
osmo_static_assert(sizeof(GsmL1_Prim_t) + 128 <= SYSMOBTS_PRIM_SIZE, l1_prim)
osmo_static_assert(sizeof(SuperFemto_Prim_t) + 128 <= SYSMOBTS_PRIM_SIZE, super_prim)

static int wqueue_vector_cb(struct osmo_fd *fd, unsigned int what)
{
	struct osmo_wqueue *queue;

	queue = container_of(fd, struct osmo_wqueue, bfd);

	if (what & BSC_FD_READ)
		queue->read_cb(fd);

	if (what & BSC_FD_EXCEPT)
		queue->except_cb(fd);

	if (what & BSC_FD_WRITE) {
		struct iovec iov[5];
		struct msgb *msg, *tmp;
		int written, count = 0;

		fd->when &= ~BSC_FD_WRITE;

		llist_for_each_entry(msg, &queue->msg_queue, list) {
			/* more writes than we have */
			if (count >= ARRAY_SIZE(iov))
				break;

			iov[count].iov_base = msg->l1h;
			iov[count].iov_len = msgb_l1len(msg);
			count += 1;
		}

		/* TODO: check if all lengths are the same. */


		/* Nothing scheduled? This should not happen. */
		if (count == 0) {
			if (!llist_empty(&queue->msg_queue))
				fd->when |= BSC_FD_WRITE;
			return 0;
		}

		written = writev(fd->fd, iov, count);
		if (written < 0) {
			/* nothing written?! */
			if (!llist_empty(&queue->msg_queue))
				fd->when |= BSC_FD_WRITE;
			return 0;
		}

		/* now delete the written entries */
		written = written / iov[0].iov_len;
		count = 0;
		llist_for_each_entry_safe(msg, tmp, &queue->msg_queue, list) {
			queue->current_length -= 1;

			llist_del(&msg->list);
			msgb_free(msg);

			count += 1;
			if (count >= written)
				break;
		}

		if (!llist_empty(&queue->msg_queue))
			fd->when |= BSC_FD_WRITE;
	}

	return 0;
}

static int prim_size_for_queue(int queue)
{
	switch (queue) {
	case MQ_SYS_WRITE:
		return sizeof(SuperFemto_Prim_t);
	case MQ_L1_WRITE:
#ifndef HW_SYSMOBTS_V1
	case MQ_TCH_WRITE:
	case MQ_PDTCH_WRITE:
#endif
		return sizeof(GsmL1_Prim_t);
	default:
		/* The compiler can't know that priv_nr is an enum. Assist. */
		LOGP(DL1C, LOGL_FATAL, "writing on a wrong queue: %d\n",
			queue);
		assert(false);
		break;
	}
}

/* callback when there's something to read from the l1 msg_queue */
static int read_dispatch_one(struct femtol1_hdl *fl1h, struct msgb *msg, int queue)
{
	switch (queue) {
	case MQ_SYS_WRITE:
		return l1if_handle_sysprim(fl1h, msg);
	case MQ_L1_WRITE:
#ifndef HW_SYSMOBTS_V1
	case MQ_TCH_WRITE:
	case MQ_PDTCH_WRITE:
#endif
		return l1if_handle_l1prim(queue, fl1h, msg);
	default:
		/* The compiler can't know that priv_nr is an enum. Assist. */
		LOGP(DL1C, LOGL_FATAL, "writing on a wrong queue: %d\n",
			queue);
		assert(false);
		break;
	}
};

static int l1if_fd_cb(struct osmo_fd *ofd, unsigned int what)
{
	int i, rc;

	const uint32_t prim_size = prim_size_for_queue(ofd->priv_nr);
	uint32_t count;

	struct iovec iov[3];
	struct msgb *msg[ARRAY_SIZE(iov)];

	for (i = 0; i < ARRAY_SIZE(iov); ++i) {
		msg[i] = msgb_alloc_headroom(prim_size + 128, 128, "1l_fd");
		msg[i]->l1h = msg[i]->data;

		iov[i].iov_base = msg[i]->l1h;
		iov[i].iov_len = msgb_tailroom(msg[i]);
	}

	rc = readv(ofd->fd, iov, ARRAY_SIZE(iov));
	if (rc < 0) {
		LOGP(DL1C, LOGL_ERROR, "failed to read from fd: %s\n", strerror(errno));
		/* N. B: we do not abort to let the cycle below cleanup allocated memory properly,
		   the return value is ignored by the caller anyway.
		   TODO: use libexplain's explain_readv() to provide detailed error description */
		count = 0;
	} else
		count = rc / prim_size;

	for (i = 0; i < count; ++i) {
		msgb_put(msg[i], prim_size);
		read_dispatch_one(ofd->data, msg[i], ofd->priv_nr);
	}

	for (i = count; i < ARRAY_SIZE(iov); ++i)
		msgb_free(msg[i]);

	return 1;
}

/* callback when we can write to one of the l1 msg_queue devices */
static int l1fd_write_cb(struct osmo_fd *ofd, struct msgb *msg)
{
	int rc;

	rc = write(ofd->fd, msg->l1h, msgb_l1len(msg));
	if (rc < 0) {
		LOGP(DL1C, LOGL_ERROR, "error writing to L1 msg_queue: %s\n",
			strerror(errno));
		return rc;
	} else if (rc < msg->len) {
		LOGP(DL1C, LOGL_ERROR, "short write to L1 msg_queue: "
			"%u < %u\n", rc, msg->len);
		return -EIO;
	}

	return 0;
}

int l1if_transport_open(int q, struct femtol1_hdl *hdl)
{
	int rc;

	/* Step 1: Open all msg_queue file descriptors */
	struct osmo_fd *read_ofd = &hdl->read_ofd[q];
	struct osmo_wqueue *wq = &hdl->write_q[q];
	struct osmo_fd *write_ofd = &hdl->write_q[q].bfd;

	rc = open(rd_devnames[q], O_RDONLY);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "[%d] unable to open %s for reading: %s\n",
		     q, rd_devnames[q], strerror(errno));
		return rc;
	}
	read_ofd->fd = rc;
	read_ofd->priv_nr = q;
	read_ofd->data = hdl;
	read_ofd->cb = l1if_fd_cb;
	read_ofd->when = BSC_FD_READ;
	rc = osmo_fd_register(read_ofd);
	if (rc < 0) {
		close(read_ofd->fd);
		read_ofd->fd = -1;
		return rc;
	}

	rc = open(wr_devnames[q], O_WRONLY);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "[%d] unable to open %s for writing: %s\n",
		     q, wr_devnames[q], strerror(errno));
		goto out_read;
	}
	osmo_wqueue_init(wq, 10);
	wq->write_cb = l1fd_write_cb;
	write_ofd->cb = wqueue_vector_cb;
	write_ofd->fd = rc;
	write_ofd->priv_nr = q;
	write_ofd->data = hdl;
	write_ofd->when = BSC_FD_WRITE;
	rc = osmo_fd_register(write_ofd);
	if (rc < 0) {
		close(write_ofd->fd);
		write_ofd->fd = -1;
		goto out_read;
	}

	return 0;

out_read:
	close(hdl->read_ofd[q].fd);
	osmo_fd_unregister(&hdl->read_ofd[q]);

	return rc;
}

int l1if_transport_close(int q, struct femtol1_hdl *hdl)
{
	struct osmo_fd *read_ofd = &hdl->read_ofd[q];
	struct osmo_fd *write_ofd = &hdl->write_q[q].bfd;

	osmo_fd_unregister(read_ofd);
	close(read_ofd->fd);
	read_ofd->fd = -1;

	osmo_fd_unregister(write_ofd);
	close(write_ofd->fd);
	write_ofd->fd = -1;

	return 0;
}
