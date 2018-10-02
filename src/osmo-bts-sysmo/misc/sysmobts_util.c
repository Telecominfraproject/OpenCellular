/* sysmobts-util - access to hardware related parameters */

/* (C) 2012-2013 by Harald Welte <laforge@gnumonks.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sysmobts_par.h"
#include "sysmobts_eeprom.h"

enum act {
	ACT_GET,
	ACT_SET,
	ACT_NET_GET,
	ACT_NET_SET,
};

static enum act action;
static char *write_arg;
static int void_warranty;


static struct in_addr net_ip = { 0, }, net_dns = { 0, }, net_gw = { 0, }, net_mask = { 0, };
static uint8_t net_mode = 0;

static void print_help()
{
	const struct value_string *par = sysmobts_par_names;

	printf("sysmobts-util [--void-warranty -r | -w value] param_name\n");
	printf("sysmobts-util --net-read\n");
	printf("sysmobts-util --net-write --mode INT --ip IP_STR --gw IP_STR --dns IP_STR --net-mask IP_STR\n");
	printf("Possible param names:\n");

	for (; par->str != NULL; par += 1) {
		if (!sysmobts_par_is_int(par->value))
			continue;
		printf(" %s\n", par->str);
	}
}

static int parse_options(int argc, char **argv)
{
	while (1) {
		int option_idx = 0, c;
		static const struct option long_options[] = {
			{ "help", 0, 0, 'h' },
			{ "read", 0, 0, 'r' },
			{ "void-warranty", 0, 0, 1000},
			{ "write", 1, 0, 'w' },
			{ "ip",  1, 0, 241 },
			{ "gw",  1, 0, 242 },
			{ "dns", 1, 0, 243 },
			{ "net-mask", 1, 0, 244 },
			{ "mode", 1, 0, 245 },
			{ "net-read", 0, 0, 246 },
			{ "net-write", 0, 0, 247 },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "rw:h",
				long_options, &option_idx);
		if (c == -1)
			break;
		switch (c) {
		case 'r':
			action = ACT_GET;
			break;
		case 'w':
			action = ACT_SET;
			write_arg = optarg;
			break;
		case 'h':
			print_help();
			return -1;
			break;
		case 1000:
			printf("Will void warranty on write.\n");
			void_warranty = 1;
			break;
		case 246:
			action = ACT_NET_GET;
			break;
		case 247:
			action = ACT_NET_SET;
			break;
		case 245:
			net_mode = atoi(optarg);
			break;
		case 244:
			inet_aton(optarg, &net_mask);
			break;
		case 243:
			inet_aton(optarg, &net_dns);
			break;
		case 242:
			inet_aton(optarg, &net_gw);
			break;
		case 241:
			inet_aton(optarg, &net_ip);
			break;
		default:
			printf("Unknown option %d/%c\n", c, c);
			return -1;
		}
	}

	return 0;
}

static const char *make_addr(uint32_t saddr)
{
	struct in_addr addr;
	addr.s_addr = ntohl(saddr);
	return inet_ntoa(addr);
}

static void dump_net_cfg(struct sysmobts_net_cfg *net_cfg)
{
	if (net_cfg->mode == NET_MODE_DHCP) {
		printf("IP=dhcp\n");
		printf("DNS=\n");
		printf("GATEWAY=\n");
		printf("NETMASK=\n");
	} else {
		printf("IP=%s\n", make_addr(net_cfg->ip));
		printf("GATEWAY=%s\n", make_addr(net_cfg->gw));
		printf("DNS=%s\n", make_addr(net_cfg->dns));
		printf("NETMASK=%s\n", make_addr(net_cfg->mask));
	}
}

static int handle_net(void)
{
	struct sysmobts_net_cfg net_cfg;
	int rc;

	switch (action) {
	case ACT_NET_GET:
		rc = sysmobts_par_get_net(&net_cfg);
		if (rc != 0) {
			fprintf(stderr, "Error %d\n", rc);
			exit(rc);
		}
		dump_net_cfg(&net_cfg);
		break;
	case ACT_NET_SET:
		memset(&net_cfg, 0, sizeof(net_cfg));
		net_cfg.mode = net_mode;
		net_cfg.ip = htonl(net_ip.s_addr);
		net_cfg.mask = htonl(net_mask.s_addr);
		net_cfg.gw = htonl(net_gw.s_addr);
		net_cfg.dns = htonl(net_dns.s_addr);
		printf("Going to write\n");
		dump_net_cfg(&net_cfg);

		rc = sysmobts_par_set_net(&net_cfg);
		if (rc != 0) {
			fprintf(stderr, "Error %d\n", rc);
			exit(rc);
		}
		break;
	default:
		printf("Unhandled action %d\n", action);
	}
	return 0;
}

int main(int argc, char **argv)
{
	const char *parname;
	enum sysmobts_par par;
	int rc, val;

	rc = parse_options(argc, argv);
	if (rc < 0)
		exit(2);

	if (action > ACT_SET)
		return handle_net();

	if (optind >= argc && action <+ ACT_NET_GET) {
		fprintf(stderr, "You must specify the parameter name\n");
		exit(2);
	}
	parname = argv[optind];

	rc = get_string_value(sysmobts_par_names, parname);
	if (rc < 0) {
		fprintf(stderr, "`%s' is not a valid parameter\n", parname);
		exit(2);
	} else
		par = rc;

	switch (action) {
	case ACT_GET:
		rc = sysmobts_par_get_int(par, &val);
		if (rc < 0) {
			fprintf(stderr, "Error %d\n", rc);
			goto err;
		}
		printf("%d\n", val);
		break;
	case ACT_SET:
		rc = sysmobts_par_get_int(par, &val);
		if (rc < 0) {
			fprintf(stderr, "Error %d\n", rc);
			goto err;
		}
		if (val != 0xFFFF && val != 0xFF && val != 0xFFFFFFFF && !void_warranty) {
			fprintf(stderr, "Parameter is already set!\r\n");
			goto err;
		}
		rc = sysmobts_par_set_int(par, atoi(write_arg));
		if (rc < 0) {
			fprintf(stderr, "Error %d\n", rc);
			goto err;
		}
		printf("Success setting %s=%d\n", parname,
			atoi(write_arg));
		break;
	default:
		fprintf(stderr, "Unsupported action\n");
		goto err;
	}

	exit(0);

err:
	exit(1);
}

