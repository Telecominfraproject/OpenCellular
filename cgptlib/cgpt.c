/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgpt.h"

/* stub code */
static int start[] = { 34, 10034 };

int GPTInit(GPTData_t *gpt) {
        gpt->current_kernel = 1;
        return 0;
}

int GPTNextKernelEntry(GPTData_t *gpt, uint64_t *start_sector, uint64_t *size) {
        gpt->current_kernel ^= 1;
        if (start_sector) *start_sector = start[gpt->current_kernel];
        if (size) *size = 10000;
        return 0;
}

int GPTUpdateKernelEntry(GPTData_t *gpt, uint32_t update_type) {
        gpt->modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1) <<
                         gpt->current_kernel;
        return 0;
}
