/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <arm_gic.h>
#include <assert.h>
#include <bl31.h>
#include <bl_common.h>
#include <console.h>
#include <cortex_a53.h>
#include <debug.h>
#include <errno.h>
#include <generic_delay_timer.h>
#include <mmio.h>
#include <plat_arm.h>
#include <platform.h>
#include <stddef.h>
#include <string.h>
#include "hi3798cv200.h"
#include "plat_private.h"
#include "platform_def.h"

/* Memory ranges for code and RO data sections */
#define BL31_RO_BASE	(unsigned long)(&__RO_START__)
#define BL31_RO_LIMIT	(unsigned long)(&__RO_END__)

/* Memory ranges for coherent memory section */
#define BL31_COHERENT_RAM_BASE	(unsigned long)(&__COHERENT_RAM_START__)
#define BL31_COHERENT_RAM_LIMIT	(unsigned long)(&__COHERENT_RAM_END__)

#define TZPC_SEC_ATTR_CTRL_VALUE (0x9DB98D45)

static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

static void hisi_tzpc_sec_init(void)
{
	mmio_write_32(HISI_TZPC_SEC_ATTR_CTRL, TZPC_SEC_ATTR_CTRL_VALUE);
}

entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	next_image_info = (type == NON_SECURE)
			? &bl33_image_ep_info : &bl32_image_ep_info;
	/*
	 * None of the images on the ARM development platforms can have 0x0
	 * as the entrypoint
	 */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

/*******************************************************************************
 * Perform any BL31 early platform setup common to ARM standard platforms.
 * Here is an opportunity to copy parameters passed by the calling EL (S-EL1
 * in BL2 & S-EL3 in BL1) before they are lost (potentially). This needs to be
 * done before the MMU is initialized so that the memory layout can be used
 * while creating page tables. BL2 has flushed this information to memory, so
 * we are guaranteed to pick up good data.
 ******************************************************************************/
#if LOAD_IMAGE_V2
void bl31_early_platform_setup(void *from_bl2,
			       void *plat_params_from_bl2)
#else
void bl31_early_platform_setup(bl31_params_t *from_bl2,
			       void *plat_params_from_bl2)
#endif
{
	console_init(PL011_UART0_BASE, PL011_UART0_CLK_IN_HZ, PL011_BAUDRATE);

	/* Init console for crash report */
	plat_crash_console_init();

#if LOAD_IMAGE_V2
		/*
		 * Check params passed from BL2 should not be NULL,
		 */
		bl_params_t *params_from_bl2 = (bl_params_t *)from_bl2;

		assert(params_from_bl2 != NULL);
		assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
		assert(params_from_bl2->h.version >= VERSION_2);

		bl_params_node_t *bl_params = params_from_bl2->head;

		/*
		 * Copy BL33 and BL32 (if present), entry point information.
		 * They are stored in Secure RAM, in BL2's address space.
		 */
		while (bl_params) {
			if (bl_params->image_id == BL32_IMAGE_ID)
				bl32_image_ep_info = *bl_params->ep_info;

			if (bl_params->image_id == BL33_IMAGE_ID)
				bl33_image_ep_info = *bl_params->ep_info;

			bl_params = bl_params->next_params_info;
		}

		if (bl33_image_ep_info.pc == 0)
			panic();

#else /* LOAD_IMAGE_V2 */

	/*
	 * Check params passed from BL2 should not be NULL,
	 */
	assert(params_from_bl2 != NULL);
	assert(params_from_bl2->h.type == PARAM_BL31);
	assert(params_from_bl2->h.version >= VERSION_1);

	/*
	 * Copy BL32 (if populated by BL2) and BL33 entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	if (from_bl2->bl32_ep_info)
		bl32_image_ep_info = *from_bl2->bl32_ep_info;
	bl33_image_ep_info = *from_bl2->bl33_ep_info;
#endif /* LOAD_IMAGE_V2 */
}

void bl31_platform_setup(void)
{
	/* Init arch timer */
	generic_delay_timer_init();

	/* Init GIC distributor and CPU interface */
	plat_arm_gic_driver_init();
	plat_arm_gic_init();

	/* Init security properties of IP blocks */
	hisi_tzpc_sec_init();
}

void bl31_plat_runtime_setup(void)
{
	/* do nothing */
}

void bl31_plat_arch_setup(void)
{
	plat_configure_mmu_el3(BL31_BASE,
			       (BL31_LIMIT - BL31_BASE),
			       BL31_RO_BASE,
			       BL31_RO_LIMIT,
			       BL31_COHERENT_RAM_BASE,
			       BL31_COHERENT_RAM_LIMIT);

	INFO("Boot BL33 from 0x%lx for %lu Bytes\n",
	     bl33_image_ep_info.pc, bl33_image_ep_info.args.arg2);
}
