/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


#include <stdlib.h>

#include "lte_utils.h"
#include "logger/logger.h"


typedef struct {
	uint16_t	band_id;
	uint32_t	Fdl_low_100khz;
	uint32_t	Fdl_high_100khz;
	uint32_t	Ndl_offset;
	uint32_t	Ndl_high_offset;
	uint32_t	Ful_low_100khz;
	uint32_t	Ful_high_100khz;
	uint32_t	Nul_offset;
	uint32_t	Nul_high_offset;
	uint16_t	bw_mhz;			// max bandwidth
} lte_band_info_t ;

static lte_band_info_t lte_band_info[] = {
	{
		13,			// band_id;
		7460,		// Fdl_low_100khz;
		7560,		// Fdl_high_100khz;
		5180,		// Ndl_offset;
		5279,		// Ndl_high_offset;
		7770,		// Ful_low_100khz;
		7870,		// Ful_high_100khz;
		23180,		// Nul_offset;
		23279,		// Nul_high_offset;
		10			// bw_mhz - max bandwidth
	}
};



//#######################################################################################
static lte_band_info_t* get_lte_band_info(uint16_t band_id)
{
	// walk lte_band_info and find band
	for (uint32_t i = 0; i < (sizeof(lte_band_info) / sizeof(lte_band_info_t)); i++)
	{
		lte_band_info_t* b = &lte_band_info[i];
		if (b->band_id == band_id)
		{
			return b;
		}
	}
	return NULL;

}


//#######################################################################################
bool get_lte_band_channel(uint16_t band_id, lte_direction_e dir, uint32_t* start_hz, uint32_t *end_hz, int32_t *spacing_hz)
{
	bool found = false;
	
	lte_band_info_t* band = get_lte_band_info(band_id);
	if (band)
	{
		found = true;
		*start_hz = (dir == LTE_UL) ? (band->Ful_low_100khz*100000) : (band->Fdl_low_100khz*100000);
		*end_hz = *start_hz + (band->bw_mhz*1000000);
		*spacing_hz = (band->Ful_low_100khz * 100000) - (band->Fdl_low_100khz * 100000);
	}

	return found;
}



//#######################################################################################
uint32_t dl_hz_to_earfcn(uint16_t band_id, uint32_t fdl_)
{
	lte_band_info_t* band = NULL;

	uint32_t fdl = fdl_ / 100000;			//convert to 100khz

	if ((band = get_lte_band_info(band_id)))
	{
		// is it even in range
		if ((fdl < band->Fdl_low_100khz) || (fdl > band->Fdl_high_100khz))
			return 0;

		// Fdl = Fdl_low + 0.1(Ndl - Ndl_offset)
		// so,
		// Ndl = ((fdl - Fdl_low) * 10) + Ndl_offset
		// 
		uint32_t ndl = (fdl - band->Fdl_low_100khz) + band->Ndl_offset;
		return ndl;
	}
	return 0;
}


//#######################################################################################
uint32_t ul_hz_to_earfcn(uint16_t band_id, uint32_t ful_)
{
	lte_band_info_t* band = NULL;

	uint32_t ful = ful_ / 100000;			//convert to 100khz

	if ((band = get_lte_band_info(band_id)))
	{
		// is it even in range
		if ((ful < band->Ful_low_100khz) || (ful > band->Ful_high_100khz))
			return 0;

		// Ful = Ful_flow + 0.1(Nul - Nul_offset)
		// so,
		// Nul = ((ful - Ful_low) * 10) + Nul_offset
		// 
		uint32_t nul = (ful - band->Ful_low_100khz) + band->Nul_offset;
		return nul;
	}
	return 0;
}


//#######################################################################################
uint32_t dl_earfcn_to_hz(uint16_t band_id, uint32_t Ndl)
{
	lte_band_info_t* band = NULL;

	if ((band = get_lte_band_info(band_id)))
	{
		// is it even in range
		if ((Ndl < band->Ndl_offset) || (Ndl > band->Ndl_high_offset))
			return 0;

		// Fdl = Fdl_low + 0.1(Ndl - Ndl_offset)
		uint32_t fdl = band->Fdl_low_100khz + (Ndl - band->Ndl_offset);
		fdl *= 100000;			//convert to hz
		return fdl;
	}
	return 0;
}


//#######################################################################################
uint32_t ul_earfcn_to_hz(uint16_t band_id, uint32_t Nul)
{
	lte_band_info_t* band = NULL;

	if ((band = get_lte_band_info(band_id)))
	{
		// is it even in range
		if ((Nul < band->Nul_offset) || (Nul > band->Nul_high_offset))
			return 0;

		// Ful = Ful_low + 0.1(Nul - Nul_offset)
		uint32_t ful = (band->Ful_low_100khz + ((Nul - band->Nul_offset)));
		ful *= 100000;			//convert to hz
		return ful;
	}
	return 0;
}



//#######################################################################################
// pmax and ref_sig_power are stored in units of 0.1dB
bool validate_lte_cfg(uint16_t band_id, int bandwidth, int ref_sig_power, int earfcndl, int earfcnul, int pmax)
{
	lte_band_info_t* band = NULL;

	if (!(band = get_lte_band_info(band_id)))
		return false;

	// is Pmax in range
	if (pmax < (LTE_PMAX_MIN * 10))					// pmax is stored in units of 0.1dB
		return false;
	if (pmax > (LTE_PMAX_MAX * 10))					// pmax is stored in units of 0.1dB
		return false;

	// bandwidth to Mhz
	uint32_t bw_hz;
	if (bandwidth == 25)
		bw_hz = 5000000;
	else if (bandwidth == 50)
		bw_hz = 10000000;
	else if(bandwidth == 100)
		bw_hz = 20000000;
	else
		return false;

	// is DL in range
	uint32_t fdl = dl_earfcn_to_hz(band_id, earfcndl);
	if (fdl == 0)
		return 0;

	if (((fdl - (bw_hz/2)) < (band->Fdl_low_100khz * 100000)) || ((fdl + (bw_hz/2)) > (band->Fdl_high_100khz * 100000)))
		return 0;

	// is UL in range
	uint32_t ful = ul_earfcn_to_hz(band_id, earfcnul);
	if (ful == 0)
		return 0;
	if (((ful - (bw_hz/2)) < (band->Ful_low_100khz * 100000)) || ((ful + (bw_hz/2)) > (band->Ful_high_100khz * 100000)))
		return 0;

	// ref sig power
	if (ref_sig_power < (-50 * 10))	// ref_sig_power is stored in units of 0.1dB
	{
		return false;
	}
	
	return true;
}

