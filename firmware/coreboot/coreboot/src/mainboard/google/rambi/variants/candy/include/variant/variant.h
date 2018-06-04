/*
 * This file is part of the coreboot project.
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

#ifndef VARIANT_H
#define VARIANT_H

/*
 * RAM_ID[3:0] are on GPIO_SSUS[40:37]
 * 0b0000 - 4GiB total - 2 x 2GiB Micron MT41K256M16HA-125:E 1600MHz
 * 0b0001 - 4GiB total - 2 x 2GiB Hynix  H5TC4G63AFR-PBA 1600MHz
 * 0b0010 - 4GiB total - 2 x 2GiB Samsung K4B4G1646Q-HYK0 1600MHz
 * 0b0011 - 2GiB total - 1 x 2GiB Samsung K4B4G1646Q-HYK0 1600MHz
 * 0b0100 - 2GiB total - 1 x 2GiB Micron MT41K256M16HA-125:E 1600MHz
 * 0b0101 - 2GiB total - 1 x 2GiB Hynix  H5TC4G63AFR-PBA 1600MHz
 * 0b0110 - 4GiB total - 2 x 2GiB Samsung K4B4G1646E-BYK0 1600MHz
 * 0b0111 - 4GiB total - 2 x 2GiB Micron MT41K256M16TW-107 1600MHz
 * 0b1000 - 2GiB total - 1 x 2GiB Samsung K4B4G1646E-BYK0 1600MHz
 * 0b1001 - 2GiB total - 1 x 2GiB Micron MT41K256M16TW-107 1600MHz
 * 0b1010 - 4GiB total - 2 x 2GiB Hynix  H5TC4G63CFR-PBA 1600MHz
 * 0b1011 - 2GiB total - 1 x 2GiB Hynix  H5TC4G63CFR-PBA 1600MHz
 */

static const uint32_t dual_channel_config =
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 6) | (1 << 7) | (1 << 10);

#define SPD_SIZE 256
#define GPIO_SSUS_37_PAD 57
#define GPIO_SSUS_38_PAD 50
#define GPIO_SSUS_39_PAD 58
#define GPIO_SSUS_40_PAD 52
#define GPIO_SSUS_40_PAD_USE_PULLDOWN

#endif
