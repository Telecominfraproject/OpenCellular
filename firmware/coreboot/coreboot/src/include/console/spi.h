/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 Google Inc.
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

#ifndef CONSOLE_SPI_H
#define CONSOLE_SPI_H 1

#include <rules.h>
#include <stdint.h>
#include <compiler.h>

void spiconsole_init(void);
void spiconsole_tx_byte(unsigned char c);

#define __CONSOLE_SPI_ENABLE__	(IS_ENABLED(CONFIG_SPI_CONSOLE) && \
	(ENV_RAMSTAGE || (ENV_SMM && IS_ENABLED(CONFIG_DEBUG_SMI))))

#if __CONSOLE_SPI_ENABLE__
static inline void __spiconsole_init(void)	{ spiconsole_init(); }
static inline void __spiconsole_tx_byte(u8 data)
{
	spiconsole_tx_byte(data);
}
#else
static inline void __spiconsole_init(void)	{}
static inline void __spiconsole_tx_byte(u8 data)	{}
#endif /* __CONSOLE_SPI_ENABLE__ */

#define MAX_MSG_LENGTH	128

#define EM100_DEDICATED_CMD	0x11
#define EM100_UFIFO_CMD		0xC0
#define EM100_MSG_SIGNATURE	0x47364440

enum em100_message_types {
	EM100_MSG_CHECKPOINT_1B = 0x01,
	EM100_MSG_CHECKPOINT_2B,
	EM100_MSG_CHECKPOINT_4B,
	EM100_MSG_HEX,
	EM100_MSG_ASCII,
	EM100_MSG_TIMESTAMP,
	EM100_MSG_LOOKUP
};

struct em100_msg_header {
	uint8_t		spi_command;
	uint8_t		reserved;
	uint8_t		em100_command;
	uint32_t	msg_signature;
	uint8_t		msg_type;
	uint8_t		msg_length;
} __packed;

struct em100_msg {
	struct em100_msg_header header;
	char data[MAX_MSG_LENGTH];
} __packed;



#endif /* CONSOLE_SPI_H */
