/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "AGESA.h"
#include <spd_bin.h>
#include <northbridge/amd/agesa/BiosCallOuts.h>
#include "FchPlatform.h"
#include "cbfs.h"
#include "gpio_ftns.h"
#include "imc.h"
#include "hudson.h"
#include <stdlib.h>

static AGESA_STATUS Fch_Oem_config(UINT32 Func, UINTN FchData, VOID *ConfigPtr);
static AGESA_STATUS board_ReadSpd_from_cbfs(UINT32 Func, UINTN Data, VOID *ConfigPtr);

const BIOS_CALLOUT_STRUCT BiosCallouts[] =
{
	{AGESA_READ_SPD,                 board_ReadSpd_from_cbfs },
	{AGESA_DO_RESET,                 agesa_Reset },
	{AGESA_READ_SPD_RECOVERY,        agesa_NoopUnsupported },
	{AGESA_RUNFUNC_ONAP,             agesa_RunFuncOnAp },
	{AGESA_GET_IDS_INIT_DATA,        agesa_EmptyIdsInitData },
	{AGESA_HOOKBEFORE_DQS_TRAINING,  agesa_NoopSuccess },
	{AGESA_HOOKBEFORE_EXIT_SELF_REF, agesa_NoopSuccess },
	{AGESA_FCH_OEM_CALLOUT,          Fch_Oem_config }
};
const int BiosCalloutsLen = ARRAY_SIZE(BiosCallouts);

//{AGESA_GNB_GFX_GET_VBIOS_IMAGE,  agesa_NoopUnsupported }


/*
 * Hardware Monitor Fan Control
 * Hardware limitation:
 *  HWM will fail to read the input temperature via I2C if other
 *  software switches the I2C address.  AMD recommends using IMC
 *  to control fans, instead of HWM.
 */
static void oem_fan_control(FCH_DATA_BLOCK *FchParams)
{
	FchParams->Imc.ImcEnable = FALSE;
	FchParams->Hwm.HwMonitorEnable = FALSE;
	FchParams->Hwm.HwmFchtsiAutoPoll = FALSE;                /* 1 enable, 0 disable TSI Auto Polling */
}

/**
 * Fch Oem setting callback
 *
 *  Configure platform specific Hudson device,
 *   such Azalia, SATA, IMC etc.
 */
static AGESA_STATUS Fch_Oem_config(UINT32 Func, UINTN FchData, VOID *ConfigPtr)
{
	AMD_CONFIG_PARAMS *StdHeader = (AMD_CONFIG_PARAMS *)ConfigPtr;
	if (StdHeader->Func == AMD_INIT_RESET) {
		FCH_RESET_DATA_BLOCK *FchParams =  (FCH_RESET_DATA_BLOCK *) FchData;
		printk(BIOS_DEBUG, "Fch OEM config in INIT RESET ");
		//FchParams_reset->EcChannel0 = TRUE; /* logical devicd 3 */
		FchParams->LegacyFree = CONFIG_HUDSON_LEGACY_FREE;
		FchParams->FchReset.SataEnable = hudson_sata_enable();
		FchParams->FchReset.IdeEnable = hudson_ide_enable();
		FchParams->FchReset.Xhci0Enable = IS_ENABLED(CONFIG_HUDSON_XHCI_ENABLE);
		FchParams->FchReset.Xhci1Enable = FALSE;
	} else if (StdHeader->Func == AMD_INIT_ENV) {
		FCH_DATA_BLOCK *FchParams = (FCH_DATA_BLOCK *)FchData;
		printk(BIOS_DEBUG, "Fch OEM config in INIT ENV ");


		FchParams->Azalia.AzaliaEnable = AzDisable;

		/* Fan Control */
		oem_fan_control(FchParams);

		/* XHCI configuration */
		FchParams->Usb.Xhci0Enable = IS_ENABLED(CONFIG_HUDSON_XHCI_ENABLE);
		FchParams->Usb.Xhci1Enable = FALSE;

		/* EHCI configuration */
		FchParams->Usb.Ehci3Enable = !IS_ENABLED(CONFIG_HUDSON_XHCI_ENABLE);

		if (IS_ENABLED(CONFIG_BOARD_PCENGINES_APU2)) {
			// Disable EHCI 0 (port 0 to 3)
			FchParams->Usb.Ehci1Enable = FALSE;
		} else {
			// Enable EHCI 0 (port 0 to 3)
			FchParams->Usb.Ehci1Enable = TRUE;
		}

		// Enable EHCI 1 ( port 4 to 7)
		// port 4 and 5 to EHCI header port 6 and 7 to PCIe slot.
		FchParams->Usb.Ehci2Enable = TRUE;

		/* sata configuration */
		FchParams->Sata.SataDevSlpPort0 = 0;	// Disable DEVSLP0 and 1 to make sure GPIO55 and 59 are not used by DEVSLP
		FchParams->Sata.SataDevSlpPort1 = 0;

		FchParams->Sata.SataClass = CONFIG_HUDSON_SATA_MODE;
		switch ((SATA_CLASS)CONFIG_HUDSON_SATA_MODE) {
		case SataRaid:
		case SataAhci:
		case SataAhci7804:
		case SataLegacyIde:
			FchParams->Sata.SataIdeMode = FALSE;
			break;
		case SataIde2Ahci:
		case SataIde2Ahci7804:
		default: /* SataNativeIde */
			FchParams->Sata.SataIdeMode = TRUE;
			break;
		}
	}
	printk(BIOS_DEBUG, "Done\n");

	return AGESA_SUCCESS;
}

static AGESA_STATUS board_ReadSpd_from_cbfs(UINT32 Func, UINTN Data, VOID *ConfigPtr)
{
	AGESA_STATUS Status = AGESA_UNSUPPORTED;
#ifdef __PRE_RAM__
	AGESA_READ_SPD_PARAMS	*info = ConfigPtr;
	u8 index = get_spd_offset();

	if (info->MemChannelId > 0)
		return AGESA_UNSUPPORTED;
	if (info->SocketId != 0)
		return AGESA_UNSUPPORTED;
	if (info->DimmId != 0)
		return AGESA_UNSUPPORTED;

	/* Read index 0, first SPD_SIZE bytes of spd.bin file. */
	if (read_ddr3_spd_from_cbfs((u8*)info->Buffer, index) < 0)
		die("No SPD data\n");

	Status = AGESA_SUCCESS;
#endif
	return Status;
}
