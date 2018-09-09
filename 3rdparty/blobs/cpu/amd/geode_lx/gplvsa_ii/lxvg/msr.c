/*
* Copyright (c) 2007-2008 Advanced Micro Devices,Inc. ("AMD").
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General
* Public License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
* Boston, MA 02111-1307 USA
*/




#include "lxvg.h"
#include "vsa2.h"

#define FOUND			0
#define UNKNOWN			1
#define REQ_NOT_FOUND	2

struct tagMSR msrDev[] = { 
	{ UNKNOWN,	ID_MCP,		FAKE_ADDRESS },
	{ UNKNOWN,	ID_MC,		FAKE_ADDRESS },		
	{ UNKNOWN,	ID_GP,		FAKE_ADDRESS },
	{ UNKNOWN,	ID_VG,		FAKE_ADDRESS },
	{ UNKNOWN,	ID_DF,		FAKE_ADDRESS },
	{ UNKNOWN,	ID_VIP,		FAKE_ADDRESS }  };		
// DEBUG
//	{ UNKNOWN,	ID_MBIU,	FAKE_ADDRESS }  };
// DEBUG

#define NUM_DEVS sizeof(msrDev) / sizeof(struct tagMSR)


//=================================================================
//	BOOL msrInit(void)
//
//		Handles the details of finding each possible device on the MBUS.
//		If a given device is not found, its structure is left inited at default.
//		If a given device is found, its structure is updated.
//
//		This init routine only checks for devices already in the structure.
//
//		Returns:
//			TRUE - If, for every device, its address was found.
//			FALSE - If, for any device, an error was encountered.
//

unsigned char msrInit(void)
{
	unsigned short issues=0, i;

	//
	// For each item in the list, try to find its address
	//
	for (i=0; i < NUM_DEVS; i++) 
	{
		msrDev[i].Present = msrFindDevice(&msrDev[i]);
		if (msrDev[i].Present != FOUND) issues++;
	}

	if (issues) return(FALSE);

/*
	//
	// For each item in the list, get its real device ID
	//
	for (i=0; i < NUM_DEVS; i++)
		msrDev[i].Id = msrIdDevice(msrDev[i].Address);
*/
	return(TRUE);

}


//=================================================================
// unsigned short msrFindDevice(struct msr *pDev)
//
//		Returns:
//			FOUND - if no errors were detected.  msrAddress has been updated.
//			REQ_NOT_FOUND - Address for 'devId' is unknown.  Caller should 
//					call msrInit() first.  ptr->Address is not updated.
//
//			NOTE: The structure default for the 'Present' field is "UNKNOWN".
//				Therefore, if a given id is passed to msrFindDevice, the 'Present'
//				field will be updated with either 'FOUND' or 'REQ_NOT_FOUND'.  If
//				a given ID is not passed to msrFindDevice, the 'Present' field will
//				be left as 'UNKNOWN'.
//

unsigned short msrFindDevice(struct tagMSR *pDev)
{
	unsigned long msrAdr;
	msrAdr = SYS_LOOKUP_DEVICE(pDev->Id,1);
	if (0 != msrAdr)
	{
		pDev->Routing = msrAdr;
		return(FOUND);
	}

	// All done...
	return(REQ_NOT_FOUND);
}


//=================================================================
// unsigned short msrIdDevice(unsigned long address)
//
//		Reads the capabilities MSR register (typically 0x2000)
//		and returns the 'id' field (bits 23:8)
//
//		Returns:
//			Bits 23:8 of MSR low DWORD
//

unsigned short msrIdDevice(unsigned long address)
{
	struct mValue msrValue;

	asmRead(MBD_MSR_CAP, address, &msrValue.high, &msrValue.low);

	return((unsigned short)((msrValue.low & DEVID_MASK) >> 8)); 

}


//=================================================================
// unsigned short msrRead(unsigned short msrIdx, unsigned short msrReg, struct mValue msrValue)
//
//		Performs a 64-bit read from 'msrReg' in device 'devID'.
//
//		Returns:
//			FOUND - if no errors were detected and msrValue has been updated.
//			UNKNOWN	- an error was detected.  msrValue is not updated.
//			REQ_NOT_FOUND - 'msrAddress' for 'devID' is unknown.  Caller
//						should call msrInit() first.  msrValue is not updated.
//

unsigned short msrRead(unsigned short msrIdx, unsigned short msrReg, struct mValue *msrValue)
{
	
	if (msrDev[msrIdx].Present == FOUND) {
		asmRead(msrReg, msrDev[msrIdx].Routing, &msrValue->high, &msrValue->low);
	}

	return (msrDev[msrIdx].Present);

}


//=================================================================
// unsigned short msrWrite(unsigned short msrIdx, unsigned short msrReg, unsigned long outHi, unsigned long outLo)
//
//		Performs a 64-bit write to 'msrReg' in device 'devID'.
//
//		Returns:
//			FOUND - if no errors were detected and msrValue has been updated.
//			UNKNOWN	- an error was detected.  msrValue is not updated.
//			REQ_NOT_FOUND - 'msrAddress' for 'devID' is unknown.  Caller
//						should call msrInit() first.  msrValue is not updated.
//

unsigned short msrWrite(unsigned short msrIdx, unsigned short msrReg, unsigned long outHi, unsigned long outLo)
{
	struct mValue mVal;

	if (msrDev[msrIdx].Present == FOUND) {
		// Incorporate the OR values
		mVal.high = outHi;
		mVal.low = outLo;

		asmWrite(msrReg, msrDev[msrIdx].Routing, &mVal.high, &mVal.low);
	}

	return (msrDev[msrIdx].Present);

}


//=================================================================
// void msrModify(unsigned short msrIdx, unsigned short msrReg, unsigned long maskHi, unsigned long maskLo, 
//						unsigned long orHi, unsigned long orLo)
//
//		Performs a 64-bit read-modify-write of 'msrReg' in device 'msrIdx'.
//		The mask values indicate the bits that are to be changed, so the 
//		register values are ANDed with the NOT of the mask values to clear
//		out the affected bits.
//
//		Returns:
//			NONE
//

void msrModify(unsigned short msrIdx, unsigned short msrReg, unsigned long maskHi, unsigned long maskLo, unsigned long orHi, unsigned long orLo) 
{
	struct mValue mVal;

	if (msrDev[msrIdx].Present == FOUND) {
		asmRead(msrReg, msrDev[msrIdx].Routing, &mVal.high, &mVal.low);

		// Incorporate the AND values
		mVal.high &= ~maskHi;
		mVal.low &= ~maskLo;

		// Incorporate the OR values
		mVal.high |= orHi;
		mVal.low |= orLo;

		asmWrite(msrReg, msrDev[msrIdx].Routing, &mVal.high, &mVal.low);
	}

	return;

}


//=================================================================
// void msrSave(unsigned short msrIdx, unsigned short *msrList[], struct mValue *msrValue[])
//
//		Saves a list of MSRs limited by an index value of 0xFFFF. 
//
//

void msrSave(unsigned short msrIdx, unsigned short *msrList, struct mValue *msrValue)
{
	unsigned char finished = FALSE;
	unsigned char i = 0;

	if (msrDev[msrIdx].Present == FOUND)
	{
		while (FALSE == finished)
		{	
			if (0xFFFF == msrList[i])
				finished = TRUE;
			else
			{
				asmRead(msrList[i], msrDev[msrIdx].Routing, &msrValue[i].high, &msrValue[i].low);
				i++;
			}
		}
	}

	return;

}


//=================================================================
// void msrRestore(unsigned short msrIdx, unsigned short *msrList[], struct mValue *msrValue[])
//
//		Restores a previously saved list of MSRs limited by an index value of 0xFFFF. 
//
//

void msrRestore(unsigned short msrIdx, unsigned short *msrList, struct mValue *msrValue)
{
	unsigned char finished = FALSE;
	unsigned char i = 0;

	if (msrDev[msrIdx].Present == FOUND)
	{
		while (FALSE == finished)
		{	
			if (0xFFFF == msrList[i])
				finished = TRUE;
			else
			{
				asmWrite(msrList[i], msrDev[msrIdx].Routing, &msrValue[i].high, &msrValue[i].low);
				i++;
			}
		}
	}

	return;

}


//=================================================================
// void msrDump(unsigned short msrIdx, unsigned short msrReg)
//
//		Performs a 64-bit read of 'msrReg' in device 'msrIdx' for debug.
//
//		Returns:
//			NONE
//

void msrDump(unsigned short msrIdx, unsigned short msrReg) 
{
	struct mValue mVal;

	asmRead(msrReg, msrDev[msrIdx].Routing, &mVal.high, &mVal.low);

	return;

}

