/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 Jonathan Neuschäfer
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

#include <arch/io.h>
#include <boot/coreboot_tables.h>
#include <console/uart.h>
#include <types.h>

/*
 * This is a driver for SiFive's own UART, documented in the FU540 manual:
 * https://www.sifive.com/documentation/chips/freedom-u540-c000-manual/
 */

struct sifive_uart_registers {
	uint32_t txdata;	/* Transmit data register */
	uint32_t rxdata;	/* Receive data register */
	uint32_t txctrl;	/* Transmit control register */
	uint32_t rxctrl;	/* Receive control register */
	uint32_t ie;		/* UART interrupt enable */
	uint32_t ip;		/* UART interrupt pending */
	uint32_t div;		/* Baud rate divisor */
} __packed;

#define TXDATA_FULL		BIT(31)
#define RXDATA_EMPTY		BIT(31)
#define TXCTRL_TXEN		BIT(0)
#define TXCTRL_NSTOP_SHIFT	1
#define TXCTRL_NSTOP(x)		(((x)-1) << TXCTRL_NSTOP_SHIFT)
#define TXCTRL_TXCNT_SHIFT	16
#define TXCTRL_TXCNT(x)		((x) << TXCTRL_TXCNT_SHIFT)
#define RXCTRL_RXEN		BIT(0)
#define RXCTRL_RXCNT_SHIFT	16
#define RXCTRL_RXCNT(x)		((x) << RXCTRL_RXCNT_SHIFT)
#define IP_TXWM			BIT(0)
#define IP_RXWM			BIT(1)

void uart_init(int idx)
{
	struct sifive_uart_registers *regs = uart_platform_baseptr(idx);

	/* TODO: Configure the divisor */

	/* Enable transmission, one stop bit, transmit watermark at 1 */
	write32(&regs->txctrl, TXCTRL_TXEN|TXCTRL_NSTOP(1)|TXCTRL_TXCNT(1));

	/* Enable reception, receive watermark at 0 */
	write32(&regs->rxctrl, RXCTRL_RXEN|RXCTRL_RXCNT(0));
}

static bool uart_can_tx(struct sifive_uart_registers *regs)
{
	return !(read32(&regs->txdata) & TXDATA_FULL);
}

void uart_tx_byte(int idx, unsigned char data)
{
	struct sifive_uart_registers *regs = uart_platform_baseptr(idx);

	while (!uart_can_tx(regs))
		; /* TODO: implement a timeout */

	write32(&regs->txdata, data);
}

void uart_tx_flush(int idx)
{
	struct sifive_uart_registers *regs = uart_platform_baseptr(idx);
	uint32_t ip;

	/* Use the TX watermark bit to find out if the TX FIFO is empty */
	do {
		ip = read32(&regs->ip);
	} while (!(ip & IP_TXWM));
}

unsigned char uart_rx_byte(int idx)
{
	struct sifive_uart_registers *regs = uart_platform_baseptr(idx);
	uint32_t rxdata;

	do {
		rxdata = read32(&regs->rxdata);
	} while (rxdata & RXDATA_EMPTY);

	return rxdata & 0xff;
}

unsigned int uart_input_clock_divider(void)
{
	/*
	 * The SiFive UART handles oversampling internally. The divided clock
	 * is the baud clock.
	 */
	return 1;
}

#ifndef __PRE_RAM__
void uart_fill_lb(void *data)
{
	/* TODO */
}
#endif
