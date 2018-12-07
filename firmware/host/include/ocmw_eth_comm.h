/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCCLI_IPC_COMM_H_
#define _OCCLI_IPC_COMM_H_

/* stdlib includes */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#define OCMW_EC_DEV 1
#define OCMW_EC_STUB_DEV 2

/*
 * Initialize the ocmw ethernet communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_init_eth_comm(int32_t sentDev);
/*
 * Deinitialize the ocmw ethernet communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_deinit_eth_comm(int32_t sentDev);
/*
 * @param cmd an input string (by pointer)
 * @param cmdlen an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_send_eth_msgto_ec(const int8_t *cmd, int32_t cmdlen,
                                      int32_t sentDev);
/*
 * @param resp an input value (by pointer)
 * @param resplen an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_recv_eth_msgfrom_ec(int8_t *resp, int32_t resplen,
                                        int32_t sentDev);
#endif /* _OCCLI_IPC_COMM_H_ */
