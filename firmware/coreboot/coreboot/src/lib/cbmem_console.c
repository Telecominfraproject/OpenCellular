/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
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

#include <compiler.h>
#include <console/console.h>
#include <console/cbmem_console.h>
#include <console/uart.h>
#include <cbmem.h>
#include <arch/early_variables.h>
#include <symbols.h>
#include <string.h>

/*
 * Structure describing console buffer. It is overlaid on a flat memory area,
 * with body covering the extent of the memory. Once the buffer is full,
 * output will wrap back around to the start of the buffer. The high bit of the
 * cursor field gets set to indicate that this happened. If the underlying
 * storage allows this, the buffer will persist across multiple boots and append
 * to the previous log.
 *
 * NOTE: These are known implementations accessing this console that need to be
 * updated in case of structure/API changes:
 *
 * cbmem:	[coreboot]/src/util/cbmem/cbmem.c
 * libpayload:	[coreboot]/payloads/libpayload/drivers/cbmem_console.c
 * coreinfo:	[coreboot]/payloads/coreinfo/bootlog_module.c
 * Linux:	drivers/firmware/google/memconsole-coreboot.c
 * SeaBIOS:	src/firmware/coreboot.c
 * GRUB:	grub-core/term/i386/coreboot/cbmemc.c
 */
struct cbmem_console {
	u32 size;
	u32 cursor;
	u8  body[0];
}  __packed;

#define MAX_SIZE (1 << 28)	/* can't be changed without breaking readers! */
#define CURSOR_MASK (MAX_SIZE - 1)	/* bits 31-28 are reserved for flags */
#define OVERFLOW (1UL << 31)		/* set if in ring-buffer mode */
_Static_assert(CONFIG_CONSOLE_CBMEM_BUFFER_SIZE <= MAX_SIZE,
	"cbmem_console format cannot support buffers larger than 256MB!");

static struct cbmem_console *cbmem_console_p CAR_GLOBAL;

#ifdef __PRE_RAM__
/*
 * While running from ROM, before DRAM is initialized, some area in cache as
 * RAM space is used for the console buffer storage. The size and location of
 * the area are defined by the linker script with _(e)preram_cbmem_console.
 */

#else

/*
 * When running from RAM, a lot of console output is generated before CBMEM is
 * reinitialized. This static buffer is used to store that output temporarily,
 * to be concatenated with the CBMEM console buffer contents accumulated
 * during the ROM stage, once CBMEM becomes available at RAM stage.
 */

#if IS_ENABLED(CONFIG_EARLY_CBMEM_INIT)
#define STATIC_CONSOLE_SIZE 1024
#else
#define STATIC_CONSOLE_SIZE CONFIG_CONSOLE_CBMEM_BUFFER_SIZE
#endif
static u8 static_console[STATIC_CONSOLE_SIZE];
#endif

static struct cbmem_console *current_console(void)
{
	return car_sync_var(cbmem_console_p);
}

static void current_console_set(struct cbmem_console *new_console_p)
{
	car_set_var(cbmem_console_p, new_console_p);
}

static int buffer_valid(struct cbmem_console *cbm_cons_p, u32 total_space)
{
	return (cbm_cons_p->cursor & CURSOR_MASK) < cbm_cons_p->size &&
	       cbm_cons_p->size <= MAX_SIZE &&
	       cbm_cons_p->size == total_space - sizeof(struct cbmem_console);
}

static void init_console_ptr(void *storage, u32 total_space)
{
	struct cbmem_console *cbm_cons_p = storage;

	if (!cbm_cons_p || total_space <= sizeof(struct cbmem_console)) {
		current_console_set(NULL);
		return;
	}

	if (!buffer_valid(cbm_cons_p, total_space)) {
		cbm_cons_p->size = total_space - sizeof(struct cbmem_console);
		cbm_cons_p->cursor = 0;
	}

	current_console_set(cbm_cons_p);
}

void cbmemc_init(void)
{
#ifdef __PRE_RAM__
	/* Pre-RAM environments use special buffer placed by linker script. */
	init_console_ptr(_preram_cbmem_console, _preram_cbmem_console_size);
#else
	/* Post-RAM uses static (BSS) buffer before CBMEM is reinitialized. */
	init_console_ptr(static_console, sizeof(static_console));
#endif
}

void cbmemc_tx_byte(unsigned char data)
{
	struct cbmem_console *cbm_cons_p = current_console();

	if (!cbm_cons_p || !cbm_cons_p->size)
		return;

	u32 flags = cbm_cons_p->cursor & ~CURSOR_MASK;
	u32 cursor = cbm_cons_p->cursor & CURSOR_MASK;

	cbm_cons_p->body[cursor++] = data;
	if (cursor >= cbm_cons_p->size) {
		cursor = 0;
		flags |= OVERFLOW;
	}

	cbm_cons_p->cursor = flags | cursor;
}

/*
 * Copy the current console buffer (either from the cache as RAM area or from
 * the static buffer, pointed at by src_cons_p) into the newly initialized CBMEM
 * console. The use of cbmemc_tx_byte() ensures that all special cases for the
 * target console (e.g. overflow) will be handled. If there had been an
 * overflow in the source console, log a message to that effect.
 */
static void copy_console_buffer(struct cbmem_console *src_cons_p)
{
	u32 c;

	if (!src_cons_p)
		return;

	if (src_cons_p->cursor & OVERFLOW) {
		const char overflow_warning[] = "\n*** Pre-CBMEM " ENV_STRING
			" console overflowed, log truncated! ***\n";
		for (c = 0; c < sizeof(overflow_warning) - 1; c++)
			cbmemc_tx_byte(overflow_warning[c]);
		for (c = src_cons_p->cursor & CURSOR_MASK;
		     c < src_cons_p->size; c++)
			cbmemc_tx_byte(src_cons_p->body[c]);
	}

	for (c = 0; c < (src_cons_p->cursor & CURSOR_MASK); c++)
		cbmemc_tx_byte(src_cons_p->body[c]);

	/* Invalidate the source console, so it will be reinitialized on the
	   next reboot. Otherwise, we might copy the same bytes again. */
	src_cons_p->size = 0;
}

static void cbmemc_reinit(int is_recovery)
{
	const size_t size = CONFIG_CONSOLE_CBMEM_BUFFER_SIZE;
	/* If CBMEM entry already existed, old contents are not altered. */
	struct cbmem_console *cbmem_cons_p = cbmem_add(CBMEM_ID_CONSOLE, size);
	struct cbmem_console *previous_cons_p = current_console();

	init_console_ptr(cbmem_cons_p, size);
	copy_console_buffer(previous_cons_p);
}
ROMSTAGE_CBMEM_INIT_HOOK(cbmemc_reinit)
RAMSTAGE_CBMEM_INIT_HOOK(cbmemc_reinit)
POSTCAR_CBMEM_INIT_HOOK(cbmemc_reinit)

#if IS_ENABLED(CONFIG_CONSOLE_CBMEM_DUMP_TO_UART)
void cbmem_dump_console(void)
{
	struct cbmem_console *cbm_cons_p;
	u32 cursor;

	cbm_cons_p = current_console();
	if (!cbm_cons_p)
		return;

	uart_init(0);
	if (cbm_cons_p->cursor & OVERFLOW)
		for (cursor = cbm_cons_p->cursor & CURSOR_MASK;
		     cursor < cbm_cons_p->size; cursor++)
			uart_tx_byte(0, cbm_cons_p->body[cursor]);
	for (cursor = 0; cursor < (cbm_cons_p->cursor & CURSOR_MASK); cursor++)
		uart_tx_byte(0, cbm_cons_p->body[cursor]);
}
#endif
