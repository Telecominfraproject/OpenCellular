/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <util.h>
#include <logger.h>

/* OC includes */
#include <ocmw_core.h>
#include <ocmw_uart_comm.h>
#include <ocmp_frame.h>

static int32_t s_fd = 0;
struct termios s_oldtio;
static handle_msg_from_ec_t s_uart_msg_hndlr = NULL;
static void sa_handler_read(int32_t status);
static bool S_STOP_UART_RX_SERVICE = true;
static pthread_t s_uartrxthread;
static volatile bool s_waitForUartMsg = true;
extern uint8_t mcuMsgBuf[OCMP_MSG_SIZE];

/**************************************************************************
 * Function Name    : ocmw_ec_uart_msg_hndlr
 * Description      : This Function used to handle uart messages
 * Input(s)         : msgstr, msgsize
 * Output(s)        :
 ***************************************************************************/
void ocmw_ec_uart_msg_hndlr(const unsigned char* msgstr, int32_t msgsize)
{
    /* Process the OC message received from EC  */
    if (msgsize > OCMP_MSG_SIZE) {
        logerr("message size is more than %d bytes", OCMP_MSG_SIZE);
        return;
    }

    /* Store the uart msg into global buffer */
    memcpy(mcuMsgBuf, msgstr, msgsize);

    /* Signal the msg parser for ec uart msg */
    sem_post(&semecMsgParser);

    return;
}

/**************************************************************************
 * Function Name    : ocmw_recv_uart_msg_service
 * Description      : This Function used to receive uart messages
 * Input(s)         :
 * Output(s)        : args
 ***************************************************************************/
static void* ocmw_recv_uart_msg_service(void* args)
{
    unsigned char msg[OCMP_MSG_SIZE] = {0};
    unsigned char buf[OCMP_MSG_SIZE] = {0};
    int32_t ret = 0;
    const unsigned int msglen = OCMP_MSG_SIZE;

    while (!S_STOP_UART_RX_SERVICE) {
        /* Receive UART messages from EC */
        if (s_waitForUartMsg) {
            usleep(10);
            continue;
        }
        s_waitForUartMsg = true;

        /* Read  message frame from tty */
        memset(msg, 0, msglen);
        memset(buf, 0, OCMP_MSG_SIZE);
        ret = read(s_fd, msg, msglen);
        if (ret < 0) {
            logerr("Error reading from %s (%d - %s)", ECTTY, errno,
                    strerror(errno));
            continue;
        }
        if (memcmp(msg, buf, OCMP_MSG_SIZE) == 0) {
            continue; /* Not a valid UART message */
        }

        /* printf("Data from EC:\n"); */
        /* hexdisp(msg, msglen); */
        if (s_uart_msg_hndlr != NULL) {
            (*s_uart_msg_hndlr)(msg, msglen);
        }
    }
    return NULL;
}

/**************************************************************************
 * Function Name    : sa_handler_read
 * Description      : This Function used to read the handler
 * Input(s)         : status
 * Output(s)        :
 ***************************************************************************/
static void sa_handler_read(int32_t status)
{
    s_waitForUartMsg = false;
    return;
}

/**************************************************************************
 * Function Name    : ocmw_init_ec_comm
 * Description      : This Function used to initialize the ec communication
 * Input(s)         : msghndlr
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_init_ec_comm(handle_msg_from_ec_t msghndlr)
{
    char pathName[100] = {0};
    int32_t ret = 0;
    struct termios newtio;
    struct sigaction saio;

    memset(pathName, 0, sizeof(pathName));
#ifdef  INTERFACE_USB
    ret = ocmw_find_ttyacm_port(pathName);
    if (ret != 0) {
        return ret;
    }
#else
    memcpy(pathName, (char *) ECTTY, sizeof(ECTTY));
#endif
    /*
     * Open the HSUART-1 port for communicating with EC
     *
     * O_NOCTTY - Not an interactive console
     * O_NONBLOCK - Read need not be a blocking call
     */
    s_fd = open(pathName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (s_fd == -1) {
        if (s_fd == -1) {
            logerr("Error opening %s (%d - %s)", ECTTY, errno,
                    strerror(errno));
            return -1;
        }
    }

    /* Install signal handler for asynchronous read */
    saio.sa_handler = sa_handler_read;
    sigemptyset(&saio.sa_mask);
    saio.sa_flags = SA_RESTART;
    saio.sa_restorer = NULL;
    sigaction(SIGIO, &saio, NULL);

    /* Allow this process to receive SIGIO */
    fcntl(s_fd, F_SETOWN, getpid());

    /* Set file descriptor to handle asynchronous reads */
    fcntl(s_fd, F_SETFL, O_ASYNC);

    tcgetattr(s_fd, &s_oldtio);
    memset(&newtio, 0, sizeof(newtio));
    cfsetispeed(&newtio, B115200); /* Baud rate = 115200 */
    cfsetospeed(&newtio, B115200);
    newtio.c_cflag &= ~PARENB; /* No parity check */
    newtio.c_cflag &= ~CSTOPB; /* 1 STOP bit */
    newtio.c_cflag &= ~CSIZE; /* 8 DATA bits */
    newtio.c_cflag |= CS8;
    newtio.c_cflag &= ~CRTSCTS; /* No HW flow control */
    /* Enable always. CREAD - Enable receiver.
     * CLOCAL - program doesn't own the tty. */
    newtio.c_cflag |= (CLOCAL | CREAD);
    newtio.c_cc[VTIME] = 10; /* Inter character TIME=t*0.1s, where t=10. */
    newtio.c_cc[VMIN] = 0; //33; /* EC message frame size = 32 bytes */
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY); /* No SW flow control */
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    newtio.c_oflag &= ~OPOST;
    tcflush(s_fd, TCIFLUSH);
    tcsetattr(s_fd, TCSANOW, &newtio);

    S_STOP_UART_RX_SERVICE = false;
    ret = pthread_create(&s_uartrxthread, NULL, ocmw_recv_uart_msg_service,
                                                                    NULL);
    if (ret != 0) {
        logerr("pthread creation failed [%d-%s]", errno, strerror(errno));
    } else {
        s_uart_msg_hndlr = msghndlr;
    }

    if (ret != 0) {
        ocmw_deinit_ec_comm();
    }
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_deinit_ec_comm
 * Description      : This Function used to deinitialize the ec communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_deinit_ec_comm(void)
{
    S_STOP_UART_RX_SERVICE = true;
    tcsetattr(s_fd, TCSANOW, &s_oldtio);
    if (s_fd > 0) {
        close(s_fd);
        s_fd = 0;
    }
    if (s_uart_msg_hndlr != NULL) {
        s_uart_msg_hndlr = NULL;
    }
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocmw_send_uart_msg_to_ec
 * Description      : This Function used to send uart message to ec
 * Input(s)         : msgstr, msgsize
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_send_uart_msg_to_ec(const uint8_t* msgstr, int32_t msgsize)
{
    uint8_t buf[OCMP_MSG_SIZE] = {0};
    int32_t ret = 0, bufWriteCount = 0;
    int32_t sendPktNonpayloadSize = 0;
    int32_t loopCount = 0;
    OCMPMessageFrame *ecMsgFrame;

    ecMsgFrame = (OCMPMessageFrame *) msgstr;

    sendPktNonpayloadSize = (sizeof(OCMPMessage) - sizeof(void *)
            + sizeof(OCMPHeader));

    if (msgstr == NULL) {
        logerr("Error: Memory allocation problem");
        return -1;
    }

    if (msgsize > OCMP_MSG_SIZE) {
        logerr("Error: msgstr size is more than %d bytes", OCMP_MSG_SIZE);
        return -EMSGSIZE;
    }

    /* create message frame */
    memset(buf, 0, sizeof(buf));
    memcpy(buf, msgstr, sendPktNonpayloadSize);
    memcpy(&buf[sendPktNonpayloadSize], ((ecMsgFrame->message).info),
            MAX_PARM_COUNT);

    /* Write message frame to tty */
    bufWriteCount = write(s_fd, buf, sizeof(buf));
    if (bufWriteCount < 0) {
        ret = errno;
        logerr("Error writing to %s (%d - %s)", ECTTY, ret, strerror(errno));
        return -ret;
    }

#ifndef MSG_LOG
    printf(" \n send_msg_to_ec \n");

    for (loopCount = 0; loopCount < OCMP_MSG_SIZE; loopCount++) {
        printf("0x%x  ", buf[loopCount] & 0x00ff);
    }

    printf("\n");
#endif /* MSG_LOG */

    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_find_ttyacm_port
 * Description      : This Function used to find the tty port
 * Input(s)         :
 * Output(s)        : pathName
 ***************************************************************************/
int32_t ocmw_find_ttyacm_port(char *pathName)
{
    FILE *fp;
    int32_t nodeNum = -1;
    int32_t ret = 0;

    if (pathName == NULL) {
        return ret = -1;
    }

    /* Open the command for reading. */
    fp = popen(ECTTY, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return ret = -1;
    }

    fscanf(fp, "%d", &nodeNum);
    if (nodeNum < 0) {
        /* close */
        pclose(fp);
        printf(" No ttyACM port found \n");
        return ret = -1;
    } else {
        sprintf(pathName, "/dev/ttyACM%d", nodeNum);
        printf(" ttyACM port = %s \n", pathName);
        /* close */
        pclose(fp);
    }
    return ret;
}
