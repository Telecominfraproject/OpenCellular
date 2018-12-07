/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCMW_UART_COMM_H_
#define _OCMW_UART_COMM_H_

#ifdef INTERFACE_USB
#    define ECTTY "find /dev/ -iname \"ttyACM*\" | tr -cd [:digit:]"
#else
#    define ECTTY "/dev/ttyS4"
#endif
#define OCMP_MSG_SIZE (64)

/*
 * @param msgstr an input value (by pointer)
 * @param msgsize an input value (by value)
 *
 * @return the function handler
 */
typedef void (*handle_msg_from_ec_t)(const unsigned char *msgstr,
                                     int32_t msgsize);
/*
 * @param msgstr an input value (by pointer)
 * @param msgsize an input value (by value)
 *
 */
extern void ocmw_ec_uart_msg_hndlr(const unsigned char *msgstr,
                                   int32_t msgsize);
/*
 * Initialize the ocmw ec communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_init_ec_comm(handle_msg_from_ec_t msghndlr);
/*
 * Deinitialize the ocmw ec communication
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_deinit_ec_comm(void);
/*
 * @param msgstr an input value (by pointer)
 * @param size an input value (by value)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_send_uart_msg_to_ec(const uint8_t *msgstr, int32_t size);
/*
 * @param pathName an input value (by pointer)
 *
 * @return true if function succeeds, false otherwise
 */
extern int32_t ocmw_find_ttyacm_port(char *pathName);

#endif /* _OCMW_UART_COMM_H_ */
