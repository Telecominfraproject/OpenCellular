/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gicv2.h>
#include <debug.h>
#include "ls_16550.h"
#include "plat_ls.h"
#include "soc.h"

#define BL32_END (unsigned long)(&__BL32_END__)

const unsigned int g0_interrupt_array1[] = {
	9
};

gicv2_driver_data_t ls_gic_data = {
	.gicd_base = GICD_BASE,
	.gicc_base = GICC_BASE,
	.g0_interrupt_num = ARRAY_SIZE(g0_interrupt_array1),
	.g0_interrupt_array = g0_interrupt_array1,
};

/*******************************************************************************
 * Initialize the UART
 ******************************************************************************/
void ls_tsp_early_platform_setup(void)
{
	static console_ls_16550_t console;
	/*
	 * Initialize a different console than already in use to display
	 * messages from TSP
	 */
	console_ls_16550_register(PLAT_LS1043_UART2_BASE, PLAT_LS1043_UART_CLOCK,
			PLAT_LS1043_UART_BAUDRATE, &console);
	NOTICE(FIRMWARE_WELCOME_STR_LS1043_BL32);
}

/*******************************************************************************
 * Perform platform specific setup placeholder
 ******************************************************************************/
void tsp_platform_setup(void)
{
	uint32_t gicc_base, gicd_base;

	/* Initialize the GIC driver, cpu and distributor interfaces */
	get_gic_offset(&gicc_base, &gicd_base);
	ls_gic_data.gicd_base = (uintptr_t)gicd_base;
	ls_gic_data.gicc_base = (uintptr_t)gicc_base;
	gicv2_driver_init(&ls_gic_data);
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the MMU
 ******************************************************************************/
void tsp_plat_arch_setup(void)
{
	ls_setup_page_tables(BL32_BASE,
			      (BL32_END - BL32_BASE),
			      BL_CODE_BASE,
			      BL_CODE_END,
			      BL_RO_DATA_BASE,
			      BL_RO_DATA_END
#if USE_COHERENT_MEM
			      , BL_COHERENT_RAM_BASE,
			      BL_COHERENT_RAM_END
#endif
			      );
	enable_mmu_el1(0);
}
