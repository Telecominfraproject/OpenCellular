/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SPE_H__
#define __SPE_H__

int spe_supported(void);
void spe_enable(int el2_unused);
void spe_disable(void);

#endif /* __SPE_H__ */
