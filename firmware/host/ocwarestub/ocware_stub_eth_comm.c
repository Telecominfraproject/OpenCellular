#include <ocware_stub_eth_comm.h>
#include <ocware_stub_main_module.h>

static int32_t s_stubSockFd;
static struct sockaddr_in s_ocmwStubServer;
/******************************************************************************
 * Function Name    : ocware_stub_init_ethernet_comm
 * Description      : initialise the socket IPC
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_init_ethernet_comm(void)
{
    int32_t rc = 0;
    int32_t inet_atom = 0;

    /* Create socket */
    s_stubSockFd = socket(OCMW_STUB_ETH_SOCK_DOMAIN,
            OCMW_STUB_ETH_SOCK_TYPE, OCMW_STUB_ETH_SOCK_PROTOCOL);
    if (s_stubSockFd < 0) {
        printf("socket creation error [%d-%s]", errno, strerror(errno));
        return STUB_FAILED;
    }

    /* Initialize socket structure */
    memset(&s_ocmwStubServer, 0, sizeof(s_ocmwStubServer));
    s_ocmwStubServer.sin_family = OCMW_STUB_ETH_SOCK_DOMAIN;
    s_ocmwStubServer.sin_port = htons(OCMW_STUB_ETH_SOCK_SERVER_PORT);
    s_ocmwStubServer.sin_addr.s_addr = inet_addr(OCMW_STUB_ETH_SOCK_SERVER_IP);
    inet_atom = inet_aton(OCMW_STUB_ETH_SOCK_SERVER_IP,
				&s_ocmwStubServer.sin_addr);
    if (inet_atom == 0) {
        printf("inet_aton failed");
        return STUB_FAILED;
    }
    memset(s_ocmwStubServer.sin_zero, '\0', sizeof(s_ocmwStubServer.sin_zero));

    /*Bind socket with address struct*/
    rc = bind(s_stubSockFd, (struct sockaddr *) &s_ocmwStubServer,
                                                sizeof(s_ocmwStubServer));
    if (rc != 0) {
        ocware_stub_deinit_ethernet_comm();
        printf("Ehernet init failed \n");
        return STUB_FAILED;
    }

    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : ocware_stub_deinit_ethernet_comm
 * Description      : close the IPC socket
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_deinit_ethernet_comm()
{
    close(s_stubSockFd);
    s_stubSockFd = 0; /* Close the IPC socket */

    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : ocware_stub_send_msgframe_middleware
 * Description      : send message to the MW
 *
 * @param bufferlen - length of the message (by value)
 * @param buffer  - pointer to the message to be sent to MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_send_msgframe_middleware(char **buffer,
                                                        int32_t bufferlen)
{
    int32_t rc = 0;
    const int32_t errstr_len = OCWARE_STUB_ERR_STR_LEN;
    char errstr[errstr_len];
    int8_t data[OC_EC_MSG_SIZE] = { 0 };
    int32_t dataLen = sizeof(s_ocmwStubServer);

    if (buffer == NULL) {
        return STUB_FAILED;
    }

    memcpy(data, *buffer, sizeof(data));

    rc = sendto(s_stubSockFd, data, OC_EC_MSG_SIZE, 0,
                            (struct sockaddr *) &s_ocmwStubServer,
                    dataLen);
    if (rc < 0) {
        strerror_r(errno, errstr, errstr_len);
        printf("Error: 'send' [%d-%s]", errno, errstr);
        return STUB_FAILED;
    }

    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : ocware_stub_recv_msgfrom_middleware
 * Description      : Receive message from MW
 *
 * @param bufferlen - length of the message (by value)
 * @param buffer  - pointer to the location where the message from MW is to be
 *                  stored for further processing (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_recv_msgfrom_middleware(
    char **buffer, int32_t bufferlen)
{
    int32_t rc = 0;
    const int32_t errstr_len = OCWARE_STUB_ERR_STR_LEN;
    char errstr[errstr_len];
    int8_t data[OC_EC_MSG_SIZE] = { 0 };
    int32_t dataLen = sizeof(s_ocmwStubServer);

    if (buffer == NULL) {
        return STUB_FAILED;
    }
    /* Receive the CLI command execution response string from OCMW over UDP socket */
    rc = recvfrom(s_stubSockFd, data, OC_EC_MSG_SIZE,
            0, (struct sockaddr *) &s_ocmwStubServer,
				(socklen_t*)&dataLen);
    if (rc < 0) {
        strerror_r(rc, errstr, errstr_len);
        logerr("Error: 'recv' [%d-%s]", rc, errstr);
        return STUB_FAILED;
    }

    memcpy(*buffer, data, sizeof(data));
    return STUB_SUCCESS;
}
