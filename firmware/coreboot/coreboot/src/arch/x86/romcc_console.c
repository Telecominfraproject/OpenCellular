/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2003 Eric Biederman
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

#include <build.h>
#include <console/streams.h>
#include <console/early_print.h>
#include <commonlib/loglevel.h>

/* Include the sources. */
#if IS_ENABLED(CONFIG_CONSOLE_SERIAL) && IS_ENABLED(CONFIG_DRIVERS_UART_8250IO)
#include "drivers/uart/util.c"
#include "drivers/uart/uart8250io.c"
#endif
#if IS_ENABLED(CONFIG_CONSOLE_NE2K)
#include "drivers/net/ne2k.c"
#endif

void console_hw_init(void)
{
#if IS_ENABLED(CONFIG_CONSOLE_SERIAL)
	uart_init(CONFIG_UART_FOR_CONSOLE);
#endif
#if IS_ENABLED(CONFIG_CONSOLE_NE2K)
	ne2k_init(CONFIG_CONSOLE_NE2K_IO_PORT);
#endif
}

void console_tx_byte(unsigned char byte)
{
#if IS_ENABLED(CONFIG_CONSOLE_SERIAL)
	uart_tx_byte(CONFIG_UART_FOR_CONSOLE, byte);
#endif
#if IS_ENABLED(CONFIG_CONSOLE_NE2K)
	ne2k_append_data_byte(byte, CONFIG_CONSOLE_NE2K_IO_PORT);
#endif
}

void console_tx_flush(void)
{
#if IS_ENABLED(CONFIG_CONSOLE_SERIAL)
	uart_tx_flush(CONFIG_UART_FOR_CONSOLE);
#endif
#if IS_ENABLED(CONFIG_CONSOLE_NE2K)
	ne2k_transmit(CONFIG_CONSOLE_NE2K_IO_PORT);
#endif
}

#include <console/early_print.c>
#include <console/post.c>
#include <console/die.c>

void console_init(void)
{
	static const char console_test[] =
		"\n\ncoreboot-"
		COREBOOT_VERSION
		COREBOOT_EXTRA_VERSION
		" "
		COREBOOT_BUILD
		" romstage starting...\n";

	console_hw_init();

	print_info(console_test);
}

void die(const char *msg)
{
	print_emerg(msg);
	halt();
}
