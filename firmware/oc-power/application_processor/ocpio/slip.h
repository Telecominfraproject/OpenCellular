/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "ocps_slip.h"
#include "util.h" 

/* SLIP special character codeC
 */
#define SLIP_END             ((uint8_t)0xC0)    /* indicates end of packet */
#define SLIP_ESC             ((uint8_t)0xDB)    /* indicates byte stuffing */
#define SLIP_ESC_END         ((uint8_t)0xDC)    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         ((uint8_t)0xDD)    /* ESC ESC_ESC means ESC data byte */

void slip_init();
bool slip_recvd_packet_status();
void append_char(uint8_t ch);
int slip_decode(uint8_t *ch);
void slip_send_packet(SlipTxChar tx_char, uint8_t *packet, int len);
void slip_encode(SlipTxChar tx_char, uint8_t *packet, int len);
