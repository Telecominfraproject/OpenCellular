/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2003 Eric Biederman
 * Copyright (C) 2006-2010 coresystems GmbH
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

#include <rules.h>
#include <stdlib.h>
#include <arch/io.h>
#include <console/uart.h>
#include <trace.h>
#include "uart8250reg.h"

#ifndef __ROMCC__
#include <boot/coreboot_tables.h>
#endif

/* Should support 8250, 16450, 16550, 16550A type UARTs */

/* Expected character delay at 1200bps is 9ms for a working UART
 * and no flow-control. Assume UART as stuck if shift register
 * or FIFO takes more than 50ms per character to appear empty.
 *
 * Estimated that inb() from UART takes 1 microsecond.
 */
#define SINGLE_CHAR_TIMEOUT	(50 * 1000)
#define FIFO_TIMEOUT		(16 * SINGLE_CHAR_TIMEOUT)

static int uart8250_can_tx_byte(unsigned base_port)
{
	return inb(base_port + UART8250_LSR) & UART8250_LSR_THRE;
}

static void uart8250_tx_byte(unsigned base_port, unsigned char data)
{
	unsigned long int i = SINGLE_CHAR_TIMEOUT;
	while (i-- && !uart8250_can_tx_byte(base_port));
	outb(data, base_port + UART8250_TBR);
}

static void uart8250_tx_flush(unsigned base_port)
{
	unsigned long int i = FIFO_TIMEOUT;
	while (i-- && !(inb(base_port + UART8250_LSR) & UART8250_LSR_TEMT));
}

static int uart8250_can_rx_byte(unsigned base_port)
{
	return inb(base_port + UART8250_LSR) & UART8250_LSR_DR;
}

static unsigned char uart8250_rx_byte(unsigned base_port)
{
	unsigned long int i = SINGLE_CHAR_TIMEOUT;
	while (i && !uart8250_can_rx_byte(base_port))
		i--;

	if (i)
		return inb(base_port + UART8250_RBR);
	else
		return 0x0;
}

static void uart8250_init(unsigned base_port, unsigned divisor)
{
	DISABLE_TRACE;
	/* Disable interrupts */
	outb(0x0, base_port + UART8250_IER);
	/* Enable FIFOs */
	outb(UART8250_FCR_FIFO_EN, base_port + UART8250_FCR);

	/* assert DTR and RTS so the other end is happy */
	outb(UART8250_MCR_DTR | UART8250_MCR_RTS, base_port + UART8250_MCR);

	/* DLAB on */
	outb(UART8250_LCR_DLAB | CONFIG_TTYS0_LCS, base_port + UART8250_LCR);

	/* Set Baud Rate Divisor. 12 ==> 9600 Baud */
	outb(divisor & 0xFF,   base_port + UART8250_DLL);
	outb((divisor >> 8) & 0xFF,    base_port + UART8250_DLM);

	/* Set to 3 for 8N1 */
	outb(CONFIG_TTYS0_LCS, base_port + UART8250_LCR);
	ENABLE_TRACE;
}

static const unsigned bases[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };

uintptr_t uart_platform_base(int idx)
{
	if (idx < ARRAY_SIZE(bases))
		return bases[idx];
	return 0;
}

void uart_init(int idx)
{
	if (!IS_ENABLED(CONFIG_DRIVERS_UART_8250IO_SKIP_INIT)) {
		unsigned int div;
		div = uart_baudrate_divisor(get_uart_baudrate(),
			uart_platform_refclk(), uart_input_clock_divider());
		uart8250_init(uart_platform_base(idx), div);
	}
}

void uart_tx_byte(int idx, unsigned char data)
{
	uart8250_tx_byte(uart_platform_base(idx), data);
}

unsigned char uart_rx_byte(int idx)
{
	return uart8250_rx_byte(uart_platform_base(idx));
}

void uart_tx_flush(int idx)
{
	uart8250_tx_flush(uart_platform_base(idx));
}

#if ENV_RAMSTAGE
void uart_fill_lb(void *data)
{
	struct lb_serial serial;
	serial.type = LB_SERIAL_TYPE_IO_MAPPED;
	serial.baseaddr = uart_platform_base(CONFIG_UART_FOR_CONSOLE);
	serial.baud = get_uart_baudrate();
	serial.regwidth = 1;
	serial.input_hertz = uart_platform_refclk();
	serial.uart_pci_addr = CONFIG_UART_PCI_ADDR;
	lb_add_serial(&serial, data);

	lb_add_console(LB_TAG_CONSOLE_SERIAL8250, data);
}
#endif
