/* Copyright (C) 2016 by NuRAN Wireless <support@nuranwireless.com>
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

#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "lc15bts_mgr.h"
#include "lc15bts_bts.h"

static int check_eth_status(char *dev_name)
{
	int fd, rc;
	struct ifreq ifr;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0)
		return fd;

	memset(&ifr, 0, sizeof(ifr));
	memcpy(&ifr.ifr_name, dev_name, sizeof(ifr.ifr_name));
	rc = ioctl(fd, SIOCGIFFLAGS, &ifr);
	close(fd);

	if (rc < 0)
		return rc;

	if ((ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING))
		return 0;

	return 1;
}

void check_bts_led_pattern(uint8_t *led)
{
	FILE *fp;
	char str[64] =  "\0";
	int rc;

	/* check for existing of BTS state file */
	if ((fp = fopen("/var/run/osmo-bts/state", "r")) == NULL) {
		led[BLINK_PATTERN_INIT] = 1;
		return;
	}

	/* check Ethernet interface status */
	rc = check_eth_status("eth0");
	if (rc > 0) {
		LOGP(DTEMP, LOGL_DEBUG,"External link is DOWN\n");
		led[BLINK_PATTERN_EXT_LINK_MALFUNC] = 1;
		fclose(fp);
		return;
	}

	/* check for BTS is still alive */
	if (system("pidof osmo-bts-lc15 > /dev/null")) {
		LOGP(DTEMP, LOGL_DEBUG,"BTS process has stopped\n");
		led[BLINK_PATTERN_INT_PROC_MALFUNC] = 1;
		fclose(fp);
		return;
	}

	/* check for BTS state */
	while (fgets(str, 64, fp) != NULL) {
		LOGP(DTEMP, LOGL_DEBUG,"BTS state is %s\n", (strstr(str, "ABIS DOWN") != NULL) ? "DOWN" : "UP");
		if (strstr(str, "ABIS DOWN") != NULL)
			led[BLINK_PATTERN_INT_PROC_MALFUNC] = 1;
	}
	fclose(fp);

	return;
}

int check_sensor_led_pattern( struct lc15bts_mgr_instance *mgr, uint8_t *led)
{
	if(mgr->alarms.temp_high == 1)
		led[BLINK_PATTERN_TEMP_HIGH] = 1;

	if(mgr->alarms.temp_max == 1)
		led[BLINK_PATTERN_TEMP_MAX] = 1;

	if(mgr->alarms.supply_low == 1)
		led[BLINK_PATTERN_SUPPLY_VOLT_LOW] = 1;

	if(mgr->alarms.supply_min == 1)
		led[BLINK_PATTERN_SUPPLY_VOLT_MIN] = 1;

	if(mgr->alarms.vswr_high == 1)
		led[BLINK_PATTERN_VSWR_HIGH] = 1;

	if(mgr->alarms.vswr_max == 1)
		led[BLINK_PATTERN_VSWR_MAX] = 1;

	if(mgr->alarms.supply_pwr_high == 1)
		led[BLINK_PATTERN_SUPPLY_PWR_HIGH] = 1;

	if(mgr->alarms.supply_pwr_max == 1)
		led[BLINK_PATTERN_SUPPLY_PWR_MAX] = 1;

	if(mgr->alarms.supply_pwr_max2 == 1)
		led[BLINK_PATTERN_SUPPLY_PWR_MAX2] = 1;

	if(mgr->alarms.pa_pwr_high == 1)
		led[BLINK_PATTERN_PA_PWR_HIGH] = 1;

	if(mgr->alarms.pa_pwr_max == 1)
		led[BLINK_PATTERN_PA_PWR_MAX] = 1;

	if(mgr->alarms.gps_fix_lost == 1)
		led[BLINK_PATTERN_GPS_FIX_LOST] = 1;

	return 0;
}
