#ifndef _STUB_ETH_COMM_H_
#define _STUB_ETH_COMM_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define OCMW_STUB_ETH_SOCK_DOMAIN (AF_INET)
#define OCMW_STUB_ETH_SOCK_TYPE (SOCK_DGRAM)
#define OCMW_STUB_ETH_SOCK_PROTOCOL (IPPROTO_UDP)
#define OCMW_STUB_ETH_SOCK_SERVER_PORT (2000)
#define OCMW_STUB_ETH_SOCK_SERVER_IP ("127.0.0.1")
#define OCWARE_STUB_ERR_STR_LEN (80)

#endif /* _STUB_ETH_COMM_H_ */
