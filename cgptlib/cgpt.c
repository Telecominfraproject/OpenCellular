/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cgpt.h"
#include <string.h>
#include "gpt.h"
#include "utility.h"

/* stub code */
static int start[] = { 34, 10034 };

int GptInit(GptData_t *gpt) {
  int valid_headers[2] = {1, 1};

  /* check header signature */
  if (Memcmp(gpt->primary_header, GPT_HEADER_SIGNATURE,
                                  GPT_HEADER_SIGNATURE_SIZE))
    valid_headers[0] = 0;
  if (Memcmp(gpt->secondary_header, GPT_HEADER_SIGNATURE,
                                  GPT_HEADER_SIGNATURE_SIZE))
    valid_headers[1] = 0;

  if (!valid_headers[0] && !valid_headers[1])
    return GPT_ERROR_INVALID_HEADERS;

  gpt->current_kernel = 1;
  return GPT_SUCCESS;
}

int GptNextKernelEntry(GptData_t *gpt, uint64_t *start_sector, uint64_t *size) {
  /* FIXME: the following code is not really code, just returns anything */
  gpt->current_kernel ^= 1;
  if (start_sector) *start_sector = start[gpt->current_kernel];
  if (size) *size = 10000;
  return GPT_SUCCESS;
}

int GptUpdateKernelEntry(GptData_t *gpt, uint32_t update_type) {
  /* FIXME: the following code is not really code, just return anything */
  gpt->modified |= (GPT_MODIFIED_HEADER1 | GPT_MODIFIED_ENTRIES1) <<
                   gpt->current_kernel;
  return GPT_SUCCESS;
}
