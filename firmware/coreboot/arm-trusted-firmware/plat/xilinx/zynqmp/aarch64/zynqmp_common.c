/*
 * Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <generic_delay_timer.h>
#include <mmio.h>
#include <platform.h>
#include <stdbool.h>
#include <string.h>
#include <xlat_tables.h>
#include "../zynqmp_private.h"
#include "pm_api_sys.h"

/*
 * Table of regions to map using the MMU.
 * This doesn't include TZRAM as the 'mem_layout' argument passed to
 * configure_mmu_elx() will give the available subset of that,
 */
const mmap_region_t plat_arm_mmap[] = {
	{ DEVICE0_BASE, DEVICE0_BASE, DEVICE0_SIZE, MT_DEVICE | MT_RW | MT_SECURE },
	{ DEVICE1_BASE, DEVICE1_BASE, DEVICE1_SIZE, MT_DEVICE | MT_RW | MT_SECURE },
	{ CRF_APB_BASE, CRF_APB_BASE, CRF_APB_SIZE, MT_DEVICE | MT_RW | MT_SECURE },
	{0}
};

static unsigned int zynqmp_get_silicon_ver(void)
{
	static unsigned int ver;

	if (!ver) {
		ver = mmio_read_32(ZYNQMP_CSU_BASEADDR +
				   ZYNQMP_CSU_VERSION_OFFSET);
		ver &= ZYNQMP_SILICON_VER_MASK;
		ver >>= ZYNQMP_SILICON_VER_SHIFT;
	}

	return ver;
}

unsigned int zynqmp_get_uart_clk(void)
{
	unsigned int ver = zynqmp_get_silicon_ver();

	switch (ver) {
	case ZYNQMP_CSU_VERSION_VELOCE:
		return 48000;
	case ZYNQMP_CSU_VERSION_EP108:
		return 25000000;
	case ZYNQMP_CSU_VERSION_QEMU:
		return 133000000;
	default:
		/* Do nothing in default case */
		break;
	}

	return 100000000;
}

#if LOG_LEVEL >= LOG_LEVEL_NOTICE
static const struct {
	unsigned int id;
	unsigned int ver;
	char *name;
	bool evexists;
} zynqmp_devices[] = {
	{
		.id = 0x10,
		.name = "3EG",
	},
	{
		.id = 0x10,
		.ver = 0x2c,
		.name = "3CG",
	},
	{
		.id = 0x11,
		.name = "2EG",
	},
	{
		.id = 0x11,
		.ver = 0x2c,
		.name = "2CG",
	},
	{
		.id = 0x20,
		.name = "5EV",
		.evexists = true,
	},
	{
		.id = 0x20,
		.ver = 0x100,
		.name = "5EG",
		.evexists = true,
	},
	{
		.id = 0x20,
		.ver = 0x12c,
		.name = "5CG",
	},
	{
		.id = 0x21,
		.name = "4EV",
		.evexists = true,
	},
	{
		.id = 0x21,
		.ver = 0x100,
		.name = "4EG",
		.evexists = true,
	},
	{
		.id = 0x21,
		.ver = 0x12c,
		.name = "4CG",
	},
	{
		.id = 0x30,
		.name = "7EV",
		.evexists = true,
	},
	{
		.id = 0x30,
		.ver = 0x100,
		.name = "7EG",
		.evexists = true,
	},
	{
		.id = 0x30,
		.ver = 0x12c,
		.name = "7CG",
	},
	{
		.id = 0x38,
		.name = "9EG",
	},
	{
		.id = 0x38,
		.ver = 0x2c,
		.name = "9CG",
	},
	{
		.id = 0x39,
		.name = "6EG",
	},
	{
		.id = 0x39,
		.ver = 0x2c,
		.name = "6CG",
	},
	{
		.id = 0x40,
		.name = "11EG",
	},
	{ /* For testing purpose only */
		.id = 0x50,
		.ver = 0x2c,
		.name = "15CG",
	},
	{
		.id = 0x50,
		.name = "15EG",
	},
	{
		.id = 0x58,
		.name = "19EG",
	},
	{
		.id = 0x59,
		.name = "17EG",
	},
	{
		.id = 0x60,
		.name = "28DR",
	},
	{
		.id = 0x61,
		.name = "21DR",
	},
	{
		.id = 0x62,
		.name = "29DR",
	},
	{
		.id = 0x63,
		.name = "23DR",
	},
	{
		.id = 0x64,
		.name = "27DR",
	},
	{
		.id = 0x65,
		.name = "25DR",
	},
};

#define ZYNQMP_PL_STATUS_BIT	9
#define ZYNQMP_PL_STATUS_MASK	BIT(ZYNQMP_PL_STATUS_BIT)
#define ZYNQMP_CSU_VERSION_MASK	~(ZYNQMP_PL_STATUS_MASK)

static char *zynqmp_get_silicon_idcode_name(void)
{
	uint32_t id, ver, chipid[2];
	size_t i, j, len;
	enum pm_ret_status ret;
	const char *name = "EG/EV";

	ret = pm_get_chipid(chipid);
	if (ret)
		return "UNKN";

	id = chipid[0] & (ZYNQMP_CSU_IDCODE_DEVICE_CODE_MASK |
			  ZYNQMP_CSU_IDCODE_SVD_MASK);
	id >>= ZYNQMP_CSU_IDCODE_SVD_SHIFT;
	ver = chipid[1] >> ZYNQMP_EFUSE_IPDISABLE_SHIFT;

	for (i = 0; i < ARRAY_SIZE(zynqmp_devices); i++) {
		if (zynqmp_devices[i].id == id &&
		    zynqmp_devices[i].ver == (ver & ZYNQMP_CSU_VERSION_MASK))
			break;
	}

	if (i >= ARRAY_SIZE(zynqmp_devices))
		return "UNKN";

	if (!zynqmp_devices[i].evexists)
		return zynqmp_devices[i].name;

	if (ver & ZYNQMP_PL_STATUS_MASK)
		return zynqmp_devices[i].name;

	len = strlen(zynqmp_devices[i].name) - 2;
	for (j = 0; j < strlen(name); j++) {
		zynqmp_devices[i].name[len] = name[j];
		len++;
	}
	zynqmp_devices[i].name[len] = '\0';

	return zynqmp_devices[i].name;
}

static unsigned int zynqmp_get_rtl_ver(void)
{
	uint32_t ver;

	ver = mmio_read_32(ZYNQMP_CSU_BASEADDR + ZYNQMP_CSU_VERSION_OFFSET);
	ver &= ZYNQMP_RTL_VER_MASK;
	ver >>= ZYNQMP_RTL_VER_SHIFT;

	return ver;
}

static char *zynqmp_print_silicon_idcode(void)
{
	uint32_t id, maskid, tmp;

	id = mmio_read_32(ZYNQMP_CSU_BASEADDR + ZYNQMP_CSU_IDCODE_OFFSET);

	tmp = id;
	tmp &= ZYNQMP_CSU_IDCODE_XILINX_ID_MASK |
	       ZYNQMP_CSU_IDCODE_FAMILY_MASK;
	maskid = ZYNQMP_CSU_IDCODE_XILINX_ID << ZYNQMP_CSU_IDCODE_XILINX_ID_SHIFT |
		 ZYNQMP_CSU_IDCODE_FAMILY << ZYNQMP_CSU_IDCODE_FAMILY_SHIFT;
	if (tmp != maskid) {
		ERROR("Incorrect XILINX IDCODE 0x%x, maskid 0x%x\n", id, maskid);
		return "UNKN";
	}
	VERBOSE("Xilinx IDCODE 0x%x\n", id);
	return zynqmp_get_silicon_idcode_name();
}

static unsigned int zynqmp_get_ps_ver(void)
{
	uint32_t ver = mmio_read_32(ZYNQMP_CSU_BASEADDR + ZYNQMP_CSU_VERSION_OFFSET);

	ver &= ZYNQMP_PS_VER_MASK;
	ver >>= ZYNQMP_PS_VER_SHIFT;

	return ver + 1;
}

static void zynqmp_print_platform_name(void)
{
	unsigned int ver = zynqmp_get_silicon_ver();
	unsigned int rtl = zynqmp_get_rtl_ver();
	char *label = "Unknown";

	switch (ver) {
	case ZYNQMP_CSU_VERSION_VELOCE:
		label = "VELOCE";
		break;
	case ZYNQMP_CSU_VERSION_EP108:
		label = "EP108";
		break;
	case ZYNQMP_CSU_VERSION_QEMU:
		label = "QEMU";
		break;
	case ZYNQMP_CSU_VERSION_SILICON:
		label = "silicon";
		break;
	default:
		/* Do nothing in default case */
		break;
	}

	NOTICE("ATF running on XCZU%s/%s v%d/RTL%d.%d at 0x%x\n",
	       zynqmp_print_silicon_idcode(), label, zynqmp_get_ps_ver(),
	       (rtl & 0xf0) >> 4, rtl & 0xf, BL31_BASE);
}
#else
static inline void zynqmp_print_platform_name(void) { }
#endif

unsigned int zynqmp_get_bootmode(void)
{
	uint32_t r;
	unsigned int ret;

	ret = pm_mmio_read(CRL_APB_BOOT_MODE_USER, &r);

	if (ret != PM_RET_SUCCESS)
		r = mmio_read_32(CRL_APB_BOOT_MODE_USER);

	return r & CRL_APB_BOOT_MODE_MASK;
}

void zynqmp_config_setup(void)
{
	zynqmp_print_platform_name();
	generic_delay_timer_init();
}

unsigned int plat_get_syscnt_freq2(void)
{
	unsigned int ver = zynqmp_get_silicon_ver();

	switch (ver) {
	case ZYNQMP_CSU_VERSION_VELOCE:
		return 10000;
	case ZYNQMP_CSU_VERSION_EP108:
		return 4000000;
	case ZYNQMP_CSU_VERSION_QEMU:
		return 50000000;
	default:
		/* Do nothing in default case */
		break;
	}

	return mmio_read_32(IOU_SCNTRS_BASEFREQ);
}
