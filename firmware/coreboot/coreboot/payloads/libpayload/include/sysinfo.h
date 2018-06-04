/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYSINFO_H
#define _SYSINFO_H

/* Maximum number of memory range definitions. */
#define SYSINFO_MAX_MEM_RANGES 32
/* Allow a maximum of 8 GPIOs */
#define SYSINFO_MAX_GPIOS 8

/* Up to 10 MAC addresses */
#define SYSINFO_MAX_MACS 10

#include <coreboot_tables.h>

struct cb_serial;

/*
 * All pointers in here shall be virtual.
 *
 * If a relocation happens after the last call to lib_get_sysinfo(),
 * it is up to the user to call lib_get_sysinfo() again.
 */
struct sysinfo_t {
	unsigned int cpu_khz;
	struct cb_serial *serial;
	unsigned short ser_ioport;
	unsigned long ser_base; // for mmapped serial

	int n_memranges;

	struct memrange {
		unsigned long long base;
		unsigned long long size;
		unsigned int type;
	} memrange[SYSINFO_MAX_MEM_RANGES];

	struct cb_cmos_option_table *option_table;
	u32 cmos_range_start;
	u32 cmos_range_end;
	u32 cmos_checksum_location;
	u32 vbnv_start;
	u32 vbnv_size;

	char *version;
	char *extra_version;
	char *build;
	char *compile_time;
	char *compile_by;
	char *compile_host;
	char *compile_domain;
	char *compiler;
	char *linker;
	char *assembler;

	char *cb_version;

	struct cb_framebuffer *framebuffer;

	int num_gpios;
	struct cb_gpio gpios[SYSINFO_MAX_GPIOS];
	int num_macs;
	struct mac_address macs[SYSINFO_MAX_MACS];
	char *serialno;

	unsigned long *mbtable; /** Pointer to the multiboot table */

	struct cb_header *header;
	struct cb_mainboard *mainboard;

	void	*vboot_handoff;
	u32	vboot_handoff_size;
	void	*vdat_addr;
	u32	vdat_size;

#if IS_ENABLED(CONFIG_LP_ARCH_X86)
	int x86_rom_var_mtrr_index;
#endif

	void		*tstamp_table;
	void		*cbmem_cons;
	void		*mrc_cache;
	void		*acpi_gnvs;

#define UNDEFINED_STRAPPING_ID (~0)
	u32		board_id;
	u32		ram_code;
	u32		sku_id;

	void		*wifi_calibration;
	uint64_t	ramoops_buffer;
	uint32_t	ramoops_buffer_size;
	struct {
		uint32_t size;
		uint32_t sector_size;
		uint32_t erase_cmd;
	} spi_flash;
	uint64_t fmap_offset;
	uint64_t cbfs_offset;
	uint64_t cbfs_size;
	uint64_t boot_media_size;
	uint64_t mtc_start;
	uint32_t mtc_size;
	void	*chromeos_vpd;
};

extern struct sysinfo_t lib_sysinfo;

/*
 * Check if this is an architecture specific coreboot table record and process
 * it, if it is. Return 1 if record type was recognized, 0 otherwise.
 */
int cb_parse_arch_specific(struct cb_record *rec, struct sysinfo_t *info);

/*
 * Check if the region in range addr..addr+len contains a 16 byte aligned
 * coreboot table. If it does - process the table filling up the sysinfo
 * structure with information from the table. Return 0 on success and -1 on
 * failure.
 */
int cb_parse_header(void *addr, int len, struct sysinfo_t *info);

#endif
