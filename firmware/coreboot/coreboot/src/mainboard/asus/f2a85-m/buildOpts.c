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

/**
 * @file
 *
 * AMD User options selection for a Brazos platform solution system
 *
 * This file is placed in the user's platform directory and contains the
 * build option selections desired for that platform.
 *
 * For Information about this file, see @ref platforminstall.
 *
 */

#include <stdlib.h>

#include <vendorcode/amd/agesa/f15tn/AGESA.h>

/*  Include the files that instantiate the configuration definitions.  */
#include <vendorcode/amd/agesa/f15tn/Include/AdvancedApi.h>
#include <vendorcode/amd/agesa/f15tn/Proc/CPU/cpuFamilyTranslation.h>
#include <vendorcode/amd/agesa/f15tn/Proc/CPU/Feature/cpuFeatures.h>
#include <vendorcode/amd/agesa/f15tn/Proc/CPU/heapManager.h>
/* the next two headers depend on heapManager.h */
#include <vendorcode/amd/agesa/f15tn/Proc/Common/CreateStruct.h>
#include <vendorcode/amd/agesa/f15tn/Proc/CPU/cpuEarlyInit.h>
/* These tables are optional and may be used to adjust memory timing settings */
#include <vendorcode/amd/agesa/f15tn/Proc/Mem/mm.h>
#include <vendorcode/amd/agesa/f15tn/Proc/Mem/mn.h>


/*  Select the CPU family.  */
#define INSTALL_FAMILY_10_SUPPORT FALSE
#define INSTALL_FAMILY_12_SUPPORT FALSE
#define INSTALL_FAMILY_14_SUPPORT FALSE
#define INSTALL_FAMILY_15_MODEL_1x_SUPPORT TRUE

/*  Select the CPU socket type.  */
#define INSTALL_G34_SOCKET_SUPPORT  FALSE
#define INSTALL_C32_SOCKET_SUPPORT  FALSE
#define INSTALL_S1G3_SOCKET_SUPPORT FALSE
#define INSTALL_S1G4_SOCKET_SUPPORT FALSE
#define INSTALL_ASB2_SOCKET_SUPPORT FALSE
#define INSTALL_FS1_SOCKET_SUPPORT  FALSE
#define INSTALL_FM1_SOCKET_SUPPORT  FALSE
#define INSTALL_FP2_SOCKET_SUPPORT  FALSE
#define INSTALL_FT1_SOCKET_SUPPORT  FALSE
#define INSTALL_AM3_SOCKET_SUPPORT  FALSE

#define INSTALL_FM2_SOCKET_SUPPORT  TRUE

//#define BLDOPT_REMOVE_UDIMMS_SUPPORT           TRUE
#define BLDOPT_REMOVE_SODIMMS_SUPPORT          TRUE
#define BLDOPT_REMOVE_RDIMMS_SUPPORT           TRUE
#define BLDOPT_REMOVE_LRDIMMS_SUPPORT          TRUE
#define BLDOPT_REMOVE_ECC_SUPPORT              TRUE
//#define BLDOPT_REMOVE_BANK_INTERLEAVE          TRUE
//#define BLDOPT_REMOVE_DCT_INTERLEAVE           TRUE
#define BLDOPT_REMOVE_NODE_INTERLEAVE          TRUE
#define BLDOPT_REMOVE_PARALLEL_TRAINING        TRUE
#define BLDOPT_REMOVE_ONLINE_SPARE_SUPPORT     TRUE
//#define BLDOPT_REMOVE_MEM_RESTORE_SUPPORT      TRUE
#define BLDOPT_REMOVE_MULTISOCKET_SUPPORT        TRUE
//#define BLDOPT_REMOVE_ACPI_PSTATES             FALSE
#define BLDOPT_REMOVE_SRAT                     FALSE //TRUE
#define BLDOPT_REMOVE_SLIT                     FALSE //TRUE
#define BLDOPT_REMOVE_WHEA                     FALSE //TRUE
#define	BLDOPT_REMOVE_CRAT			TRUE
#define BLDOPT_REMOVE_DMI                      TRUE
//#define BLDOPT_REMOVE_EARLY_SAMPLES            FALSE
//#define BLDCFG_REMOVE_ACPI_PSTATES_PPC               TRUE
//#define BLDCFG_REMOVE_ACPI_PSTATES_PCT               TRUE
//#define BLDCFG_REMOVE_ACPI_PSTATES_PSD               TRUE
//#define BLDCFG_REMOVE_ACPI_PSTATES_PSS               TRUE
//#define BLDCFG_REMOVE_ACPI_PSTATES_XPSS              TRUE

//This element selects whether P-States should be forced to be independent,
// as reported by the ACPI _PSD object. For single-link processors,
// setting TRUE for OS to support this feature.

//#define BLDCFG_FORCE_INDEPENDENT_PSD_OBJECT  TRUE

#define BLDCFG_PCI_MMIO_BASE	CONFIG_MMCONF_BASE_ADDRESS
#define BLDCFG_PCI_MMIO_SIZE	CONFIG_MMCONF_BUS_NUMBER
/* Build configuration values here.
 */
#define BLDCFG_VRM_CURRENT_LIMIT                 90000
#define BLDCFG_VRM_LOW_POWER_THRESHOLD           0
#define BLDCFG_VRM_MAXIMUM_CURRENT_LIMIT          0
#define BLDCFG_PLAT_NUM_IO_APICS                 3
#define BLDCFG_CORE_LEVELING_MODE                CORE_LEVEL_LOWEST
#define BLDCFG_MEM_INIT_PSTATE                   0

#define BLDCFG_AMD_PLATFORM_TYPE                  AMD_PLATFORM_MOBILE

#define BLDCFG_MEMORY_BUS_FREQUENCY_LIMIT         DDR1866_FREQUENCY
#define BLDCFG_MEMORY_MODE_UNGANGED               TRUE
#define BLDCFG_MEMORY_QUAD_RANK_CAPABLE           TRUE
#define BLDCFG_MEMORY_QUADRANK_TYPE               QUADRANK_UNBUFFERED
#define BLDCFG_MEMORY_RDIMM_CAPABLE               FALSE
#define BLDCFG_MEMORY_UDIMM_CAPABLE               TRUE
#define BLDCFG_MEMORY_SODIMM_CAPABLE              FALSE
#define BLDCFG_MEMORY_ENABLE_BANK_INTERLEAVING    TRUE
#define BLDCFG_MEMORY_ENABLE_NODE_INTERLEAVING    FALSE
#define BLDCFG_MEMORY_CHANNEL_INTERLEAVING        TRUE
#define BLDCFG_MEMORY_POWER_DOWN                  TRUE
#define BLDCFG_POWER_DOWN_MODE                    POWER_DOWN_BY_CHIP_SELECT
#define BLDCFG_ONLINE_SPARE                       FALSE
#define BLDCFG_BANK_SWIZZLE                       TRUE
#define BLDCFG_TIMING_MODE_SELECT                 TIMING_MODE_AUTO
#define BLDCFG_MEMORY_CLOCK_SELECT                DDR1600_FREQUENCY
#define BLDCFG_DQS_TRAINING_CONTROL               TRUE
#define BLDCFG_IGNORE_SPD_CHECKSUM                FALSE
#define BLDCFG_USE_BURST_MODE                     FALSE
#define BLDCFG_MEMORY_ALL_CLOCKS_ON               FALSE
#define BLDCFG_ENABLE_ECC_FEATURE                 FALSE
#define BLDCFG_ECC_REDIRECTION                    FALSE
#define BLDCFG_SCRUB_DRAM_RATE                    0
#define BLDCFG_SCRUB_L2_RATE                      0
#define BLDCFG_SCRUB_L3_RATE                      0
#define BLDCFG_SCRUB_IC_RATE                      0
#define BLDCFG_SCRUB_DC_RATE                      0
#define BLDCFG_ECC_SYMBOL_SIZE                    4
#define BLDCFG_HEAP_DRAM_ADDRESS                  0xB0000
#define BLDCFG_ECC_SYNC_FLOOD                     FALSE
#define BLDCFG_VRM_HIGH_SPEED_ENABLE              TRUE
#define BLDCFG_1GB_ALIGN                          FALSE
#define BLDCFG_VRM_HIGH_SPEED_ENABLE              TRUE
#define BLDCFG_PCIE_REFCLK_SPREAD_SPECTRUM        36		// PCIE Spread Spectrum default value 0.36%
#define BLDCFG_PLATFORM_CSTATE_IO_BASE_ADDRESS    0x1770

#define BLDOPT_REMOVE_ALIB                    FALSE
#define BLDCFG_PLATFORM_CPB_MODE                  CpbModeDisabled
#define BLDCFG_PROCESSOR_SCOPE_NAME0              'P'
#define BLDCFG_PROCESSOR_SCOPE_NAME1              '0'
#define BLDCFG_PLATFORM_CSTATE_MODE               CStateModeC6

#define BLDCFG_CFG_LCD_BACK_LIGHT_CONTROL         200
#define BLDCFG_CFG_ABM_SUPPORT                    0

//#define BLDCFG_PLATFORM_CSTATE_OPDATA             0x1770

// Specify the default values for the VRM controlling the VDDNB plane.
// If not specified, the values used for the core VRM will be applied
//#define BLDCFG_VRM_NB_CURRENT_LIMIT               0  // Not currently used on Trinity
//#define BLDCFG_VRM_NB_LOW_POWER_THRESHOLD         1  // Zero - disable NBPSI_L, Non-zero - enable NBPSI_L
//#define BLDCFG_VRM_NB_SLEW_RATE                   5000 // Used in calculating the VSRampSlamTime
//#define BLDCFG_VRM_NB_ADDITIONAL_DELAY            0  // Not currently used on Trinity
//#define BLDCFG_VRM_NB_HIGH_SPEED_ENABLE           0  // Not currently used on Trinity
//#define BLDCFG_VRM_NB_INRUSH_CURRENT_LIMIT        0  // Not currently used on Trinity

#define BLDCFG_VRM_NB_CURRENT_LIMIT               60000

#define BLDCFG_LVDS_POWER_ON_SEQ_VARY_BL_TO_BLON	3
#define BLDCFG_LVDS_POWER_ON_SEQ_BLON_TO_VARY_BL	3

#if IS_ENABLED(CONFIG_GFXUMA)
#define BLDCFG_UMA_ALIGNMENT                      UMA_4MB_ALIGNED
#define BLDCFG_UMA_ALLOCATION_MODE		  UMA_SPECIFIED
//#define BLDCFG_UMA_ALLOCATION_SIZE      	  0x1000//0x1800//0x1000 /* (1000 << 16) = 256M*/
#define BLDCFG_UMA_ALLOCATION_SIZE      	  0x2000//512M
#define BLDCFG_UMA_ABOVE4G_SUPPORT                FALSE
#endif

#define BLDCFG_IOMMU_SUPPORT    TRUE

#define BLDCFG_CFG_GNB_HD_AUDIO TRUE
//#define BLDCFG_IGPU_SUBSYSTEM_ID            OEM_IGPU_SSID
//#define BLDCFG_IGPU_HD_AUDIO_SUBSYSTEM_ID   OEM_IGPU_HD_AUDIO_SSID
//#define BLFCFG_APU_PCIE_PORTS_SUBSYSTEM_ID  OEM_APU_PCIE_PORTS_SSID

/*  Process the options...
 * This file include MUST occur AFTER the user option selection settings
 */
/*
 * Customized OEM build configurations for FCH component
 */
// #define BLDCFG_SMBUS0_BASE_ADDRESS            0xB00
// #define BLDCFG_SMBUS1_BASE_ADDRESS            0xB20
// #define BLDCFG_SIO_PME_BASE_ADDRESS           0xE00
// #define BLDCFG_ACPI_PM1_EVT_BLOCK_ADDRESS     0x400
// #define BLDCFG_ACPI_PM1_CNT_BLOCK_ADDRESS     0x404
// #define BLDCFG_ACPI_PM_TMR_BLOCK_ADDRESS      0x408
// #define BLDCFG_ACPI_CPU_CNT_BLOCK_ADDRESS     0x410
// #define BLDCFG_ACPI_GPE0_BLOCK_ADDRESS        0x420
// #define BLDCFG_SPI_BASE_ADDRESS               0xFEC10000
// #define BLDCFG_WATCHDOG_TIMER_BASE            0xFEC00000
// #define BLDCFG_HPET_BASE_ADDRESS              0xFED00000
// #define BLDCFG_SMI_CMD_PORT_ADDRESS           0xB0
// #define BLDCFG_ACPI_PMA_BLK_ADDRESS           0xFE00
// #define BLDCFG_ROM_BASE_ADDRESS               0xFED61000
// #define BLDCFG_AZALIA_SSID                    0x780D1022
// #define BLDCFG_SMBUS_SSID                     0x780B1022
// #define BLDCFG_IDE_SSID                       0x780C1022
// #define BLDCFG_SATA_AHCI_SSID                 0x78011022
// #define BLDCFG_SATA_IDE_SSID                  0x78001022
// #define BLDCFG_SATA_RAID5_SSID                0x78031022
// #define BLDCFG_SATA_RAID_SSID                 0x78021022
// #define BLDCFG_EHCI_SSID                      0x78081022
// #define BLDCFG_OHCI_SSID                      0x78071022
// #define BLDCFG_LPC_SSID                       0x780E1022
// #define BLDCFG_SD_SSID                        0x78061022
// #define BLDCFG_XHCI_SSID                      0x78121022
// #define BLDCFG_FCH_PORT80_BEHIND_PCIB         FALSE
// #define BLDCFG_FCH_ENABLE_ACPI_SLEEP_TRAP     TRUE
// #define BLDCFG_FCH_GPP_LINK_CONFIG            PortA4
// #define BLDCFG_FCH_GPP_PORT0_PRESENT          FALSE
// #define BLDCFG_FCH_GPP_PORT1_PRESENT          FALSE
// #define BLDCFG_FCH_GPP_PORT2_PRESENT          FALSE
// #define BLDCFG_FCH_GPP_PORT3_PRESENT          FALSE
// #define BLDCFG_FCH_GPP_PORT0_HOTPLUG          FALSE
// #define BLDCFG_FCH_GPP_PORT1_HOTPLUG          FALSE
// #define BLDCFG_FCH_GPP_PORT2_HOTPLUG          FALSE
// #define BLDCFG_FCH_GPP_PORT3_HOTPLUG          FALSE

CONST AP_MTRR_SETTINGS ROMDATA TrinityApMtrrSettingsList[] =
{
  { AMD_AP_MTRR_FIX64k_00000, 0x1E1E1E1E1E1E1E1E },
  { AMD_AP_MTRR_FIX16k_80000, 0x1E1E1E1E1E1E1E1E },
  { AMD_AP_MTRR_FIX16k_A0000, 0x0000000000000000 },
  { AMD_AP_MTRR_FIX4k_C0000, 0x0000000000000000 },
  { AMD_AP_MTRR_FIX4k_C8000, 0x0000000000000000 },
  { AMD_AP_MTRR_FIX4k_D0000, 0x0000000000000000 },
  { AMD_AP_MTRR_FIX4k_D8000, 0x0000000000000000 },
  { AMD_AP_MTRR_FIX4k_E0000, 0x1818181818181818 },
  { AMD_AP_MTRR_FIX4k_E8000, 0x1818181818181818 },
  { AMD_AP_MTRR_FIX4k_F0000, 0x1818181818181818 },
  { AMD_AP_MTRR_FIX4k_F8000, 0x1818181818181818 },
  { CPU_LIST_TERMINAL }
};

#define BLDCFG_AP_MTRR_SETTINGS_LIST &TrinityApMtrrSettingsList

                  // This is the delivery package title, "BrazosPI"
                  // This string MUST be exactly 8 characters long
#define AGESA_PACKAGE_STRING  {'c', 'b', '_', 'A', 'g', 'e', 's', 'a'}

                  // This is the release version number of the AGESA component
                  // This string MUST be exactly 12 characters long
#define AGESA_VERSION_STRING  {'V', '0', '.', '0', '.', '0', '.', '1', ' ', ' ', ' ', ' '}

/* MEMORY_BUS_SPEED */
#define DDR400_FREQUENCY   200     ///< DDR 400
#define DDR533_FREQUENCY   266     ///< DDR 533
#define DDR667_FREQUENCY   333     ///< DDR 667
#define DDR800_FREQUENCY   400     ///< DDR 800
#define DDR1066_FREQUENCY   533    ///< DDR 1066
#define DDR1333_FREQUENCY   667    ///< DDR 1333
#define DDR1600_FREQUENCY   800    ///< DDR 1600
#define DDR1866_FREQUENCY   933    ///< DDR 1866
#define DDR2100_FREQUENCY   1050   ///< DDR 2100
#define DDR2133_FREQUENCY   1066   ///< DDR 2133
#define DDR2400_FREQUENCY   1200   ///< DDR 2400
#define UNSUPPORTED_DDR_FREQUENCY		1201 ///< Highest limit of DDR frequency

/* QUANDRANK_TYPE*/
#define QUADRANK_REGISTERED				0 ///< Quadrank registered DIMM
#define QUADRANK_UNBUFFERED				1 ///< Quadrank unbuffered DIMM

/* USER_MEMORY_TIMING_MODE */
#define TIMING_MODE_AUTO				0 ///< Use best rate possible
#define TIMING_MODE_LIMITED				1 ///< Set user top limit
#define TIMING_MODE_SPECIFIC			2 ///< Set user specified speed

/* POWER_DOWN_MODE */
#define POWER_DOWN_BY_CHANNEL			0 ///< Channel power down mode
#define POWER_DOWN_BY_CHIP_SELECT		1 ///< Chip select power down mode

/*
 * Agesa optional capabilities selection.
 * Uncomment and mark FALSE those features you wish to include in the build.
 * Comment out or mark TRUE those features you want to REMOVE from the build.
 */

#define DFLT_SMBUS0_BASE_ADDRESS            0xB00
#define DFLT_SMBUS1_BASE_ADDRESS            0xB20
/* The AGESA likes to enable 512 bytes region on this base for LPC bus */
#define DFLT_SIO_PME_BASE_ADDRESS           0xE00
#define DFLT_ACPI_PM1_EVT_BLOCK_ADDRESS     0x800
#define DFLT_ACPI_PM1_CNT_BLOCK_ADDRESS     0x804
#define DFLT_ACPI_PM_TMR_BLOCK_ADDRESS      0x808
#define DFLT_ACPI_CPU_CNT_BLOCK_ADDRESS     0x810
#define DFLT_ACPI_GPE0_BLOCK_ADDRESS        0x820
#define DFLT_SPI_BASE_ADDRESS               0xFEC10000
#define DFLT_WATCHDOG_TIMER_BASE_ADDRESS    0xFEC000F0
#define DFLT_HPET_BASE_ADDRESS              0xFED00000
#define DFLT_SMI_CMD_PORT                   0xB0
#define DFLT_ACPI_PMA_CNT_BLK_ADDRESS       0xFE00
#define DFLT_GEC_BASE_ADDRESS               0xFED61000
#define DFLT_AZALIA_SSID                    0x780D1022
#define DFLT_SMBUS_SSID                     0x780B1022
#define DFLT_IDE_SSID                       0x780C1022
#define DFLT_SATA_AHCI_SSID                 0x78011022
#define DFLT_SATA_IDE_SSID                  0x78001022
#define DFLT_SATA_RAID5_SSID                0x78031022
#define DFLT_SATA_RAID_SSID                 0x78021022
#define DFLT_EHCI_SSID                      0x78081022
#define DFLT_OHCI_SSID                      0x78071022
#define DFLT_LPC_SSID                       0x780E1022
#define DFLT_SD_SSID                        0x78061022
#define DFLT_XHCI_SSID                      0x78121022
#define DFLT_FCH_PORT80_BEHIND_PCIB         FALSE
#define DFLT_FCH_ENABLE_ACPI_SLEEP_TRAP     TRUE
#define DFLT_FCH_GPP_LINK_CONFIG            PortA1B1C1D1
#define DFLT_FCH_GPP_PORT0_PRESENT          TRUE
#define DFLT_FCH_GPP_PORT1_PRESENT          TRUE
#define DFLT_FCH_GPP_PORT2_PRESENT          FALSE
#define DFLT_FCH_GPP_PORT3_PRESENT          FALSE
#define DFLT_FCH_GPP_PORT0_HOTPLUG          FALSE
#define DFLT_FCH_GPP_PORT1_HOTPLUG          FALSE
#define DFLT_FCH_GPP_PORT2_HOTPLUG          FALSE
#define DFLT_FCH_GPP_PORT3_HOTPLUG          FALSE
//#define BLDCFG_IR_PIN_CONTROL	0x33
//#define FCH_NO_XHCI_SUPPORT			FALSE
GPIO_CONTROL   f2a85_m_gpio[] = {
//	{183, Function1, PullUpB},
	{-1}
};
#define BLDCFG_FCH_GPIO_CONTROL_LIST           (&f2a85_m_gpio[0])

// The following definitions specify the default values for various parameters in which there are
// no clearly defined defaults to be used in the common file.  The values below are based on product
// and BKDG content, please consult the AGESA Memory team for consultation.
#define DFLT_SCRUB_DRAM_RATE            (0)
#define DFLT_SCRUB_L2_RATE              (0)
#define DFLT_SCRUB_L3_RATE              (0)
#define DFLT_SCRUB_IC_RATE              (0)
#define DFLT_SCRUB_DC_RATE              (0)
#define DFLT_MEMORY_QUADRANK_TYPE       QUADRANK_UNBUFFERED
#define DFLT_VRM_SLEW_RATE              (5000)

/* Moving this include up will break AGESA. */
#include <PlatformInstall.h>
