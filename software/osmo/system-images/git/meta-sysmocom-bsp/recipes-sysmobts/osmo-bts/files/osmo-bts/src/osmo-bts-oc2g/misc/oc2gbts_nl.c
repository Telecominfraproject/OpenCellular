/* Helper for netlink */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_nl.c
 *     (C) 2014 by Holger Hans Peter Freyther
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

#include <arpa/inet.h>
#include <netinet/ip.h>

#include <sys/socket.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NLMSG_TAIL(nmsg) \
        ((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

/**
 * In case one binds to 0.0.0.0/INADDR_ANY and wants to know which source
 * address will be used when sending a message this function can be used.
 * It will ask the routing code of the kernel for the PREFSRC
 */
int source_for_dest(const struct in_addr *dest, struct in_addr *loc_source)
{
	int fd, rc;
	struct rtmsg *r;
	struct rtattr *rta;
        struct {
		struct nlmsghdr         n;
		struct rtmsg            r;
		char                    buf[1024];
        } req;

	memset(&req, 0, sizeof(req));

	fd = socket(AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC, NETLINK_ROUTE);
	if (fd < 0) {
		perror("nl socket");
		return -1;
	}

	/* Send a rtmsg and ask for a response */
        req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
        req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
        req.n.nlmsg_type = RTM_GETROUTE;
        req.n.nlmsg_seq = 1;

	/* Prepare the routing request */
        req.r.rtm_family = AF_INET;

	/* set the dest */
	rta = NLMSG_TAIL(&req.n);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(*dest));
	memcpy(RTA_DATA(rta), dest, sizeof(*dest));

	/* update sizes for dest */
	req.r.rtm_dst_len = sizeof(*dest) * 8;
	req.n.nlmsg_len = NLMSG_ALIGN(req.n.nlmsg_len) + RTA_ALIGN(rta->rta_len);

	rc = send(fd, &req, req.n.nlmsg_len, 0);
	if (rc != req.n.nlmsg_len) {
		perror("short write");
		close(fd);
		return -2;
	}


	/* now receive a response and parse it */
	rc = recv(fd, &req, sizeof(req), 0);
	if (rc <= 0) {
		perror("short read");
		close(fd);
		return -3;
	}

	if (!NLMSG_OK(&req.n, rc) || req.n.nlmsg_type != RTM_NEWROUTE) {
		close(fd);
		return -4;
	}

	r = NLMSG_DATA(&req.n);
	rc -= NLMSG_LENGTH(sizeof(*r));
	rta = RTM_RTA(r);
	while (RTA_OK(rta, rc)) {
		if (rta->rta_type != RTA_PREFSRC) {
			rta = RTA_NEXT(rta, rc);
			continue;
		}

		/* we are done */
		memcpy(loc_source, RTA_DATA(rta), RTA_PAYLOAD(rta));
		close(fd);
		return 0;
	}

	close(fd);
	return -5;
}
