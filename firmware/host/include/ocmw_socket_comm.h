/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCMW_SOCK_COMM_H_
#define _OCMW_SOCK_COMM_H_

#define OCMW_SOCK_SERVER_IP ("127.0.0.1")
#define OCMW_SOCK_SERVER_PORT (5000)
#define OCMW_SOCK_SERVER_ALERT_PORT (6000)

#define OCMW_SOCK_SERVER_CONN_COUNT (1)
#define OCMW_SOCK_SERVER_ALERT_CONN_COUNT (2)

#define OCMW_SOCK_DOMAIN (AF_INET)
#define OCMW_SOCK_TYPE (SOCK_DGRAM)
#define OCMW_SOCK_PROTOCOL (IPPROTO_UDP)

#define OCMW_SOCK_ALERT_SERVER_IP ("127.0.0.1")
#define OCMW_SOCK_ALERT_SERVER_PORT (6000)
#define OCMW_SOCK_ALERT_DOMAIN (AF_INET)
#define OCMW_SOCK_ALERT_TYPE (SOCK_DGRAM)
#define OCMW_SOCK_ALERT_PROTOCOL (IPPROTO_UDP)

#define OCMW_ETH_SOCK_SERVER_IP ("192.168.1.2")
#define OCMW_ETH_SOCK_SERVER_PORT (1000)
#define OCMW_ETH_SOCK_DOMAIN (AF_INET)
#define OCMW_ETH_SOCK_TYPE (SOCK_STREAM)
#define OCMW_ETH_SOCK_PROTOCOL (IPPROTO_TCP)
#define OCMW_SOCKET_ERROR_SIZE 256

#define OCMW_SOCK_STUB_SERVER_IP ("127.0.0.1")
#define OCMW_SOCK_STUB_SERVER_PORT (2000)
#define OCMW_SOCK_STUB_DOMAIN (AF_INET)
#define OCMW_SOCK_STUB_TYPE (SOCK_DGRAM)
#define OCMW_SOCK_STUB_PROTOCOL (IPPROTO_UDP)
#endif /* _OCMW_SOCK_COMM_H_ */
