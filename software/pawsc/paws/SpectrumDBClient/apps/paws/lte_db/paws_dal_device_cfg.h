/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_CFG_DEVICE_H_
#define PAWS_DAL_CFG_DEVICE_H_

#include "paws_types.h"

typedef struct
{
			// NOTE: all power fields are stored in this structure in units of 0.1dB.
			// Some are written to the SQL db in this form, and some are converted to dB.  This is governed by the SQL schema on a per fields basis.

	bool	admin_state;					// enabled/disabled													[FAPServiceFAPControlLTE].[AdminState]
	int		x_000295_external_pa_gain;		// [-500:+500] in 0.1dBm,								SQL: 0.1db	[FAPServiceCellConfigLTERANRF].[X_000295_ExternalPAGain]	
	int		capabilities_max_tx_power;		// converted from dbM to units of 0.1dBm				SQL: db		[FAPServiceCapabilities].[MaxTxPower]						
	int		capabilities_min_tx_power;		// [-500:+500] in 0.1dBm,								SQL: 0.1db	[FAPServiceCapabilitiesLTE].[X_000295_MinTxPower]			
	int		bandwidth;						// in Number-of-RB  5MHz=25, 10MHz=50, 20MHz=100					[FAPServiceCellConfigLTERANRF].[ULBandwidth]
	int		earfcndl;						//																	[FAPServiceCellConfigLTERANRF].EARFCNDL
	int		earfcnul;						//																	[FAPServiceCellConfigLTERANRF].EARFCNUL
	int		pmax;							// converted from dbM to units of 0.1dBm				SQL: db		[FAPServiceCellConfigLTERANMobilityIdleModeIntraFreq].[PMax]	
	int		ref_sig_power;					// converted from dbM to units of 0.1dBm				SQL: db		This is the DL power per RB  [FAPServiceCellConfigLTERANRF].[ReferenceSignalPower]		
	int		total_ref_sig_power;			// in units of 0.1dB
	int		antenna_gain;					// in units of 0.1dB
	bool    limited;						// is configured power limited by FAP
} device_cfg_params_t;				




#endif  // PAWS_DAL_CFG_DEVICE_H_

