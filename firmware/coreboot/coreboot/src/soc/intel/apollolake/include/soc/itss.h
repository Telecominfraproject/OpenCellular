/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#ifndef _SOC_APOLLOLAKE_ITSS_H_
#define _SOC_APOLLOLAKE_ITSS_H_

#define GPIO_IRQ_START	50
#define GPIO_IRQ_END	ITSS_MAX_IRQ

#define ITSS_MAX_IRQ	119
#define IRQS_PER_IPC	32
#define NUM_IPC_REGS	((ITSS_MAX_IRQ + IRQS_PER_IPC - 1)/IRQS_PER_IPC)

#endif /* _SOC_APOLLOLAKE_ITSS_H_ */
