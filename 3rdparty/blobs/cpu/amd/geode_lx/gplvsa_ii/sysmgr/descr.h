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

// Descriptor types: (don't use 0)


#define   P2D_BM		1
#define   P2D_BMO		2
#define   P2D_R			3
#define   P2D_RO		4
#define   P2D_SC		5
#define   P2D_SCO		6
#define   P2D_BMK		7
#define   IOD_BM		8
#define   IOD_SC        9
#define   MDD_LBAR		10
#define   GX2_RCONF     11
#define   MPCI_RCONF    12
#define   EPCI			13
#define   USB_LBAR		14


typedef struct {
  unsigned long Mbiu;
  unsigned long SubtrPid;
  unsigned long ClockGating;
  unsigned char NumCounters;				// Number of statistic counters on this MBIU
  unsigned char ActiveCounters;				// Count of # active statistic counters in use
} MBIU_INFO;

#define MAX_MBIU 3
