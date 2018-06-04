/*
* Copyright (c) 2006-2008 Advanced Micro Devices,Inc. ("AMD").
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

//*	 Function:                                                          *
//*     This file contains code for power-managing the ATA drives.


#include "vsa2.h"
#include "isa.h"

extern UCHAR pascal in_8(USHORT);
extern void pascal out_8(USHORT, UCHAR);


// NOTE: Win95 touches CD-ROMs every 4.2 seconds for AutoDetect

#define HDD0		1
#define HDD1		2
#define HDD2		4 
#define HDD3		8

#define IDE_BSY		0x80		// Status register
#define IDE_RDY		0x40		// Status register
#define IDE_MSK		IDE_BSY+IDE_RDY

#define	IDE_CMD_SPINDOWN			0xE0	// Spindown Command
#define	IDE_CMD_SPINUP				0xE1	// Spinup   Command
#define	IDE_CMD_CHECK_POWER_MODE	0xE5	// Get Power Management State Command
#define IDE_CMD_SLEEP				0xE6

UCHAR DrivesPresent = 0, DrvHd;


//*************************************************************************
// Issues specified command to one or more drive units
//*************************************************************************
void pascal IDE_Command(UCHAR DriveUnit, UCHAR Command)
{ UCHAR DriveSelect, Mask = 1;
  USHORT IO_Port;
  ULONG Timeout; 

  // Only spindown drives that are present
  DriveUnit &= DrivesPresent;

  while (DriveUnit) {

    // Is drive present ?
    if (DriveUnit & Mask) {

      // Yes, determine I/O port
      IO_Port = PRIMARY_IDE;
      if (Mask & (HDD2+HDD3)) {
        IO_Port = SECONDARY_IDE;
      }

      // Determine master/slave
      DriveSelect = 0xA0;					// Master drive
      if (Mask & (HDD1+HDD3)) {
        DriveSelect = 0xB0;					// Slave drive
      }

      // Wait for IDE channel READY
      for (Timeout=0xFFFFF; Timeout; Timeout--) {
        if ((IDE_MSK & in_8(IO_Port+1)) == IDE_RDY)
          break;
      }

      DrvHd = in_8(IO_Port);     	 		// Save Drive/Head register

      out_8(IO_Port, DriveSelect);			// Select drive to spin down
      out_8(IO_Port+1, Command);			// Issue command to the the drive

	  for (Timeout=0xFFFFF; Timeout; Timeout--) {
		if (!(in_8(IO_Port+1) & IDE_BSY))
			break;
	  }

	  in_8(IO_Port+1);						// Clear IRQ

      DriveUnit &= ~Mask;
    }

    // Next drive
    Mask <<= 1;
  }
}

//*************************************************************************
// Spins down specified drive(s)
//*************************************************************************
void pascal SpinDownHardDrive(UCHAR DriveUnit)
{
  IDE_Command(DriveUnit, IDE_CMD_SPINDOWN);
}


//*************************************************************************
// Puts drive(s) into sleep mode
//*************************************************************************
void pascal DriveSleep(UCHAR DriveUnit)
{
//  IDE_Command(DriveUnit, IDE_CMD_SLEEP);
}

//*************************************************************************
// Determines what hard drives are present
// Input:  I/O address of IDE Drive/Head register
//*************************************************************************
UCHAR Check_IDE_Channel(USHORT IDE_DrvHd)
{ UCHAR HDD_Status, DrivesPresent=0;

  DrvHd = in_8(IDE_DrvHd);					// Save Drive/Head register

  out_8(IDE_DrvHd, (UCHAR)(DrvHd & ~0x10));	// Select master drive
  HDD_Status = in_8(IDE_DrvHd);
  if (in_8(IDE_DrvHd+1) == 0x50) {			// Is drive present ?
    DrivesPresent |= HDD0;

    out_8(IDE_DrvHd, (UCHAR)(DrvHd | 0x10));	// Select slave drive
    HDD_Status = in_8(IDE_DrvHd);
    if (in_8(IDE_DrvHd+1) == 0x50) {			// Is drive present ?
      DrivesPresent |= HDD1;
    }
  }

  // Restore Drive/Head register
  out_8(IDE_DrvHd, DrvHd);
  return DrivesPresent;
}


//*************************************************************************
// Determines what hard drives are present
// Input:  I/O address of IDE Drive/Head register
//*************************************************************************
UCHAR GetDrivesPresent(void)
{
  DrivesPresent  = Check_IDE_Channel(PRIMARY_IDE);
  DrivesPresent |= Check_IDE_Channel(SECONDARY_IDE) << 2;

  return DrivesPresent;
}
