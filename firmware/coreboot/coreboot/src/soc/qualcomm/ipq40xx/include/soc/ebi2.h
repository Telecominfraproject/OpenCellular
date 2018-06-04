/*
 * Copyright (c) 2012 The Linux Foundation. All rights reserved.
 *
 * Taken from U-Boot.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SOC_QUALCOMM_IPQ40XX_EBI2_H_
#define __SOC_QUALCOMM_IPQ40XX_EBI2_H_

#define EBI2CR_BASE                                       (0x1A600000)

struct ebi2cr_regs {
	uint32_t chip_select_cfg0;                        /* 0x00000000 */
	uint32_t cfg;                                     /* 0x00000004 */
	uint32_t hw_info;                                 /* 0x00000008 */
	uint8_t reserved0[20];
	uint32_t lcd_cfg0;                                /* 0x00000020 */
	uint32_t lcd_cfg1;                                /* 0x00000024 */
	uint8_t reserved1[8];
	uint32_t arbiter_cfg;                             /* 0x00000030 */
	uint8_t reserved2[28];
	uint32_t debug_sel;                               /* 0x00000050 */
	uint32_t crc_cfg;                                 /* 0x00000054 */
	uint32_t crc_reminder_cfg;                        /* 0x00000058 */
	uint32_t nand_adm_mux;                            /* 0x0000005C */
	uint32_t mutex_addr_offset;                       /* 0x00000060 */
	uint32_t misr_value;                              /* 0x00000064 */
	uint32_t clkon_cfg;                               /* 0x00000068 */
	uint32_t core_clkon_cfg;                          /* 0x0000006C */
};

/* Register: EBI2_CHIP_SELECT_CFG0 */
#define CS7_CFG_MASK                             0x00001000
#define CS7_CFG_DISABLE                          0x00000000
#define CS7_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00001000
#define CS7_CFG(i)                               ((i) << 12)

#define CS6_CFG_MASK                             0x00000800
#define CS6_CFG_DISABLE                          0x00000000
#define CS6_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00000800
#define CS6_CFG(i)                               ((i) << 11)

#define ETM_CS_CFG_MASK                          0x00000400
#define ETM_CS_CFG_DISABLE                       0x00000000
#define ETM_CS_CFG_GENERAL_SRAM_MEMORY_INTERFACE 0x00000400
#define ETM_CS_CFG(i)                            ((i) << 10)

#define CS5_CFG_MASK                             0x00000300
#define CS5_CFG_DISABLE                          0x00000000
#define CS5_CFG_LCD_DEVICE_CONNECTED             0x00000100
#define CS5_CFG_LCD_DEVICE_CHIP_ENABLE           0x00000200
#define CS5_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00000300
#define CS5_CFG(i)                               ((i) << 8)

#define CS4_CFG_MASK                             0x000000c0
#define CS4_CFG_DISABLE                          0x00000000
#define CS4_CFG_LCD_DEVICE_CONNECTED             0x00000040
#define CS4_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x000000C0
#define CS4_CFG(i)                               ((i) << 6)

#define CS3_CFG_MASK                             0x00000020
#define CS3_CFG_DISABLE                          0x00000000
#define CS3_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00000020
#define CS3_CFG(i)                               ((i) << 5)

#define CS2_CFG_MASK                             0x00000010
#define CS2_CFG_DISABLE                          0x00000000
#define CS2_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00000010
#define CS2_CFG(i)                               ((i) << 4)

#define CS1_CFG_MASK                             0x0000000c
#define CS1_CFG_DISABLE                          0x00000000
#define CS1_CFG_SERIAL_FLASH_DEVICE              0x00000004
#define CS1_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00000008
#define CS1_CFG(i)                               ((i) << 2)

#define CS0_CFG_MASK                             0x00000003
#define CS0_CFG_DISABLE                          0x00000000
#define CS0_CFG_SERIAL_FLASH_DEVICE              0x00000001
#define CS0_CFG_GENERAL_SRAM_MEMORY_INTERFACE    0x00000002
#define CS0_CFG(i)                               ((i) << 0)

#endif
