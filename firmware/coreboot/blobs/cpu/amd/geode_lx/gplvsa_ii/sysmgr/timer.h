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


typedef struct {
  UCHAR  Timer;        // Timer number
  UCHAR  Mapper;       // Unrestricted Z field 
  USHORT Setup;        // Scale factor & clock select
  USHORT Period;       // microseconds/count
  ULONG  Interval;     // Current interval in milliseconds
  USHORT Mask;         // Bit mask
  USHORT TimerBase;    // I/O base
} TIMERS;


#define MFGPT_ENABLE     0x8000
#define MFGPT_COMPARE2   0x4000
#define MFGPT_COMPARE1   0x2000
#define MFGPT_INITED     0x1000
#define MFGPT_STOP_EN    0x0800
#define MFGPT_EXT_EN     0x0400
#define MFGPT_CMP2MODE   0x0300
#define MFGPT_CMP2GE     0x0200
#define MFGPT_CMP1MODE   0x00C0
#define MFGPT_CMP1GE     0x0080
#define MFGPT_REV_EN     0x0020
#define MFGPT_CLK_SEL    0x0010
#define MFGPT_SCALE_32   0x0005
#define MFGPT_SCALE_1K   0x000A
#define MFGPT_SCALE_2K   0x000B
#define MFGPT_SCALE_4K   0x000C
#define MFGPT_SCALE_8K   0x000D
#define MFGPT_SCALE_16K  0x000E
#define MFGPT_SCALE_32K  0x000F

