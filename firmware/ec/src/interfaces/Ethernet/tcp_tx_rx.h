/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef SRC_INTERFACES_ETHERNET_TCP_TX_RX_H_
#define SRC_INTERFACES_ETHERNET_TCP_TX_RX_H_
Void tcp_Tx_Worker(UArg arg0, UArg arg1);
Void tcp_Rx_Worker(UArg arg0, UArg arg1);
Void tcpHandler(UArg arg0, UArg arg1);
Void tcpHandler_client(UArg arg0, UArg arg1);

#endif /* SRC_INTERFACES_ETHERNET_TCP_TX_RX_H_ */
