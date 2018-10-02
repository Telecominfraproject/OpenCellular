/* sysmobts - access to hardware related parameters */

/* (C) 2012 by Harald Welte <laforge@gnumonks.org>
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <osmocom/core/crc8gen.h>
#include <osmocom/core/utils.h>

#include "sysmobts_eeprom.h"
#include "sysmobts_par.h"
#include "eeprom.h"

#define EEPROM_PATH	"/sys/devices/platform/i2c_davinci.1/i2c-1/1-0050/eeprom"

static const struct osmo_crc8gen_code crc8_ccit = {
	.bits = 8,
	.poly = 0x83,
	.init = 0xFF,
	.remainder = 0x00,
};

const struct value_string sysmobts_par_names[_NUM_SYSMOBTS_PAR+1] = {
	{ SYSMOBTS_PAR_MAC,		"ethaddr" },
	{ SYSMOBTS_PAR_CLK_FACTORY,	"clk-factory" },
	{ SYSMOBTS_PAR_TEMP_DIG_MAX,	"temp-dig-max" },
	{ SYSMOBTS_PAR_TEMP_RF_MAX,	"temp-rf-max" },
	{ SYSMOBTS_PAR_SERNR,		"serial-nr" },
	{ SYSMOBTS_PAR_HOURS, 		"hours-running" },
	{ SYSMOBTS_PAR_BOOTS, 		"boot-count" },
	{ SYSMOBTS_PAR_KEY, 		"key" },
	{ SYSMOBTS_PAR_MODEL_NR, 	"model-nr" },
	{ SYSMOBTS_PAR_MODEL_FLAGS, 	"model-flags" },
	{ SYSMOBTS_PAR_TRX_NR, 		"trx-nr" },
	{ 0, NULL }
};

static struct {
	int read;
	struct sysmobts_eeprom ee;
} g_ee;

static struct sysmobts_eeprom *get_eeprom(int update_rqd)
{
	if (update_rqd || g_ee.read == 0) {
		int fd, rc;

		fd = open(EEPROM_PATH, O_RDONLY);
		if (fd < 0)
			return NULL;

		rc = read(fd, &g_ee.ee, sizeof(g_ee.ee));

		close(fd);

		if (rc < sizeof(g_ee.ee))
			return NULL;

		g_ee.read = 1;
	}

	return &g_ee.ee;
}

static int set_eeprom(struct sysmobts_eeprom *ee)
{
	int fd, rc;

	memcpy(&g_ee.ee, ee, sizeof(*ee));

	fd = open(EEPROM_PATH, O_WRONLY);
	if (fd < 0)
		return fd;

	rc = write(fd, ee, sizeof(*ee));
	if (rc < sizeof(*ee)) {
		close(fd);
		return -EIO;
	}

	close(fd);

	return 0;
}

int sysmobts_par_is_int(enum sysmobts_par par)
{
	switch (par) {
	case SYSMOBTS_PAR_CLK_FACTORY:
	case SYSMOBTS_PAR_TEMP_DIG_MAX:
	case SYSMOBTS_PAR_TEMP_RF_MAX:
	case SYSMOBTS_PAR_SERNR:
	case SYSMOBTS_PAR_HOURS:
	case SYSMOBTS_PAR_BOOTS:
	case SYSMOBTS_PAR_MODEL_NR:
	case SYSMOBTS_PAR_MODEL_FLAGS:
	case SYSMOBTS_PAR_TRX_NR:
		return 1;
	default:
		return 0;
	}
}

int sysmobts_par_get_int(enum sysmobts_par par, int *ret)
{
	eeprom_RfClockCal_t rf_clk;
	eeprom_Error_t err;
	struct sysmobts_eeprom *ee = get_eeprom(0);

	if (!ee)
		return -EIO;

	if (par >= _NUM_SYSMOBTS_PAR)
		return -ENODEV;

	switch (par) {
	case SYSMOBTS_PAR_CLK_FACTORY:
		err = eeprom_ReadRfClockCal(&rf_clk);
		if (err != EEPROM_SUCCESS)
			return -EIO;
		*ret = rf_clk.iClkCor;
		break;
	case SYSMOBTS_PAR_TEMP_DIG_MAX:
		*ret = ee->temp1_max;
		break;
	case SYSMOBTS_PAR_TEMP_RF_MAX:
		*ret = ee->temp2_max;
		break;
	case SYSMOBTS_PAR_SERNR:
		*ret = ee->serial_nr;
		break;
	case SYSMOBTS_PAR_HOURS:
		*ret = ee->operational_hours;
		break;
	case SYSMOBTS_PAR_BOOTS:
		*ret = ee->boot_count;
		break;
	case SYSMOBTS_PAR_MODEL_NR:
		*ret = ee->model_nr;
		break;
	case SYSMOBTS_PAR_MODEL_FLAGS:
		*ret = ee->model_flags;
		break;
	case SYSMOBTS_PAR_TRX_NR:
		*ret = ee->trx_nr;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int sysmobts_par_set_int(enum sysmobts_par par, int val)
{
	eeprom_RfClockCal_t rf_clk;
	eeprom_Error_t err;
	struct sysmobts_eeprom *ee = get_eeprom(1);

	if (!ee)
		return -EIO;

	if (par >= _NUM_SYSMOBTS_PAR)
		return -ENODEV;

	switch (par) {
	case SYSMOBTS_PAR_CLK_FACTORY:
		err = eeprom_ReadRfClockCal(&rf_clk);
		if (err != EEPROM_SUCCESS)
			return -EIO;
		rf_clk.iClkCor = val;
		err = eeprom_WriteRfClockCal(&rf_clk);
		if (err != EEPROM_SUCCESS)
			return -EIO;
		break;
	case SYSMOBTS_PAR_TEMP_DIG_MAX:
		ee->temp1_max = val;
		break;
	case SYSMOBTS_PAR_TEMP_RF_MAX:
		ee->temp2_max = val;
		break;
	case SYSMOBTS_PAR_SERNR:
		ee->serial_nr = val;
		break;
	case SYSMOBTS_PAR_HOURS:
		ee->operational_hours = val;
		break;
	case SYSMOBTS_PAR_BOOTS:
		ee->boot_count = val;
		break;
	case SYSMOBTS_PAR_MODEL_NR:
		ee->model_nr = val;
		break;
	case SYSMOBTS_PAR_MODEL_FLAGS:
		ee->model_flags = val;
		break;
	case SYSMOBTS_PAR_TRX_NR:
		ee->trx_nr = val;
		break;
	default:
		return -EINVAL;
	}

	set_eeprom(ee);

	return 0;
}

int sysmobts_par_get_buf(enum sysmobts_par par, uint8_t *buf,
			 unsigned int size)
{
	uint8_t *ptr;
	unsigned int len;
	struct sysmobts_eeprom *ee = get_eeprom(0);

	if (!ee)
		return -EIO;

	if (par >= _NUM_SYSMOBTS_PAR)
		return -ENODEV;

	switch (par) {
	case SYSMOBTS_PAR_MAC:
		ptr = ee->eth_mac;
		len = sizeof(ee->eth_mac);
		break;
	case SYSMOBTS_PAR_KEY:
		ptr = ee->gpg_key;
		len = sizeof(ee->gpg_key);
		break;
	default:
		return -EINVAL;
	}

	if (size < len)
		len = size;
	memcpy(buf, ptr, len);

	return len;
}

int sysmobts_par_set_buf(enum sysmobts_par par, const uint8_t *buf,
			 unsigned int size)
{
	uint8_t *ptr;
	unsigned int len;
	struct sysmobts_eeprom *ee = get_eeprom(0);

	if (!ee)
		return -EIO;

	if (par >= _NUM_SYSMOBTS_PAR)
		return -ENODEV;

	switch (par) {
	case SYSMOBTS_PAR_MAC:
		ptr = ee->eth_mac;
		len = sizeof(ee->eth_mac);
		break;
	case SYSMOBTS_PAR_KEY:
		ptr = ee->gpg_key;
		len = sizeof(ee->gpg_key);
		break;
	default:
		return -EINVAL;
	}

	if (len < size)
		size = len;

	memcpy(ptr, buf, size);

	return len;
}

int sysmobts_par_get_net(struct sysmobts_net_cfg *cfg)
{
	struct sysmobts_eeprom *ee = get_eeprom(0);
	ubit_t bits[sizeof(*cfg) * 8];
	uint8_t crc;
	int rc;

	if (!ee)
		return -EIO;

	/* convert the net_cfg to unpacked bits */
	rc = osmo_pbit2ubit(bits, (uint8_t *) &ee->net_cfg, sizeof(bits));
	if (rc != sizeof(bits))
		return -EFAULT;
	/* compute the crc and compare */
	crc = osmo_crc8gen_compute_bits(&crc8_ccit, bits, sizeof(bits));
	if (crc != ee->crc) {
		fprintf(stderr, "Computed CRC(%d) wanted CRC(%d)\n", crc, ee->crc);
		return -EBADMSG;
	}
	/* return the actual data */
	*cfg = ee->net_cfg;
	return 0;
}

int sysmobts_get_type(int *bts_type)
{
	return sysmobts_par_get_int(SYSMOBTS_PAR_MODEL_NR, bts_type);
}

int sysmobts_get_trx(int *trx_number)
{
	return sysmobts_par_get_int(SYSMOBTS_PAR_TRX_NR, trx_number);
}

char *sysmobts_model(int bts_type, int trx_num)
{
		switch(bts_type) {
		case 0:
		case 0xffff:
		case 1002:
			return "sysmoBTS 1002";
		case 2050:
			switch(trx_num) {
			case 0:
				return "sysmoBTS 2050 (master)";
			case 1:
				return "sysmoBTS 2050 (slave)";
			default:
				return "sysmoBTS 2050 (unknown)";
			}
		default:
			return "Unknown";
		}
}

int sysmobts_par_set_net(struct sysmobts_net_cfg *cfg)
{
	struct sysmobts_eeprom *ee = get_eeprom(1);
	ubit_t bits[sizeof(*cfg) * 8];
	int rc;

	if (!ee)
		return -EIO;

	/* convert the net_cfg to unpacked bits */
	rc = osmo_pbit2ubit(bits, (uint8_t *) cfg, sizeof(bits));
	if (rc != sizeof(bits))
		return -EFAULT;
	/* compute and store the result */
	ee->net_cfg = *cfg;
	ee->crc = osmo_crc8gen_compute_bits(&crc8_ccit, bits, sizeof(bits));
	return set_eeprom(ee);
}

osmo_static_assert(offsetof(struct sysmobts_eeprom, trx_nr) == 36, offset_36);
osmo_static_assert(offsetof(struct sysmobts_eeprom, boot_state) == 37, offset_37);
osmo_static_assert(offsetof(struct sysmobts_eeprom, _pad1) == 85, offset_85);
osmo_static_assert(offsetof(struct sysmobts_eeprom, net_cfg.mode) == 103, offset_103);
osmo_static_assert((offsetof(struct sysmobts_eeprom, net_cfg.ip) & 0x3)  == 0, ip_32bit_aligned);
osmo_static_assert(offsetof(struct sysmobts_eeprom, gpg_key) == 121, offset_121);
