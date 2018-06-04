/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
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

#ifndef _SOC_PCI_DEVS_H_
#define _SOC_PCI_DEVS_H_

/* All these devices live on bus 0 with the associated device and function */

/* SoC transaction router */
#define SOC_DEV 0x0
#define SOC_FUNC 0
# define SOC_DEVID 0x2280

/* Graphics and Display */
#define GFX_DEV 0x2
#define GFX_FUNC 0
# define GFX_DEVID 0x22b1

/* MMC Port */
#define MMC_DEV 0x10
#define MMC_FUNC 0
# define MMC_DEVID 0x2294

/* SDIO Port */
#define SDIO_DEV 0x11
#define SDIO_FUNC 0
# define SDIO_DEVID 0x2295

/* SD Port */
#define SD_DEV 0x12
#define SD_FUNC 0
# define SD_DEVID 0x2296

/* SATA */
#define SATA_DEV 0x13
#define SATA_FUNC 0
#define AHCI1_DEVID 0x22a3

/* xHCI */
#define XHCI_DEV 0x14
#define XHCI_FUNC 0
#define XHCI_DEVID 0x22b5

/* LPE Audio */
#define LPE_DEV 0x15
#define LPE_FUNC 0
# define LPE_DEVID 0x22a8

/* Serial IO 1 */
#define SIO1_DEV 0x18
# define SIO_DMA1_DEV SIO1_DEV
# define SIO_DMA1_FUNC 0
# define SIO_DMA1_DEVID 0x22c0
# define I2C1_DEV SIO1_DEV
# define I2C1_FUNC 1
# define I2C1_DEVID 0x22c1
# define I2C2_DEV SIO1_DEV
# define I2C2_FUNC 2
# define I2C2_DEVID 0x22c2
# define I2C3_DEV SIO1_DEV
# define I2C3_FUNC 3
# define I2C3_DEVID 0x22c3
# define I2C4_DEV SIO1_DEV
# define I2C4_FUNC 4
# define I2C4_DEVID 0x22c4
# define I2C5_DEV SIO1_DEV
# define I2C5_FUNC 5
# define I2C5_DEVID 0x22c5
# define I2C6_DEV SIO1_DEV
# define I2C6_FUNC 6
# define I2C6_DEVID 0x22c6
# define I2C7_DEV SIO1_DEV
# define I2C7_FUNC 7
# define I2C7_DEVID 0x22c7

/* Trusted Execution Engine */
#define TXE_DEV 0x1a
#define TXE_FUNC 0
# define TXE_DEVID 0x2298

/* HD Audio */
#define HDA_DEV 0x1b
#define HDA_FUNC 0
# define HDA_DEVID 0x2284

/* PCIe Ports */
#define PCIE_DEV 0x1c
# define PCIE_PORT1_DEV PCIE_DEV
# define PCIE_PORT1_FUNC 0
# define PCIE_PORT1_DEVID 0x22c8
# define PCIE_PORT2_DEV PCIE_DEV
# define PCIE_PORT2_FUNC 1
# define PCIE_PORT2_DEVID 0x22ca
# define PCIE_PORT3_DEV PCIE_DEV
# define PCIE_PORT3_FUNC 2
# define PCIE_PORT3_DEVID 0x22cc
# define PCIE_PORT4_DEV PCIE_DEV
# define PCIE_PORT4_FUNC 3
# define PCIE_PORT4_DEVID 0x22ce
/* Total number of ROOT PORTS */
#define MAX_ROOT_PORTS_BSW 4

/* Serial IO 2 */
#define SIO2_DEV 0x1e
# define SIO_DMA2_DEV SIO2_DEV
# define SIO_DMA2_FUNC 0
# define SIO_DMA2_DEVID 0x2286
# define PWM1_DEV SIO2_DEV
# define PWM1_FUNC 1
# define PWM1_DEVID 0x2288
# define PWM2_DEV SIO2_DEV
# define PWM2_FUNC 2
# define PWM2_DEVID 0x2289
# define HSUART1_DEV SIO2_DEV
# define HSUART1_FUNC 3
# define HSUART1_DEVID 0x228a
# define HSUART2_DEV SIO2_DEV
# define HSUART2_FUNC 4
# define HSUART2_DEVID 0x228c
# define SPI_DEV SIO2_DEV
# define SPI_FUNC 5
# define SPI_DEVID 0x228e

/* Platform Controller Unit */
#define PCU_DEV 0x1f
# define LPC_DEV PCU_DEV
# define LPC_FUNC 0
# define LPC_DEVID 0x229c
# define SMBUS_DEV PCU_DEV
# define SMBUS_FUNC 3
# define SMBUS_DEVID 0x0f12

/* PCH SCC Device Modes */
#define PCH_DISABLED 0
#define PCH_PCI_MODE 1
#define PCH_ACPI_MODE 2
#endif /* _SOC_PCI_DEVS_H_ */
