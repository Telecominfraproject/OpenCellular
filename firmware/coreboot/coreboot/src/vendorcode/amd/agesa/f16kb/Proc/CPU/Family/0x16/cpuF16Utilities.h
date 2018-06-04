/* $NoKeywords:$ */
/**
 * @file
 *
 * AMD Family_16 specific utility functions.
 *
 * Provides numerous utility functions specific to family 16h.
 *
 * @xrefitem bom "File Content Label" "Release Content"
 * @e project:      AGESA
 * @e sub-project:  CPU/Family/0x16
 * @e \$Revision: 84150 $   @e \$Date: 2012-12-12 15:46:25 -0600 (Wed, 12 Dec 2012) $
 *
 */
/*
 ******************************************************************************
 *
 * Copyright (c) 2008 - 2013, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Advanced Micro Devices, Inc. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 */

#ifndef _CPU_F16_UTILITES_H_
#define _CPU_F16_UTILITES_H_


/*---------------------------------------------------------------------------------------
 *          M I X E D   (Definitions And Macros / Typedefs, Structures, Enums)
 *---------------------------------------------------------------------------------------
 */


/*---------------------------------------------------------------------------------------
 *                 D E F I N I T I O N S     A N D     M A C R O S
 *---------------------------------------------------------------------------------------
 */


/*---------------------------------------------------------------------------------------
 *               T Y P E D E F S,   S T R U C T U R E S,    E N U M S
 *---------------------------------------------------------------------------------------
 */
/// The structure for Software Initiated NB Voltage Transitions
typedef struct {
  UINT32  VidCode;          ///< VID code to transition to
  BOOLEAN SlamMode;         ///< Whether voltage is to be slammed, or stepped
} SW_VOLT_TRANS_NB;

/*---------------------------------------------------------------------------------------
 *                        F U N C T I O N    P R O T O T Y P E
 *---------------------------------------------------------------------------------------
 */

AGESA_STATUS
F16DisablePstate (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
  IN       UINT8 StateNumber,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

AGESA_STATUS
F16TransitionPstate (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
  IN       UINT8              StateNumber,
  IN       BOOLEAN            WaitForTransition,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

AGESA_STATUS
F16GetTscRate (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
     OUT   UINT32 *FrequencyInMHz,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

BOOLEAN
F16LaunchApCore (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
  IN       UINT32 SocketNum,
  IN       UINT32 ModuleNum,
  IN       UINT32 CoreNum,
  IN       UINT32 PrimaryCoreNum,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

BOOLEAN
F16GetNbCofVidUpdate (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
  IN       PCI_ADDR *PciAddress,
     OUT   BOOLEAN *NbVidUpdateAll,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

BOOLEAN
F16IsNbPstateEnabled (
  IN       CPU_SPECIFIC_SERVICES  *FamilySpecificServices,
  IN       PLATFORM_CONFIGURATION *PlatformConfig,
  IN       AMD_CONFIG_PARAMS      *StdHeader
  );

CORE_ID_POSITION
F16CpuAmdCoreIdPositionInInitialApicId (
  IN       CPU_SPECIFIC_SERVICES  *FamilySpecificServices,
  IN       AMD_CONFIG_PARAMS      *StdHeader
  );

VOID
F16SetAgesaWarmResetFlag (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
  IN       AMD_CONFIG_PARAMS *StdHeader,
  IN       WARM_RESET_REQUEST *Request
  );

VOID
F16GetAgesaWarmResetFlag (
  IN       CPU_SPECIFIC_SERVICES *FamilySpecificServices,
  IN       AMD_CONFIG_PARAMS *StdHeader,
     OUT   WARM_RESET_REQUEST *Request
  );

#endif  // _CPU_F16_UTILITES_H_

