/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2005 Yinghai Lu
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

#include <cbfs.h>
#include <compiler.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <cpu/cpu.h>
#include <arch/cpu.h>
#include <cpu/x86/cr.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/pae.h>
#include <rules.h>
#include <string.h>
#include <symbols.h>

#define PDPTE_PRES (1ULL << 0)
#define PDPTE_ADDR_MASK (~((1ULL << 12) - 1))

#define PDE_PRES (1ULL << 0)
#define PDE_RW   (1ULL << 1)
#define PDE_US   (1ULL << 2)
#define PDE_PWT  (1ULL << 3)
#define PDE_PCD  (1ULL << 4)
#define PDE_A    (1ULL << 5)
#define PDE_D    (1ULL << 6) // only valid with PS=1
#define PDE_PS   (1ULL << 7)
#define PDE_G    (1ULL << 8)  // only valid with PS=1
#define PDE_PAT  (1ULL << 12) // only valid with PS=1
#define PDE_XD   (1ULL << 63)
#define PDE_ADDR_MASK (~((1ULL << 12) - 1))

#define PTE_PRES (1ULL << 0)
#define PTE_RW   (1ULL << 1)
#define PTE_US   (1ULL << 2)
#define PTE_PWT  (1ULL << 3)
#define PTE_PCD  (1ULL << 4)
#define PTE_A    (1ULL << 5)
#define PTE_D    (1ULL << 6)
#define PTE_PAT  (1ULL << 7)
#define PTE_G    (1ULL << 8)
#define PTE_XD   (1ULL << 63)

#define PDPTE_IDX_SHIFT 30
#define PDPTE_IDX_MASK  0x3

#define PDE_IDX_SHIFT 21
#define PDE_IDX_MASK  0x1ff

#define PTE_IDX_SHIFT 12
#define PTE_IDX_MASK  0x1ff

static const size_t s2MiB = 2 * MiB;
static const size_t s4KiB = 4 * KiB;

void paging_enable_pae_cr3(uintptr_t cr3)
{
	/* Load the page table address */
	write_cr3(cr3);
	paging_enable_pae();
}

void paging_enable_pae(void)
{
	CRx_TYPE cr0;
	CRx_TYPE cr4;

	/* Enable PAE */
	cr4 = read_cr4();
	cr4 |= CR4_PAE;
	write_cr4(cr4);

	/* Enable Paging */
	cr0 = read_cr0();
	cr0 |= CR0_PG;
	write_cr0(cr0);
}

void paging_disable_pae(void)
{
	CRx_TYPE cr0;
	CRx_TYPE cr4;

	/* Disable Paging */
	cr0 = read_cr0();
	cr0 &= ~(CRx_TYPE)CR0_PG;
	write_cr0(cr0);

	/* Disable PAE */
	cr4 = read_cr4();
	cr4 &= ~(CRx_TYPE)CR4_PAE;
	write_cr4(cr4);
}

#if ENV_RAMSTAGE
void *map_2M_page(unsigned long page)
{
	struct pde {
		uint32_t addr_lo;
		uint32_t addr_hi;
	} __packed;
	struct pg_table {
		struct pde pd[2048];
		struct pde pdp[512];
	} __packed;

	static struct pg_table pgtbl[CONFIG_MAX_CPUS]
		__attribute__((aligned(4096)));
	static unsigned long mapped_window[CONFIG_MAX_CPUS];
	unsigned long index;
	unsigned long window;
	void *result;
	int i;
	index = cpu_index();
	if (index >= CONFIG_MAX_CPUS)
		return MAPPING_ERROR;
	window = page >> 10;
	if (window != mapped_window[index]) {
		paging_disable_pae();
		if (window > 1) {
			struct pde *pd, *pdp;
			/* Point the page directory pointers at the page
			 * directories
			 */
			memset(&pgtbl[index].pdp, 0, sizeof(pgtbl[index].pdp));
			pd = pgtbl[index].pd;
			pdp = pgtbl[index].pdp;
			pdp[0].addr_lo = ((uint32_t)&pd[512*0])|1;
			pdp[1].addr_lo = ((uint32_t)&pd[512*1])|1;
			pdp[2].addr_lo = ((uint32_t)&pd[512*2])|1;
			pdp[3].addr_lo = ((uint32_t)&pd[512*3])|1;
			/* The first half of the page table is identity mapped
			 */
			for (i = 0; i < 1024; i++) {
				pd[i].addr_lo = ((i & 0x3ff) << 21) | 0xE3;
				pd[i].addr_hi = 0;
			}
			/* The second half of the page table holds the mapped
			 * page
			 */
			for (i = 1024; i < 2048; i++) {
				pd[i].addr_lo = ((window & 1) << 31)
					| ((i & 0x3ff) << 21) | 0xE3;
				pd[i].addr_hi = (window >> 1);
			}
			paging_enable_pae_cr3((uintptr_t)pdp);
		}
		mapped_window[index] = window;
	}
	if (window == 0)
		result = (void *)(page << 21);
	else
		result = (void *)(0x80000000 | ((page & 0x3ff) << 21));
	return result;
}
#endif

void paging_set_nxe(int enable)
{
	msr_t msr = rdmsr(IA32_EFER);

	if (enable)
		msr.lo |= EFER_NXE;
	else
		msr.lo &= ~EFER_NXE;

	wrmsr(IA32_EFER, msr);
}

void paging_set_pat(uint64_t pat)
{
	msr_t msr;
	msr.lo = pat;
	msr.hi = pat >> 32;
	wrmsr(MSR_IA32_PAT, msr);
}

/* PAT encoding used in util/x86/x86_page_tables.go. It matches the linux
 * kernel settings:
 *  PTE encoding:
 *      PAT
 *      |PCD
 *      ||PWT  PAT
 *      |||    slot
 *      000    0    WB : _PAGE_CACHE_MODE_WB
 *      001    1    WC : _PAGE_CACHE_MODE_WC
 *      010    2    UC-: _PAGE_CACHE_MODE_UC_MINUS
 *      011    3    UC : _PAGE_CACHE_MODE_UC
 *      100    4    WB : Reserved
 *      101    5    WP : _PAGE_CACHE_MODE_WP
 *      110    6    UC-: Reserved
 *      111    7    WT : _PAGE_CACHE_MODE_WT
 */
void paging_set_default_pat(void)
{
	uint64_t pat =  PAT_ENCODE(WB, 0) | PAT_ENCODE(WC, 1) |
			PAT_ENCODE(UC_MINUS, 2) | PAT_ENCODE(UC, 3) |
			PAT_ENCODE(WB, 4) | PAT_ENCODE(WP, 5) |
			PAT_ENCODE(UC_MINUS, 6) | PAT_ENCODE(WT, 7);
	paging_set_pat(pat);
}

static int read_from_cbfs(const char *name, void *buf, size_t size)
{
	struct cbfsf fh;
	struct region_device rdev;
	size_t rdev_sz;

	if (cbfs_boot_locate(&fh, name, NULL))
		return -1;

	cbfs_file_data(&rdev, &fh);

	rdev_sz = region_device_sz(&rdev);

	if (size < rdev_sz) {
		printk(BIOS_ERR, "%s region too small to load: %zx < %zx\n",
			name, size, rdev_sz);
		return -1;
	}

	if (rdev_readat(&rdev, buf, 0, rdev_sz) != rdev_sz)
		return -1;

	return 0;
}

int paging_enable_for_car(const char *pdpt_name, const char *pt_name)
{
	if (!ENV_CACHE_AS_RAM)
		return -1;

	if (read_from_cbfs(pdpt_name, _pdpt, _pdpt_size)) {
		printk(BIOS_ERR, "Couldn't load pdpt\n");
		return -1;
	}

	if (read_from_cbfs(pt_name, _pagetables, _pagetables_size)) {
		printk(BIOS_ERR, "Couldn't load page tables\n");
		return -1;
	}

	paging_enable_pae_cr3((uintptr_t)_pdpt);

	return 0;
}

static void *get_pdpt_addr(void)
{
	if (ENV_CACHE_AS_RAM)
		return _pdpt;
	return (void *)(uintptr_t)read_cr3();
}

static uint64_t pde_pat_flags(int pat)
{
	switch (pat) {
	case PAT_UC:
		return 0 | PDE_PCD | PDE_PWT;
	case PAT_WC:
		return 0 | 0 | PDE_PWT;
	case PAT_WT:
		return PDE_PAT | PDE_PCD | PDE_PWT;
	case PAT_WP:
		return PDE_PAT | 0 | PDE_PWT;
	case PAT_WB:
		return 0 | 0 | 0;
	case PAT_UC_MINUS:
		return 0 | PDE_PCD | 0;
	default:
		printk(BIOS_ERR, "PDE PAT defaulting to WB: %x\n", pat);
		return pde_pat_flags(PAT_WB);
	}
}

static uint64_t pde_page_flags(int pat)
{
	uint64_t flags = PDE_PS | PDE_PRES | PDE_RW | PDE_A | PDE_D;

	return flags | pde_pat_flags(pat);
}

static uint64_t pte_pat_flags(int pat)
{
	switch (pat) {
	case PAT_UC:
		return 0 | PTE_PCD | PTE_PWT;
	case PAT_WC:
		return 0 | 0 | PTE_PWT;
	case PAT_WT:
		return PTE_PAT | PTE_PCD | PTE_PWT;
	case PAT_WP:
		return PTE_PAT | 0 | PTE_PWT;
	case PAT_WB:
		return 0 | 0 | 0;
	case PAT_UC_MINUS:
		return 0 | PTE_PCD | 0;
	default:
		printk(BIOS_ERR, "PTE PAT defaulting to WB: %x\n", pat);
		return pte_pat_flags(PAT_WB);
	}
}

static uint64_t pte_page_flags(int pat)
{
	uint64_t flags = PTE_PRES | PTE_RW | PTE_A | PTE_D;
	return flags | pte_pat_flags(pat);
}

/* Identity map an address. This function does not handle splitting or adding
 * new pages to the page tables. It's assumed all the page tables are already
 * seeded with the correct amount and topology. */
static int identity_map_one_page(uintptr_t base, size_t size, int pat,
				int commit)
{
	uint64_t (*pdpt)[4];
	uint64_t pdpte;
	uint64_t (*pd)[512];
	uint64_t pde;

	pdpt = get_pdpt_addr();

	pdpte = (*pdpt)[(base >> PDPTE_IDX_SHIFT) & PDPTE_IDX_MASK];

	/* No page table page allocation. */
	if (!(pdpte & PDPTE_PRES))
		return -1;

	pd = (void *)(uintptr_t)(pdpte & PDPTE_ADDR_MASK);

	/* Map in a 2MiB page. */
	if (size == s2MiB) {
		if (!commit)
			return 0;
		pde = base;
		pde |= pde_page_flags(pat);
		(*pd)[(base >> PDE_IDX_SHIFT) & PDE_IDX_MASK] = pde;
		return 0;
	}

	if (size == s4KiB) {
		uint64_t (*pt)[512];
		uint64_t pte;

		pde = (*pd)[(base >> PDE_IDX_SHIFT) & PDE_IDX_MASK];

		/* No page table page allocation. */
		if (!(pde & PDE_PRES)) {
			printk(BIOS_ERR, "Cannot allocate page table for pde %p\n",
				(void *)base);
			return -1;
		}

		/* No splitting pages */
		if (pde & PDE_PS) {
			printk(BIOS_ERR, "Cannot split pde %p\n", (void *)base);
			return -1;
		}

		if (!commit)
			return 0;

		pt = (void *)(uintptr_t)(pde & PDE_ADDR_MASK);
		pte = base;
		pte |= pte_page_flags(pat);
		(*pt)[(base >> PTE_IDX_SHIFT) & PTE_IDX_MASK] = pte;

		return 0;
	}

	return -1;
}

static int _paging_identity_map_addr(uintptr_t base, size_t size, int pat,
					int commit)
{
	while (size != 0) {
		size_t map_size;

		map_size = IS_ALIGNED(base, s2MiB) ? s2MiB : s4KiB;
		map_size = MIN(size, map_size);

		if (identity_map_one_page(base, map_size, pat, commit) < 0)
			return -1;

		base += map_size;
		size -= map_size;
	}

	return 0;
}

static int paging_is_enabled(void)
{
	return !!(read_cr0() & CR0_PG);
}

int paging_identity_map_addr(uintptr_t base, size_t size, int pat)
{
	if (!paging_is_enabled()) {
		printk(BIOS_ERR, "Paging is not enabled.\n");
		return -1;
	}

	if (!IS_ALIGNED(base, s2MiB) && !IS_ALIGNED(base, s4KiB)) {
		printk(BIOS_ERR, "base %p is not aligned.\n", (void *)base);
		return -1;
	}

	if (!IS_ALIGNED(size, s2MiB) && !IS_ALIGNED(size, s4KiB)) {
		printk(BIOS_ERR, "size %zx is not aligned.\n", size);
		return -1;
	}

	/* First try without committing. If success commit. */
	if (_paging_identity_map_addr(base, size, pat, 0))
		return -1;

	return _paging_identity_map_addr(base, size, pat, 1);
}
