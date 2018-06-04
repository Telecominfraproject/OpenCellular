#ifndef BOOT_TABLES_H
#define BOOT_TABLES_H

#include <boot/coreboot_tables.h>

/*
 * Write architecture specific tables as well as the common
 * coreboot table.
 */
void write_tables(void);

/*
 * Allow per-architecture table writes called from write_tables(). The
 * coreboot_table parameter provides a reference to where the coreboot
 * table will be written. The parameter is to allow architectures to
 * provide a forwarding table to real coreboot table.
 */
void arch_write_tables(uintptr_t coreboot_table);

#endif /* BOOT_TABLES_H */
