/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCWGD_DAEMON_H_
#define _OCWGD_DAEMON_H_

/* OC includes */
#include <ocmw_core.h>
#include <ocmw_uart_comm.h>
#include <ocmw_eth_comm.h>

sem_t semEcWdgMsg;
/*
 * Initialize the watchdog daemon
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocwdg_init(void);
/*
 * @param pthreadData an input value (by pointer)
 *
 */
extern void *ocwdg_thread_comm_with_ec(void *pthreadData);

#endif /* _OCWGD_DAEMON_H_ */
