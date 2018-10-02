/* Layer1 handling for the DSP/FPGA */
/*
 * (C) 2012-2013 Holger Hans Peter Freyther
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1prim.h>

#include "sysmobts-layer1.h"

#define ARRAY_SIZE(ar)	(sizeof(ar)/sizeof((ar)[0]))

#define BTS_DSP2ARM    "/dev/msgq/superfemto_dsp2arm"
#define BTS_ARM2DSP    "/dev/msgq/superfemto_arm2dsp"
#define L1_SIG_ARM2DSP "/dev/msgq/gsml1_sig_arm2dsp"
#define L1_SIG_DSP2ARM "/dev/msgq/gsml1_sig_dsp2arm"

int set_clock_cor(int clock_cor, int calib, int source);
static int wait_read_ignore(int seconds);

static int sys_dsp2arm = -1,
	   sys_arm2dsp = -1,
	   sig_dsp2arm = -1,
	   sig_arm2dsp = -1;

static int sync_indicated = 0;
static int time_indicated = 0;

static int open_devices()
{
	sys_dsp2arm = open(BTS_DSP2ARM, O_RDONLY);
	if (sys_dsp2arm == -1) {
		perror("Failed to open dsp2arm system queue");	
		return -1;
	}

	sys_arm2dsp = open(BTS_ARM2DSP, O_WRONLY);
	if (sys_arm2dsp == -1) {
		perror("Failed to open arm2dsp system queue");
		return -2;
	}

	sig_dsp2arm = open(L1_SIG_DSP2ARM, O_RDONLY);
	if (sig_dsp2arm == -1) {
		perror("Failed to open dsp2arm sig queue");
		return -3;
	}

	sig_arm2dsp = open(L1_SIG_ARM2DSP, O_WRONLY);
	if (sig_arm2dsp == -1) {
		perror("Failed to open arm2dsp sig queue");
		return -4;
	}

	return 0;
}

/**
 * Send a primitive to the system queue
 */
static int send_primitive(int primitive, SuperFemto_Prim_t *prim)
{
	prim->id = primitive;
	return write(sys_arm2dsp, prim, sizeof(*prim)) != sizeof(*prim);
}

/**
 * Wait for a confirmation
 */
static int wait_primitive(int wait_for, SuperFemto_Prim_t *prim)
{
	memset(prim, 0, sizeof(*prim));
	int rc = read(sys_dsp2arm, prim, sizeof(*prim));
	if (rc != sizeof(*prim)) {
		printf("Short read in %s: %d\n", __func__, rc);
		return -1;
	}

	if (prim->id != wait_for) {
		printf("Got primitive %d but waited for %d\n",
			prim->id, wait_for);
		return -2;
	}

	return 0;
}

/* The Cnf for the Req, assume it is a +1 */
static int answer_for(int primitive)
{
	return primitive + 1;
}

static int send_recv_primitive(int p, SuperFemto_Prim_t *prim)
{
	int rc;
	rc = send_primitive(p, prim);
	if (rc != 0)
		return -1;

	rc = wait_primitive(answer_for(p), prim);
	if (rc != 0)
		return -2;
	return 0;
}

static int answer_for_sig(int prim)
{
	static const GsmL1_PrimId_t cnf[] = {
		[GsmL1_PrimId_MphInitReq] 	= GsmL1_PrimId_MphInitCnf,
		[GsmL1_PrimId_MphCloseReq]	= GsmL1_PrimId_MphCloseCnf,
		[GsmL1_PrimId_MphConnectReq]	= GsmL1_PrimId_MphConnectCnf,
		[GsmL1_PrimId_MphActivateReq]	= GsmL1_PrimId_MphActivateCnf,
		[GsmL1_PrimId_MphConfigReq]	= GsmL1_PrimId_MphConfigCnf,
		[GsmL1_PrimId_MphMeasureReq]	= GsmL1_PrimId_MphMeasureCnf,
	};

	if (prim < 0 || prim >= ARRAY_SIZE(cnf)) {
		printf("Unknown primitive: %d\n", prim);
		exit(-3);
	}

	return cnf[prim];
}

static int is_indication(int prim)
{
	return
		prim == GsmL1_PrimId_MphTimeInd       ||
		prim == GsmL1_PrimId_MphSyncInd       ||
		prim == GsmL1_PrimId_PhConnectInd     ||
		prim == GsmL1_PrimId_PhReadyToSendInd ||
		prim == GsmL1_PrimId_PhDataInd        ||
		prim == GsmL1_PrimId_PhRaInd;
}


static int send_recv_sig_prim(int p, GsmL1_Prim_t *prim)
{
	int rc;
	prim->id = p;
	rc = write(sig_arm2dsp, prim, sizeof(*prim));
	if (rc != sizeof(*prim)) {
		printf("Failed to write: %d\n", rc);
		return -1;
	}

	do {
		rc = read(sig_dsp2arm, prim, sizeof(*prim));
		if (rc != sizeof(*prim)) {
			printf("Failed to read: %d\n", rc);
			return -2;
		}
	} while (is_indication(prim->id));

	if (prim->id != answer_for_sig(p)) {
		printf("Wrong L1 result got %d wanted %d for prim: %d\n",
			prim->id, answer_for_sig(p), p);
		return -3;
	}

	return 0;
}

static int wait_for_indication(int p, GsmL1_Prim_t *prim)
{
	int rc;
	memset(prim, 0, sizeof(*prim));

	struct timespec start_time, now_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	/*
	 * TODO: select.... with timeout. The below will work 99% as we will
	 * get time indications very soonish after the connect
	 */
	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &now_time);
		if (now_time.tv_sec - start_time.tv_sec > 10) {
			printf("Timeout waiting for indication.\n");
			return -4;
		}

		rc = read(sig_dsp2arm, prim, sizeof(*prim));
		if (rc != sizeof(*prim)) {
			printf("Failed to read.\n");
			return -1;
		}

		if (!is_indication(prim->id)) {
			printf("No indication: %d\n", prim->id);
			return -2;
		}

		if (p != prim->id && prim->id == GsmL1_PrimId_MphSyncInd) {
			printf("Got sync.\n");
			sync_indicated = 1;
			continue;
		}
		if (p != prim->id && prim->id == GsmL1_PrimId_MphTimeInd) {
			time_indicated = 1;
			continue;
		}

		if (p != prim->id) {
			printf("Wrong indication got %d wanted %d\n",
				prim->id, p);
			return -3;
		}

		break;
	}

	return 0;
}

static int set_trace_flags(uint32_t dsp)
{
	SuperFemto_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	prim.u.setTraceFlagsReq.u32Tf = dsp;
	return send_primitive(SuperFemto_PrimId_SetTraceFlagsReq, &prim);
}
	
static int reset_and_wait()
{
	int rc;
	SuperFemto_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	rc = send_recv_primitive(SuperFemto_PrimId_Layer1ResetReq, &prim);
	if (rc != 0)
		return -1;
	if (prim.u.layer1ResetCnf.status != GsmL1_Status_Success)
		return -2;
	return 0;
}

/**
 * Open the message queues and (re-)initialize the DSP and FPGA
 */
int initialize_layer1(uint32_t dsp_flags)
{
	if (open_devices() != 0) {
		printf("Failed to open devices.\n");
		return -1;
	}

	if (set_trace_flags(dsp_flags) != 0) {
		printf("Failed to set dsp flags.\n");
		return -2;
	}
	if (reset_and_wait() != 0) {
		printf("Failed to reset the firmware.\n");
		return -3;
	}
	return 0;
}

/**
 * Print systems infos
 */
int print_system_info()
{
	int rc;
	SuperFemto_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	rc = send_recv_primitive(SuperFemto_PrimId_SystemInfoReq, &prim);
	if (rc != 0) {
		printf("Failed to send SystemInfoRequest.\n");
		return -1;
	}

	if (prim.u.systemInfoCnf.status != GsmL1_Status_Success) {
		printf("Failed to request SystemInfoRequest.\n");
		return -2;
	}

#define INFO_DSP(x)  x.u.systemInfoCnf.dspVersion
#define INFO_FPGA(x) x.u.systemInfoCnf.fpgaVersion
#ifdef FEMTOBTS_NO_BOARD_VERSION
#define BOARD_REV(x) -1
#define BOARD_OPT(x) -1
#define COMPILED_MAJOR (FEMTOBTS_API_VERSION >> 16)
#define COMPILED_MINOR ((FEMTOBTS_API_VERSION >> 8) & 0xff)
#define COMPILED_BUILD (FEMTOBTS_API_VERSION & 0xff)
#else
#define BOARD_REV(x) x.u.systemInfoCnf.boardVersion.rev
#define BOARD_OPT(x) x.u.systemInfoCnf.boardVersion.option
#define COMPILED_MAJOR (SUPERFEMTO_API_VERSION >> 16)
#define COMPILED_MINOR ((SUPERFEMTO_API_VERSION >> 8) & 0xff)
#define COMPILED_BUILD (SUPERFEMTO_API_VERSION & 0xff)
#endif

	printf("Compiled against: v%u.%u.%u\n",
		COMPILED_MAJOR, COMPILED_MINOR, COMPILED_BUILD);
	printf("Running DSP v%d.%d.%d FPGA v%d.%d.%d Rev: %d Option: %d\n",
		INFO_DSP(prim).major, INFO_DSP(prim).minor, INFO_DSP(prim).build,
		INFO_FPGA(prim).major, INFO_FPGA(prim).minor, INFO_FPGA(prim).build,
		BOARD_REV(prim), BOARD_OPT(prim));

	if (COMPILED_MAJOR != INFO_DSP(prim).major || COMPILED_MINOR != INFO_DSP(prim).minor) {
		printf("WARNING! WARNING! WARNING! WARNING! WARNING\n");
		printf("You might run this against an incompatible firmware.\n");
		printf("Continuing anyway but the result might be broken\n");
	}
#undef INFO_DSP
#undef INFO_FPGA
#undef BOARD_REV
#undef BOARD_OPT
#undef COMPILED_MAJOR
#undef COMPILED_MINOR
#undef COMPILED_BUILD
	return 0;
}

int activate_rf_frontend(int clock_source, int initial_cor)
{
	int rc;
	SuperFemto_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	prim.u.activateRfReq.timing.u8TimSrc = 1;
	prim.u.activateRfReq.msgq.u8UseTchMsgq = 0;
	prim.u.activateRfReq.msgq.u8UsePdtchMsgq = 0;

	prim.u.activateRfReq.rfTrx.iClkCor = initial_cor;
	prim.u.activateRfReq.rfTrx.clkSrc = clock_source;
#if SUPERFEMTO_API_VERSION < SUPERFEMTO_API(2,4,0)
	prim.u.activateRfReq.rfRx.iClkCor = initial_cor;
	prim.u.activateRfReq.rfRx.clkSrc = clock_source;
#endif

	rc = send_recv_primitive(SuperFemto_PrimId_ActivateRfReq, &prim);
	return rc;
}

static int mph_init(int band, int arfcn, HANDLE *layer1)
{
	int rc;
	GsmL1_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	prim.u.mphInitReq.deviceParam.devType = GsmL1_DevType_Rxd;
	prim.u.mphInitReq.deviceParam.freqBand = band;
	prim.u.mphInitReq.deviceParam.u16Arfcn = arfcn;
	prim.u.mphInitReq.deviceParam.u16BcchArfcn = arfcn;
	prim.u.mphInitReq.deviceParam.fRxPowerLevel = -75.f;
	prim.u.mphInitReq.deviceParam.u8AutoTA = 1;

	rc = send_recv_sig_prim(GsmL1_PrimId_MphInitReq, &prim);
	if (rc != 0) {
		printf("Failed to initialize the physical channel.\n");
		return -1;
	}

	if (prim.u.mphInitCnf.status != GsmL1_Status_Success) {
		printf("MPH Init failed.\n");
		return -2;
	}

#if 0
	if (prim.u.mphInitCnf.freqBand != band) {
		printf("Layer1 ignored the band: %d\n",
			prim.u.mphInitCnf.freqBand);
		return -3;
	}
#endif

	*layer1 = prim.u.mphInitCnf.hLayer1;
	return 0;
}

int mph_close(HANDLE layer1)
{
	int rc;
	GsmL1_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	prim.u.mphCloseReq.hLayer1 = layer1;
	rc = send_recv_sig_prim(GsmL1_PrimId_MphCloseReq, &prim);
	if (rc != 0) {
		printf("Failed to close the MPH\n");
		return -6;
	}
	if (prim.u.mphCloseCnf.status != GsmL1_Status_Success) {
		printf("MPH Close failed.\n");
		return -7;
	}

	return 0;
}

int follow_sch(int band, int arfcn, int clock, int ref, HANDLE *layer1)
{
	int rc;
	GsmL1_Prim_t prim;

	time_indicated = 0;
	sync_indicated = 0;

	rc = mph_init(band, arfcn, layer1);
	if (rc != 0)
		return rc;

	/* 1.) Connect */
	memset(&prim, 0, sizeof(prim));
	prim.u.mphConnectReq.hLayer1 = *layer1;
	prim.u.mphConnectReq.u8Tn = 0;
	prim.u.mphConnectReq.logChComb = GsmL1_LogChComb_IV;
	rc = send_recv_sig_prim(GsmL1_PrimId_MphConnectReq, &prim);
	if (rc != 0) {
		printf("Failed to connect.\n");
		return -1;
	}
	if (prim.u.mphConnectCnf.status != GsmL1_Status_Success) {
		printf("Connect failed.\n");
		return -2;
	}
	if (prim.u.mphConnectCnf.u8Tn != 0) {
		printf("Wrong timeslot.\n");
		return -3;
	}

	/* 2.) Activate */
	memset(&prim, 0, sizeof(prim));
	prim.u.mphActivateReq.hLayer1 = *layer1;
	prim.u.mphActivateReq.u8Tn = 0;
	prim.u.mphActivateReq.sapi = GsmL1_Sapi_Sch;
	prim.u.mphActivateReq.dir = GsmL1_Dir_RxDownlink;
	rc = send_recv_sig_prim(GsmL1_PrimId_MphActivateReq, &prim);
	if (rc != 0) {
		printf("Activation failed.\n");
		return -4;
	}
	if (prim.u.mphActivateCnf.status != GsmL1_Status_Success) {
		printf("Activation not successful.\n");
		return -5;
	}

	/* 3.) Wait for indication... TODO: check... */
	printf("Waiting for connect indication.\n");
	rc = wait_for_indication(GsmL1_PrimId_PhConnectInd, &prim);
	if (rc != 0) {
		printf("Didn't get a connect indication.\n");
		return rc;
	}

	/* 4.) Indication Syndication TODO: check... */
	if (!sync_indicated) {
		printf("Waiting for sync indication.\n");
		rc = wait_for_indication(GsmL1_PrimId_MphSyncInd, &prim);
		if (rc < 0) {
			printf("Didn't get a sync indication.\n");
			return -23;
		} else if (rc == 0) {
			if (!prim.u.mphSyncInd.u8Synced) {
				printf("Failed to get sync.\n");
				return -23;
			} else {
				printf("Synced.\n");
			}
		}
	} else {
		printf("Already synced.\n");
	}

	return 0;
}

static int follow_sapi(HANDLE layer1, const GsmL1_Sapi_t sapi)
{
	int rc;
	GsmL1_Prim_t prim;

	/* 1.) Activate BCCH or such... */
	memset(&prim, 0, sizeof(prim));
	prim.u.mphActivateReq.hLayer1 = layer1;
	prim.u.mphActivateReq.u8Tn = 0;
	prim.u.mphActivateReq.sapi = sapi;
	prim.u.mphActivateReq.dir = GsmL1_Dir_RxDownlink;

	rc = send_recv_sig_prim(GsmL1_PrimId_MphActivateReq, &prim);
	if (rc != 0) {
		printf("Activation failed.\n");
		return -4;
	}
	if (prim.u.mphActivateCnf.status != GsmL1_Status_Success) {
		printf("Activation not successful.\n");
		return -5;
	}

	/* 2.) Wait for indication... */
	printf("Waiting for connect indication.\n");
	rc = wait_for_indication(GsmL1_PrimId_PhConnectInd, &prim);
	if (rc != 0) {
		printf("Didn't get a connect indication.\n");
		return rc;
	}

	if (prim.u.phConnectInd.sapi != sapi) {
		printf("Got a connect indication for the wrong type: %d\n",
			prim.u.phConnectInd.sapi);
		return -6;
	}

	/* 3.) Wait for PhDataInd... */
	printf("Waiting for data.\n");
	rc = wait_for_indication(GsmL1_PrimId_PhDataInd, &prim);
	if (rc != 0) {
		printf("Didn't get data.\n");
		return rc;
	}

	return 0;
}

int follow_bcch(HANDLE layer1)
{
	return follow_sapi(layer1, GsmL1_Sapi_Bcch);
}

int follow_pch(HANDLE layer1)
{
	return follow_sapi(layer1, GsmL1_Sapi_Pch);
}

int find_bsic(void)
{
	int rc, i;
	GsmL1_Prim_t prim;

	printf("Waiting for SCH data.\n");
	for (i = 0; i < 10; ++i) {
		uint8_t bsic;
		rc = wait_for_indication(GsmL1_PrimId_PhDataInd, &prim);
		if (rc < 0) {
			printf("Didn't get SCH data.\n");
			return rc;
		}
		if (prim.u.phDataInd.sapi != GsmL1_Sapi_Sch)
			continue;

		bsic = (prim.u.phDataInd.msgUnitParam.u8Buffer[0] >> 2) & 0xFF;
		return bsic;
	}

	printf("Giving up finding the SCH\n");
	return -1;
}

int set_tsc_from_bsic(HANDLE layer1, int bsic)
{
	int rc;
	int tsc = bsic & 0x7;
	GsmL1_Prim_t prim;

	memset(&prim, 0, sizeof(prim));
	prim.u.mphConfigReq.hLayer3 = 0x23;
	prim.u.mphConfigReq.hLayer1 = layer1;
	prim.u.mphConfigReq.cfgParamId = GsmL1_ConfigParamId_SetNbTsc;
	prim.u.mphConfigReq.cfgParams.setNbTsc.u8NbTsc = tsc;
	rc = send_recv_sig_prim(GsmL1_PrimId_MphConfigReq, &prim);
	if (rc != 0) {
		printf("Failed to send configure.\n");
	}

	if (prim.u.mphConfigCnf.status != GsmL1_Status_Success) {
		printf("Failed to set the config cnf.\n");
		return -1;
	}

	return 0;
}

int set_clock_cor(int clock_cor, int calib, int source)
{
	int rc;
	SuperFemto_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	prim.u.rfClockSetupReq.rfTrx.iClkCor = clock_cor;
	prim.u.rfClockSetupReq.rfTrx.clkSrc = calib;
#if SUPERFEMTO_API_VERSION < SUPERFEMTO_API(2,4,0)
	prim.u.rfClockSetupReq.rfRx.iClkCor = clock_cor;
	prim.u.rfClockSetupReq.rfRx.clkSrc = calib;
#endif
	prim.u.rfClockSetupReq.rfTrxClkCal.clkSrc = source;

	rc = send_recv_primitive(SuperFemto_PrimId_RfClockSetupReq, &prim);
	if (rc != 0) {
		printf("Failed to set the clock setup.\n");
		return -1;
	}
	if (prim.u.rfClockSetupCnf.status != GsmL1_Status_Success) {
		printf("Clock setup was not successfull.\n");
		return -2;
	}

	return 0;
}

int rf_clock_info(int *clkErr, int *clkErrRes)
{
	SuperFemto_Prim_t prim;
	memset(&prim, 0, sizeof(prim));

	int rc;

	/* reset the counter */
	prim.u.rfClockInfoReq.u8RstClkCal = 1;
	rc = send_recv_primitive(SuperFemto_PrimId_RfClockInfoReq, &prim);
	if (rc != 0) {
		printf("Failed to reset the clock info.\n");
		return -1;
	}

	/* wait for a value */
	wait_read_ignore(15);

	/* ask for the current counter/error */
	memset(&prim, 0, sizeof(prim));
	prim.u.rfClockInfoReq.u8RstClkCal = 0;
	rc = send_recv_primitive(SuperFemto_PrimId_RfClockInfoReq, &prim);
	if (rc != 0) {
		printf("Failed to get the clock info.\n");
		return -2;
	}

	printf("Error: %d Res: %d\n",
		prim.u.rfClockInfoCnf.rfTrxClkCal.iClkErr,
		prim.u.rfClockInfoCnf.rfTrxClkCal.iClkErrRes);
	*clkErr = prim.u.rfClockInfoCnf.rfTrxClkCal.iClkErr;
	*clkErrRes = prim.u.rfClockInfoCnf.rfTrxClkCal.iClkErrRes;
	return 0;
}

int power_scan(int band, int arfcn, int duration, float *mean_rssi)
{
	int rc;
	HANDLE layer1;
	GsmL1_Prim_t prim;

	/* init */
	rc = mph_init(band, arfcn, &layer1);
	if (rc != 0)
		return rc;

	/* mph measure request */
	memset(&prim, 0, sizeof(prim));
	prim.u.mphMeasureReq.hLayer1 = layer1;
	prim.u.mphMeasureReq.u32Duration = duration;
	rc = send_recv_sig_prim(GsmL1_PrimId_MphMeasureReq, &prim);
	if (rc != 0) {
		printf("Failed to send measurement request.\n");
		return -4;
	}

	if (prim.u.mphMeasureCnf.status != GsmL1_Status_Success) {
		printf("MphMeasureReq was not confirmed.\n");
		return -5;
	}

	*mean_rssi = prim.u.mphMeasureCnf.fMeanRssi;

	/* close */
	rc = mph_close(layer1);
	return rc;
}

/**
 * Wait for indication...
 */
int wait_for_sync(HANDLE layer1, int cor, int calib, int source)
{
	GsmL1_Prim_t prim;
	int rc;

	rc = set_clock_cor(cor, calib, source);
	if (rc != 0) {
		printf("Failed to set the clock correction.\n");
		return -1;
	}

	sync_indicated = 0;
	rc = wait_for_indication(GsmL1_PrimId_MphSyncInd, &prim);
	if (rc < 0 && rc != -4) {
		return rc;
	} else if (rc == 0) {
		if (!prim.u.mphSyncInd.u8Synced) {
			printf("Failed to get sync.\n");
			return 0;
		}
		printf("Synced.\n");
		return 1;
	}

	return 0;
}

int wait_for_data(uint8_t *data, size_t *size, uint32_t *fn, uint8_t *block, GsmL1_Sapi_t *sap)
{
	GsmL1_Prim_t prim;
	int rc;

	rc = wait_for_indication(GsmL1_PrimId_PhDataInd, &prim);
	if (rc < 0)
		return rc;
	if (prim.u.phDataInd.sapi == GsmL1_Sapi_Sch)
		return 1;

	*size = prim.u.phDataInd.msgUnitParam.u8Size;
	*fn = prim.u.phDataInd.u32Fn;
	*block = prim.u.phDataInd.u8BlockNbr;
	*sap = prim.u.phDataInd.sapi;
	memcpy(data, prim.u.phDataInd.msgUnitParam.u8Buffer, *size);
	return 0;
}

/**
 * Make sure the pipe is not running full.
 *
 */
static int wait_read_ignore(int seconds)
{
	int max, rc;
	fd_set fds;
	struct timeval timeout;

	max = sys_dsp2arm > sig_dsp2arm ? sys_dsp2arm : sig_dsp2arm;

	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;

	while (1) {
		FD_ZERO(&fds);
		FD_SET(sys_dsp2arm, &fds);
		FD_SET(sig_dsp2arm, &fds);


		rc = select(max + 1, &fds, NULL, NULL, &timeout);
		if (rc == -1) {
			printf("Failed to select.\n");
			return -1;
		} else if (rc) {
			if (FD_ISSET(sys_dsp2arm, &fds)) {
				SuperFemto_Prim_t prim;
				rc = read(sys_dsp2arm, &prim, sizeof(prim));
				if (rc != sizeof(prim)) {
					perror("Failed to read system primitive");
					return -2;
				}
			}
			if (FD_ISSET(sig_dsp2arm, &fds)) {
				GsmL1_Prim_t prim;
				rc = read(sig_dsp2arm, &prim, sizeof(prim));
				if (rc != sizeof(prim)) {
					perror("Failed to read signal primitiven");
					return -3;
				}
			}
		} else if (timeout.tv_sec <= 0 && timeout.tv_usec <= 0) {
			break;
		}

#ifndef __linux__
#error "Non portable code"
#endif
	}
	return 0;
}
