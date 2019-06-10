/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/signal.h>
#include <pthread.h>

#include "ocps_uart.h"
#include "util.h"

static int32_t fd = 0;
struct termios oldtio;
static handle_msg_from_yapper_t uart_msg_hndlr = NULL;
static void sa_handler_read(int32_t status);
static bool STOP_UART_RX_SERVICE = true;
static bool STOP_UART_TX_SERVICE = true;
static pthread_t uartrxthread;
static pthread_t uarttxthread;
static volatile bool waitForUartMsg = true;

extern int32_t uartfd[2];
extern int32_t slipfd[2];

/**************************************************************************
 * Function Name    : ocps_yapper_uart_msg_hndlr
 * Description      : This Function used to handle uart messages
 * Input(s)         : msgstr, msgsize
 * Output(s)        :
 ***************************************************************************/
void ocps_yapper_uart_msg_hndlr(const unsigned char* msgstr, int32_t msgsize)
{
    int bufWriteCount = 0;
    /* Forward the UART message received to slip layer */
    //logger("UART RX Char : 0x%x.\n",*msgstr);
    bufWriteCount = write (uartfd[1], msgstr, sizeof(char));
    if (bufWriteCount != 1){
        perror ("Write failed to SIP pipe.");
        exit (2);
    }
    return;
}

/**************************************************************************
 * Function Name    : ocps_trans_uart_msg_service
 * Description      : This Function used to transmit uart messages
 * Input(s)         :
 * Output(s)        : args
 ***************************************************************************/
static void* ocps_trans_uart_msg_service(void* args)
{
    int32_t rc = 0, bufReadCount = 0;
    char ch = 0x00;
    //TODO:: Check where I can enablr this TX thread.
    STOP_UART_TX_SERVICE = false;
    while (!STOP_UART_TX_SERVICE) {
        while(true) {
	    /*Read msg from the slip layer*/
            bufReadCount = read (slipfd[0],&ch,sizeof(char));
            if (bufReadCount != 1) {
                perror("Read failed from SIP pipe.");
                exit(3);
            }
            else {
		/* transmit message over UART to yapper*/ 
                //logger("UART TX CHAR : %c\n", ch);
                ocps_send_uart_msg_to_yapper(&ch,sizeof(char));
            }
        }
    }
    return NULL;
}

/**************************************************************************
 * Function Name    : ocps_recv_uart_msg_service
 * Description      : This Function used to receive uart messages
 * Input(s)         :
 * Output(s)        : args
 ***************************************************************************/
static void* ocps_recv_uart_msg_service(void* args)
{
    int32_t rc = 0;
    char recvChar;
    while (!STOP_UART_RX_SERVICE) {
        /* Recieve UART messages from Yapper */
        if (waitForUartMsg) {
            usleep(10);
            continue;
        }
        waitForUartMsg = true;
	/* Recieve message from UART.*/
	//logger("Starting reading from /dev/ttyO2.\n");
	while(read(fd,&recvChar,sizeof(char))) {
           logger("0x%X\n",recvChar);
	       /* Forward the message to slip decoder */
           if (uart_msg_hndlr != NULL) {
               (*uart_msg_hndlr)(&recvChar, sizeof(char));
           }
        }
	//logger("Nothing to read.\n");
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
    waitForUartMsg = false;
    //logger("Rx Interrupt.\n");
    return;
}

/**************************************************************************
 * Function Name    : ocps_init_yapper_comm
 * Description      : This Function used to initialize the yapper mcu communication
 * Input(s)         : msghndlr
 * Output(s)        :
 ***************************************************************************/
int32_t ocps_init_yapper_comm(handle_msg_from_yapper_t msghndlr)
{
    char pathName[100] = {0};
    int32_t rc = 0;
    struct termios newtio;
    struct sigaction saio;

    memcpy(pathName, (char *) ECTTY, sizeof(ECTTY));

    /*
     * Open the HSUART-2 port for communicating with Yapper
     *
     * O_NOCTTY - Not an interactive console
     * O_NONBLOCK - Read need not be a blocking call
     */

    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        if (fd == -1) {
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
    fcntl(fd, F_SETOWN, getpid());

    /* Set file descriptor to handle asynchronous reads */
    fcntl(fd, F_SETFL, O_ASYNC);

    tcgetattr(fd, &oldtio);
    memset(&newtio, 0, sizeof(newtio));
    cfsetispeed(&newtio, B115200); /* Baud rate = 115200 */
    cfsetospeed(&newtio, B115200);
   #if 0
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
    #endif
    newtio.c_cflag = B115200 | CS8 | CREAD | CLOCAL;
    newtio.c_iflag = IGNPAR | ICRNL;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    STOP_UART_RX_SERVICE = false;
    rc = pthread_create(&uartrxthread, NULL, ocps_recv_uart_msg_service,
                                                                    NULL);
    if (rc != 0) {
        logerr("pthread creation failed [%d-%s]", errno, strerror(errno));
    } else {
        uart_msg_hndlr = msghndlr;
    }

    rc = pthread_create(&uarttxthread, NULL, ocps_trans_uart_msg_service,
                                                                    NULL);
    if (rc != 0) {
        logerr("pthread creation failed [%d-%s]", errno, strerror(errno));
    }

    if (rc != 0) {
        ocps_deinit_yapper_comm();
    }
    return rc;
}

/**************************************************************************
 * Function Name    : ocps_deinit_yapper_comm
 * Description      : This Function used to deinitialize the yapper communication
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
int32_t ocps_deinit_yapper_comm(void)
{
    STOP_UART_RX_SERVICE = true;
    tcsetattr(fd, TCSANOW, &oldtio);
    if (fd > 0) {
        close(fd);
        fd = 0;
    }
    if (uart_msg_hndlr != NULL) {
        uart_msg_hndlr = NULL;
    }
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocps_send_uart_msg_to_yapper
 * Description      : This Function used to send uart message to ec
 * Input(s)         : msgstr, msgsize
 * Output(s)        :
 ***************************************************************************/
int32_t ocps_send_uart_msg_to_yapper(const uint8_t* msgstr, int32_t msgsize)
{
    int32_t rc = 0, bufWriteCount = 0;
    char buf[1] = {0x00};
    memcpy(buf, msgstr, msgsize);
    /* Write message frame to tty */
    bufWriteCount = write(fd,buf, sizeof(char));
    if (bufWriteCount < 0) {
        rc = errno;
        logerr("Error writing to %s (%d - %s)", ECTTY, rc, strerror(errno));
        return -rc;
    }
    return rc;
}
