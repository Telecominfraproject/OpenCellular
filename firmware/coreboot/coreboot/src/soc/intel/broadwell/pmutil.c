/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

/*
 * Helper functions for dealing with power management registers
 * and the differences between PCH variants.
 */

#include <arch/io.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <console/console.h>
#include <soc/iomap.h>
#include <soc/lpc.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <soc/gpio.h>
#include <security/vboot/vbnv.h>

/* Print status bits with descriptive names */
static void print_status_bits(u32 status, const char *bit_names[])
{
	int i;

	if (!status)
		return;

	for (i = 31; i >= 0; i--) {
		if (status & (1 << i)) {
			if (bit_names[i])
				printk(BIOS_DEBUG, "%s ", bit_names[i]);
			else
				printk(BIOS_DEBUG, "BIT%d ", i);
		}
	}
}

/* Print status bits as GPIO numbers */
static void print_gpio_status(u32 status, int start)
{
	int i;

	if (!status)
		return;

	for (i = 31; i >= 0; i--) {
		if (status & (1 << i))
			printk(BIOS_DEBUG, "GPIO%d ", start + i);
	}
}


/*
 * PM1_CNT
 */

/* Enable events in PM1 control register */
void enable_pm1_control(u32 mask)
{
	u32 pm1_cnt = inl(ACPI_BASE_ADDRESS + PM1_CNT);
	pm1_cnt |= mask;
	outl(pm1_cnt, ACPI_BASE_ADDRESS + PM1_CNT);
}

/* Disable events in PM1 control register */
void disable_pm1_control(u32 mask)
{
	u32 pm1_cnt = inl(ACPI_BASE_ADDRESS + PM1_CNT);
	pm1_cnt &= ~mask;
	outl(pm1_cnt, ACPI_BASE_ADDRESS + PM1_CNT);
}


/*
 * PM1
 */

/* Clear and return PM1 status register */
static u16 reset_pm1_status(void)
{
	u16 pm1_sts = inw(ACPI_BASE_ADDRESS + PM1_STS);
	outw(pm1_sts, ACPI_BASE_ADDRESS + PM1_STS);
	return pm1_sts;
}

/* Print PM1 status bits */
static u16 print_pm1_status(u16 pm1_sts)
{
	const char *pm1_sts_bits[] = {
		[0] = "TMROF",
		[4] = "BM",
		[5] = "GBL",
		[8] = "PWRBTN",
		[10] = "RTC",
		[11] = "PRBTNOR",
		[14] = "PCIEXPWAK",
		[15] = "WAK",
	};

	if (!pm1_sts)
		return 0;

	printk(BIOS_SPEW, "PM1_STS: ");
	print_status_bits(pm1_sts, pm1_sts_bits);
	printk(BIOS_SPEW, "\n");

	return pm1_sts;
}

/* Print, clear, and return PM1 status */
u16 clear_pm1_status(void)
{
	return print_pm1_status(reset_pm1_status());
}

/* Set the PM1 register to events */
void enable_pm1(u16 events)
{
	outw(events, ACPI_BASE_ADDRESS + PM1_EN);
}


/*
 * SMI
 */

/* Clear and return SMI status register */
static u32 reset_smi_status(void)
{
	u32 smi_sts = inl(ACPI_BASE_ADDRESS + SMI_STS);
	outl(smi_sts, ACPI_BASE_ADDRESS + SMI_STS);
	return smi_sts;
}

/* Print SMI status bits */
static u32 print_smi_status(u32 smi_sts)
{
	const char *smi_sts_bits[] = {
		[2] = "BIOS",
		[3] = "LEGACY_USB",
		[4] = "SLP_SMI",
		[5] = "APM",
		[6] = "SWSMI_TMR",
		[8] = "PM1",
		[9] = "GPE0",
		[10] = "GPI",
		[11] = "MCSMI",
		[12] = "DEVMON",
		[13] = "TCO",
		[14] = "PERIODIC",
		[15] = "SERIRQ_SMI",
		[16] = "SMBUS_SMI",
		[17] = "LEGACY_USB2",
		[18] = "INTEL_USB2",
		[20] = "PCI_EXP_SMI",
		[21] = "MONITOR",
		[26] = "SPI",
		[27] = "GPIO_UNLOCK"
	};

	if (!smi_sts)
		return 0;

	printk(BIOS_DEBUG, "SMI_STS: ");
	print_status_bits(smi_sts, smi_sts_bits);
	printk(BIOS_DEBUG, "\n");

	return smi_sts;
}

/* Print, clear, and return SMI status */
u32 clear_smi_status(void)
{
	return print_smi_status(reset_smi_status());
}

/* Enable SMI event */
void enable_smi(u32 mask)
{
	u32 smi_en = inl(ACPI_BASE_ADDRESS + SMI_EN);
	smi_en |= mask;
	outl(smi_en, ACPI_BASE_ADDRESS + SMI_EN);
}

/* Disable SMI event */
void disable_smi(u32 mask)
{
	u32 smi_en = inl(ACPI_BASE_ADDRESS + SMI_EN);
	smi_en &= ~mask;
	outl(smi_en, ACPI_BASE_ADDRESS + SMI_EN);
}


/*
 * ALT_GP_SMI
 */

/* Clear GPIO SMI status and return events that are enabled and active */
static u32 reset_alt_smi_status(void)
{
	u32 alt_sts, alt_en;

	/* Low Power variant moves this to GPIO region as dword */
	alt_sts = inl(GPIO_BASE_ADDRESS + GPIO_ALT_GPI_SMI_STS);
	outl(alt_sts, GPIO_BASE_ADDRESS + GPIO_ALT_GPI_SMI_STS);
	alt_en = inl(GPIO_BASE_ADDRESS + GPIO_ALT_GPI_SMI_EN);

	/* Only report enabled events */
	return alt_sts & alt_en;
}

/* Print GPIO SMI status bits */
static u32 print_alt_smi_status(u32 alt_sts)
{
	if (!alt_sts)
		return 0;

	printk(BIOS_DEBUG, "ALT_STS: ");

	/* First 16 events are GPIO 32-47 */
	print_gpio_status(alt_sts & 0xffff, 32);

	printk(BIOS_DEBUG, "\n");

	return alt_sts;
}

/* Print, clear, and return GPIO SMI status */
u32 clear_alt_smi_status(void)
{
	return print_alt_smi_status(reset_alt_smi_status());
}

/* Enable GPIO SMI events */
void enable_alt_smi(u32 mask)
{
	u32 alt_en;

	alt_en = inl(GPIO_BASE_ADDRESS + GPIO_ALT_GPI_SMI_EN);
	alt_en |= mask;
	outl(alt_en, GPIO_BASE_ADDRESS + GPIO_ALT_GPI_SMI_EN);
}


/*
 * TCO
 */

/* Clear TCO status and return events that are enabled and active */
static u32 reset_tco_status(void)
{
	u32 tcobase = ACPI_BASE_ADDRESS + 0x60;
	u32 tco_sts = inl(tcobase + 0x04);
	u32 tco_en = inl(ACPI_BASE_ADDRESS + 0x68);

	/* Don't clear BOOT_STS before SECOND_TO_STS */
	outl(tco_sts & ~(1 << 18), tcobase + 0x04);

	/* Clear BOOT_STS */
	if (tco_sts & (1 << 18))
		outl(tco_sts & (1 << 18), tcobase + 0x04);

	return tco_sts & tco_en;
}

/* Print TCO status bits */
static u32 print_tco_status(u32 tco_sts)
{
	const char *tco_sts_bits[] = {
		[0] = "NMI2SMI",
		[1] = "SW_TCO",
		[2] = "TCO_INT",
		[3] = "TIMEOUT",
		[7] = "NEWCENTURY",
		[8] = "BIOSWR",
		[9] = "DMISCI",
		[10] = "DMISMI",
		[12] = "DMISERR",
		[13] = "SLVSEL",
		[16] = "INTRD_DET",
		[17] = "SECOND_TO",
		[18] = "BOOT",
		[20] = "SMLINK_SLV"
	};

	if (!tco_sts)
		return 0;

	printk(BIOS_DEBUG, "TCO_STS: ");
	print_status_bits(tco_sts, tco_sts_bits);
	printk(BIOS_DEBUG, "\n");

	return tco_sts;
}

/* Print, clear, and return TCO status */
u32 clear_tco_status(void)
{
	return print_tco_status(reset_tco_status());
}

/* Enable TCO SCI */
void enable_tco_sci(void)
{
	/* Clear pending events */
	outl(ACPI_BASE_ADDRESS + GPE0_STS(3), TCOSCI_STS);

	/* Enable TCO SCI events */
	enable_gpe(TCOSCI_EN);
}


/*
 * GPE0
 */

/* Clear a GPE0 status and return events that are enabled and active */
static u32 reset_gpe(u16 sts_reg, u16 en_reg)
{
	u32 gpe0_sts = inl(ACPI_BASE_ADDRESS + sts_reg);
	u32 gpe0_en = inl(ACPI_BASE_ADDRESS + en_reg);

	outl(gpe0_sts, ACPI_BASE_ADDRESS + sts_reg);

	/* Only report enabled events */
	return gpe0_sts & gpe0_en;
}

/* Print GPE0 status bits */
static u32 print_gpe_status(u32 gpe0_sts, const char *bit_names[])
{
	if (!gpe0_sts)
		return 0;

	printk(BIOS_DEBUG, "GPE0_STS: ");
	print_status_bits(gpe0_sts, bit_names);
	printk(BIOS_DEBUG, "\n");

	return gpe0_sts;
}

/* Print GPE0 GPIO status bits */
static u32 print_gpe_gpio(u32 gpe0_sts, int start)
{
	if (!gpe0_sts)
		return 0;

	printk(BIOS_DEBUG, "GPE0_STS: ");
	print_gpio_status(gpe0_sts, start);
	printk(BIOS_DEBUG, "\n");

	return gpe0_sts;
}

/* Clear all GPE status and return "standard" GPE event status */
u32 clear_gpe_status(void)
{
	const char *gpe0_sts_3_bits[] = {
		[1] = "HOTPLUG",
		[2] = "SWGPE",
		[6] = "TCO_SCI",
		[7] = "SMB_WAK",
		[9] = "PCI_EXP",
		[10] = "BATLOW",
		[11] = "PME",
		[12] = "ME",
		[13] = "PME_B0",
		[16] = "GPIO27",
		[18] = "WADT"
	};

	print_gpe_gpio(reset_gpe(GPE0_STS(GPE_31_0), GPE0_EN(GPE_31_0)), 0);
	print_gpe_gpio(reset_gpe(GPE0_STS(GPE_63_32), GPE0_EN(GPE_63_32)), 32);
	print_gpe_gpio(reset_gpe(GPE0_STS(GPE_94_64), GPE0_EN(GPE_94_64)), 64);
	return print_gpe_status(reset_gpe(GPE0_STS(GPE_STD), GPE0_EN(GPE_STD)),
				gpe0_sts_3_bits);
}

/* Enable all requested GPE */
void enable_all_gpe(u32 set1, u32 set2, u32 set3, u32 set4)
{
	outl(set1, ACPI_BASE_ADDRESS + GPE0_EN(GPE_31_0));
	outl(set2, ACPI_BASE_ADDRESS + GPE0_EN(GPE_63_32));
	outl(set3, ACPI_BASE_ADDRESS + GPE0_EN(GPE_94_64));
	outl(set4, ACPI_BASE_ADDRESS + GPE0_EN(GPE_STD));
}

/* Disable all GPE */
void disable_all_gpe(void)
{
	enable_all_gpe(0, 0, 0, 0);
}

/* Enable a standard GPE */
void enable_gpe(u32 mask)
{
	u32 gpe0_en = inl(ACPI_BASE_ADDRESS + GPE0_EN(GPE_STD));
	gpe0_en |= mask;
	outl(gpe0_en, ACPI_BASE_ADDRESS + GPE0_EN(GPE_STD));
}

/* Disable a standard GPE */
void disable_gpe(u32 mask)
{
	u32 gpe0_en = inl(ACPI_BASE_ADDRESS + GPE0_EN(GPE_STD));
	gpe0_en &= ~mask;
	outl(gpe0_en, ACPI_BASE_ADDRESS + GPE0_EN(GPE_STD));
}

int acpi_sci_irq(void)
{
	int scis = pci_read_config32(PCH_DEV_LPC, ACPI_CNTL) & SCI_IRQ_SEL;
	int sci_irq = 9;

	/* Determine how SCI is routed. */
	switch (scis) {
	case SCIS_IRQ9:
	case SCIS_IRQ10:
	case SCIS_IRQ11:
		sci_irq = scis - SCIS_IRQ9 + 9;
		break;
	case SCIS_IRQ20:
	case SCIS_IRQ21:
	case SCIS_IRQ22:
	case SCIS_IRQ23:
		sci_irq = scis - SCIS_IRQ20 + 20;
		break;
	default:
		printk(BIOS_DEBUG, "Invalid SCI route! Defaulting to IRQ9.\n");
		sci_irq = 9;
		break;
	}

	printk(BIOS_DEBUG, "SCI is IRQ%d\n", sci_irq);
	return sci_irq;
}

int rtc_failure(void)
{
	u8 reg8;
	int rtc_failed;
	device_t dev = PCH_DEV_LPC;

	reg8 = pci_read_config8(dev, GEN_PMCON_3);
	rtc_failed = reg8 & RTC_BATTERY_DEAD;
	if (rtc_failed) {
		reg8 &= ~RTC_BATTERY_DEAD;
		pci_write_config8(dev, GEN_PMCON_3, reg8);
		printk(BIOS_DEBUG, "rtc_failed = 0x%x\n", rtc_failed);
	}

	return !!rtc_failed;
}

int vbnv_cmos_failed(void)
{
	return rtc_failure();
}
