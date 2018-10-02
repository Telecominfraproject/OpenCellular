#ifndef SYSMOBTS_LAYER_H
#define SYSMOBTS_LAYER_H

#include <sysmocom/femtobts/superfemto.h>

#ifdef FEMTOBTS_API_VERSION
#define SuperFemto_PrimId_t FemtoBts_PrimId_t
#define SuperFemto_Prim_t FemtoBts_Prim_t
#define SuperFemto_PrimId_SystemInfoReq 	FemtoBts_PrimId_SystemInfoReq
#define SuperFemto_PrimId_SystemInfoCnf		FemtoBts_PrimId_SystemInfoCnf
#define SuperFemto_SystemInfoCnf_t		FemtoBts_SystemInfoCnf_t
#define SuperFemto_PrimId_SystemFailureInd	FemtoBts_PrimId_SystemFailureInd
#define SuperFemto_PrimId_ActivateRfReq		FemtoBts_PrimId_ActivateRfReq
#define SuperFemto_PrimId_ActivateRfCnf		FemtoBts_PrimId_ActivateRfCnf
#define SuperFemto_PrimId_DeactivateRfReq	FemtoBts_PrimId_DeactivateRfReq
#define SuperFemto_PrimId_DeactivateRfCnf	FemtoBts_PrimId_DeactivateRfCnf
#define SuperFemto_PrimId_SetTraceFlagsReq	FemtoBts_PrimId_SetTraceFlagsReq
#define SuperFemto_PrimId_RfClockInfoReq	FemtoBts_PrimId_RfClockInfoReq
#define SuperFemto_PrimId_RfClockInfoCnf	FemtoBts_PrimId_RfClockInfoCnf
#define SuperFemto_PrimId_RfClockSetupReq	FemtoBts_PrimId_RfClockSetupReq
#define SuperFemto_PrimId_RfClockSetupCnf	FemtoBts_PrimId_RfClockSetupCnf
#define SuperFemto_PrimId_Layer1ResetReq	FemtoBts_PrimId_Layer1ResetReq
#define SuperFemto_PrimId_Layer1ResetCnf	FemtoBts_PrimId_Layer1ResetCnf
#define SuperFemto_PrimId_NUM			FemtoBts_PrimId_NUM
#define HW_SYSMOBTS_V1				1
#define SUPERFEMTO_API(x,y,z)			FEMTOBTS_API(x,y,z)
#endif

extern int initialize_layer1(uint32_t dsp_flags);
extern int print_system_info();
extern int activate_rf_frontend(int clock_source, int clock_cor);
extern int power_scan(int band, int arfcn, int duration, float *mean_rssi);
extern int follow_sch(int band, int arfcn, int calib, int reference, HANDLE *layer1);
extern int follow_bch(HANDLE layer1);
extern int find_bsic(void);
extern int set_tsc_from_bsic(HANDLE layer1, int bsic);
extern int set_clock_cor(int clock_corr, int calib, int source);
extern int rf_clock_info(int *clkErr, int *clkErrRes);
extern int mph_close(HANDLE layer1);
extern int wait_for_sync(HANDLE layer1, int cor, int calib, int source);
extern int follow_bcch(HANDLE layer1);
extern int follow_pch(HANDLE layer1);
extern int wait_for_data(uint8_t *data, size_t *size, uint32_t *fn, uint8_t *block, GsmL1_Sapi_t *sapi);

#endif
