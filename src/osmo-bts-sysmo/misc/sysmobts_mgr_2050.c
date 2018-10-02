/* (C) 2014 by s.f.m.c. GmbH
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

#include "sysmobts_misc.h"
#include "sysmobts_par.h"
#include "sysmobts_mgr.h"
#include "btsconfig.h"

#include <osmocom/core/logging.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/serial.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef BUILD_SBTS2050
#include <sysmocom/femtobts/sbts2050_header.h>

#define SERIAL_ALLOC_SIZE	300
#define SIZE_HEADER_RSP		5
#define SIZE_HEADER_CMD		4

struct uc {
	int id;
	int fd;
	const char *path;
};

struct ucinfo {
	uint16_t id;
	int master;
	int slave;
	int pa;
};

static struct uc ucontrol0 = {
	.id = 0,
	.path = "/dev/ttyS0",
	.fd = -1,
};

/**********************************************************************
 *	Functions read/write from serial interface
 *********************************************************************/
static int hand_serial_read(int fd, struct msgb *msg, int numbytes)
{
	int rc, bread = 0;

	if (numbytes > msgb_tailroom(msg))
		return -ENOSPC;

	while (bread < numbytes) {
		rc = read(fd, msg->tail, numbytes - bread);
		if (rc < 0)
			return -1;
		if (rc == 0)
			break;

		bread += rc;
		msgb_put(msg, rc);
	}

	return bread;
}

static int hand_serial_write(int fd, struct msgb *msg)
{
	int rc, bwritten = 0;

	while (msg->len > 0) {
		rc = write(fd, msg->data, msg->len);
		if (rc <= 0)
			return -1;

		msgb_pull(msg, rc);
		bwritten += rc;
	}

	return bwritten;
}

/**********************************************************************
 *	Functions request information to Microcontroller
 *********************************************************************/
static void add_parity(cmdpkt_t *command)
{
	int n;
	uint8_t parity = 0x00;
	for (n = 0; n < SIZE_HEADER_CMD+command->u8Len; n++)
		parity ^= ((uint8_t *)command)[n];

	command->cmd.raw[command->u8Len] = parity;
}

static struct msgb *sbts2050_ucinfo_sndrcv(struct uc *ucontrol, const struct ucinfo *info)
{
	int num, rc;
	cmdpkt_t *command;
	rsppkt_t *response;
	struct msgb *msg;
	fd_set fdread;
	struct timeval tout = {
		.tv_sec = 10,
	};

	switch (info->id) {
	case SBTS2050_TEMP_RQT:
		num = sizeof(command->cmd.tempGet);
		break;
	case SBTS2050_PWR_RQT:
		num = sizeof(command->cmd.pwrSetState);
		break;
	case SBTS2050_PWR_STATUS:
		num = sizeof(command->cmd.pwrGetStatus);
		break;
	default:
		return NULL;
	}
	num = num + SIZE_HEADER_CMD+1;

	msg = msgb_alloc(SERIAL_ALLOC_SIZE, "Message Microcontroller");
	if (msg == NULL) {
		LOGP(DTEMP, LOGL_ERROR, "Error creating msg\n");
		return NULL;
	}
	command = (cmdpkt_t *) msgb_put(msg, num);

	command->u16Magic = 0xCAFE;
	switch (info->id) {
	case SBTS2050_TEMP_RQT:
		command->u8Id = info->id;
		command->u8Len = sizeof(command->cmd.tempGet);
		break;
	case SBTS2050_PWR_RQT:
		command->u8Id = info->id;
		command->u8Len = sizeof(command->cmd.pwrSetState);
		command->cmd.pwrSetState.u1MasterEn = !!info->master;
		command->cmd.pwrSetState.u1SlaveEn  = !!info->slave;
		command->cmd.pwrSetState.u1PwrAmpEn = !!info->pa;
		break;
	case SBTS2050_PWR_STATUS:
		command->u8Id     = info->id;
		command->u8Len    = sizeof(command->cmd.pwrGetStatus);
		break;
	default:
		goto err;
	}

	add_parity(command);

	if (hand_serial_write(ucontrol->fd, msg) < 0)
		goto err;

	msgb_reset(msg);

	FD_ZERO(&fdread);
	FD_SET(ucontrol->fd, &fdread);

	num = SIZE_HEADER_RSP;
	while (1) {
		rc = select(ucontrol->fd+1, &fdread, NULL, NULL, &tout);
		if (rc > 0) {
			if (hand_serial_read(ucontrol->fd, msg, num) < 0)
				goto err;

			response = (rsppkt_t *)msg->data;

			if (response->u8Id != info->id || msg->len <= 0 ||
			    response->i8Error != RQT_SUCCESS)
				goto err;

			if (msg->len == SIZE_HEADER_RSP + response->u8Len + 1)
				break;

			num = response->u8Len + 1;
		} else
			goto err;
	}

	return msg;

err:
	msgb_free(msg);
	return NULL;
}

/**********************************************************************
 *	Get power status function
 *********************************************************************/
int sbts2050_uc_get_status(struct sbts2050_power_status *status)
{
	struct msgb *msg;
	const struct ucinfo info = {
		.id = SBTS2050_PWR_STATUS,
	};
	rsppkt_t *response;

	memset(status, 0, sizeof(*status));
	msg = sbts2050_ucinfo_sndrcv(&ucontrol0, &info);

	if (msg == NULL) {
		LOGP(DTEMP, LOGL_ERROR,
			"Error requesting power status.\n");
		return -1;
	}

	response = (rsppkt_t *)msg->data;

	status->main_supply_current = response->rsp.pwrGetStatus.u8MainSupplyA / 64.f;

	status->master_enabled = response->rsp.pwrGetStatus.u1MasterEn;
	status->master_voltage = response->rsp.pwrGetStatus.u8MasterV / 32.f;
	status->master_current = response->rsp.pwrGetStatus.u8MasterA / 64.f;;

	status->slave_enabled = response->rsp.pwrGetStatus.u1SlaveEn;
	status->slave_voltage = response->rsp.pwrGetStatus.u8SlaveV / 32.f;
	status->slave_current = response->rsp.pwrGetStatus.u8SlaveA / 64.f;

	status->pa_enabled = response->rsp.pwrGetStatus.u1PwrAmpEn;
	status->pa_voltage = response->rsp.pwrGetStatus.u8PwrAmpV / 4.f;
	status->pa_current = response->rsp.pwrGetStatus.u8PwrAmpA / 64.f;

	status->pa_bias_voltage = response->rsp.pwrGetStatus.u8PwrAmpBiasV / 16.f;

	msgb_free(msg);
	return 0;
}

/**********************************************************************
 *	Uc Power Switching handling
 *********************************************************************/
int sbts2050_uc_set_power(int pmaster, int pslave, int ppa)
{
	struct msgb *msg;
	const struct ucinfo info = {
		.id = SBTS2050_PWR_RQT,
		.master = pmaster,
		.slave = pslave,
		.pa = ppa
	};

	msg = sbts2050_ucinfo_sndrcv(&ucontrol0, &info);

	if (msg == NULL) {
		LOGP(DTEMP, LOGL_ERROR, "Error switching off some unit.\n");
		return -1;
	}

	LOGP(DTEMP, LOGL_DEBUG, "Switch off/on success:\n"
				"MASTER %s\n"
				"SLAVE %s\n"
				"PA %s\n",
				pmaster ? "ON" : "OFF",
				pslave ? "ON" : "OFF",
				ppa ? "ON" : "OFF");

	msgb_free(msg);
	return 0;
}

/**********************************************************************
 *	Uc temperature handling
 *********************************************************************/
int sbts2050_uc_check_temp(int *temp_pa, int *temp_board)
{
	rsppkt_t *response;
	struct msgb *msg;
	const struct ucinfo info = {
		.id = SBTS2050_TEMP_RQT,
	};

	msg = sbts2050_ucinfo_sndrcv(&ucontrol0, &info);

	if (msg == NULL) {
		LOGP(DTEMP, LOGL_ERROR, "Error reading temperature\n");
		return -1;
	}

	response = (rsppkt_t *)msg->data;

	*temp_board = response->rsp.tempGet.i8BrdTemp;
	*temp_pa = response->rsp.tempGet.i8PaTemp;

	LOGP(DTEMP, LOGL_DEBUG, "Temperature Board: %+3d C, "
				"Tempeture PA: %+3d C\n",
				 response->rsp.tempGet.i8BrdTemp,
				 response->rsp.tempGet.i8PaTemp);
	msgb_free(msg);
	return 0;
}

void sbts2050_uc_initialize(void)
{
	if (!is_sbts2050())
		return;

	ucontrol0.fd = osmo_serial_init(ucontrol0.path, 115200);
	if (ucontrol0.fd < 0) {
		LOGP(DTEMP, LOGL_ERROR,
		     "Failed to open the serial interface\n");
		return;
	}

	if (is_sbts2050_master()) {
		LOGP(DTEMP, LOGL_NOTICE, "Going to enable the PA.\n");
		sbts2050_uc_set_pa_power(1);
	}
}

int sbts2050_uc_set_pa_power(int on_off)
{
	struct sbts2050_power_status status;
	if (sbts2050_uc_get_status(&status) != 0) {
		LOGP(DTEMP, LOGL_ERROR, "Failed to read current power status.\n");
		return -1;
	}

	return sbts2050_uc_set_power(status.master_enabled, status.slave_enabled, on_off);
}

int sbts2050_uc_set_slave_power(int on_off)
{
	struct sbts2050_power_status status;
	if (sbts2050_uc_get_status(&status) != 0) {
		LOGP(DTEMP, LOGL_ERROR, "Failed to read current power status.\n");
		return -1;
	}

	return sbts2050_uc_set_power(
			status.master_enabled,
			on_off,
			status.pa_enabled);
}
#else
void sbts2050_uc_initialize(void)
{
	LOGP(DTEMP, LOGL_NOTICE, "sysmoBTS2050 was not enabled at compile time.\n");
}

int sbts2050_uc_check_temp(int *temp_pa, int *temp_board)
{
	LOGP(DTEMP, LOGL_ERROR, "sysmoBTS2050 compiled without temp support.\n");
	*temp_pa = *temp_board = 99999;
	return -1;
}

int sbts2050_uc_get_status(struct sbts2050_power_status *status)
{
	memset(status, 0, sizeof(*status));
	LOGP(DTEMP, LOGL_ERROR, "sysmoBTS2050 compiled without status support.\n");
	return -1;
}

int sbts2050_uc_set_pa_power(int on_off)
{
	LOGP(DTEMP, LOGL_ERROR, "sysmoBTS2050 compiled without PA support.\n");
	return -1;
}

int sbts2050_uc_set_slave_power(int on_off)
{
	LOGP(DTEMP, LOGL_ERROR, "sysmoBTS2050 compiled without UC support.\n");
	return -1;
}

#endif
