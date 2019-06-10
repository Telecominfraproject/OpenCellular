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
#include <string.h>

#include "ocps_slip.h"
#include "slip.h"
#include "util.h"

#define VALID_LENGTH(x)     (((x>0)&&(x<MAX_LENGTH))?true:false)

static bool SLIP_TX_PACKET_AVAILABLE = false;

static pthread_t sliprxthread;
static pthread_t sliptxthread;

static uint8_t pktStr[MAX_LENGTH];
static uint8_t pktSize;

static uint8_t* txPkt;
uint8_t* rxPkt;

extern int32_t uartfd[2];
extern int32_t slipfd[2];

static int32_t transmit_to_uart(char ch);

CallbackHandler cb_bsp_handler;

SlipTxChar slip_tx_char = transmit_to_uart;

#define UTIL_NUMBER_ZERO    0

void hexdisp(const unsigned char* buf, int buflen)
{
    int c=UTIL_NUMBER_ZERO, i=UTIL_NUMBER_ZERO;
    for (c=UTIL_NUMBER_ZERO; c<buflen; c++) {
        printf("0x%02x ", buf[c]);
        if ((c+1)%8 == UTIL_NUMBER_ZERO) {
            printf("\t");
            for(i=(c+1)-8; i<=(c+1); i++)
                printf("%c", (buf[i]>0x20 && buf[i]<0x7F) ? buf[i] : '.');
            printf("\n");
        }
    }
    printf("\n");
}


static int32_t transmit_to_uart(char ch)
{
  int bufWriteCount = 0;
  /* Forward the UART message received to slip layer */
  bufWriteCount = write(slipfd[1], &ch,sizeof(char));
  if (bufWriteCount != 1){
      perror ("Write failed to UART pipe.");
      exit (2);
  }
  //logger("Slip::Slip to UART thread transmitted %c.\n",ch);
  return bufWriteCount;
}

static int32_t receive_from_uart(char *ch)
{
  int32_t rc = 0, bufReadCount = 0;
  bufReadCount = read(uartfd[0],ch,sizeof(char));
  if (bufReadCount != 1) {
      perror("Read failed from UART pipe.");
      exit(3);
  }
  return bufReadCount;
}

/**************************************************************************
 * Function Name    : ocps_slip_trans_msg_hndlr
 * Description      : This Function used to handle uart messages
 * Input(s)         : msgstr, msgsize
 * Output(s)        :
 ***************************************************************************/
static void* ocps_slip_trans_msg_hndlr(void* args)
{
    //TODO:: if I need some queue or something.
    logger("Slip::Starting the Transmit thread for slip.\n");
    while (true) {
    	while(SLIP_TX_PACKET_AVAILABLE) {
            logger("Slip::Started Slip encoding of the packet of size %d.\n",pktSize);
            slip_send_packet(slip_tx_char, txPkt, pktSize);
            if(txPkt) {
                 logger("Slip:: Freed slip encoded TX buffer.\n");
                 free(txPkt);
                 txPkt = NULL;
            }
            SLIP_TX_PACKET_AVAILABLE = false;
        }
	usleep(10);
    }
}

/**************************************************************************
 * Function Name    : ocps_slip_recv_msg_hndlr
 * Description      : This Function used to reveive uart messages
 * Input(s)         :
 * Output(s)        : args
 ***************************************************************************/
static void* ocps_slip_recv_msg_hndlr(void* args)
{
    int32_t rc = 0;
    char ch = 0x00;
    logger("Slip::Starting the Receive thread for slip.\n");
    while(true) {
        if(slip_recvd_packet_status() == true) {
            //TODO: Message recei`ved.
            //Add message handler.
	        logger("Slip::Received a packet.\n");
            hexdisp(rxPkt,64);
	        (*cb_bsp_handler)(rxPkt);
            if(rxPkt){
                 logger("Slip:: Freed slip decoded RX buffer.\n");
                free(rxPkt);
		        rxPkt = NULL;
            }
            slip_init();
        }
        if(receive_from_uart(&ch)) {
            //logger("Slip::SLIP Reader received char : 0x%X from UART.\n", ch);
	    if (!rxPkt) {
	    	rxPkt = malloc(sizeof(char)*MAX_LENGTH);
	    	if (!rxPkt) {
			return NULL;
	    	}
	    }
            slip_decode(&ch);
        }
    }
    return NULL;
}

void ocps_slip_send_packet(uint8_t* buffer, int32_t packetLength)
{
    logger("Slip:: Allocating TX buffer of length %d.\n",packetLength);   
    txPkt = malloc(sizeof(char)*packetLength);
      if(txPkt) {
          memcpy(txPkt,buffer,packetLength);
          pktSize = packetLength;
          SLIP_TX_PACKET_AVAILABLE = true;
          logger("Slip::Enable Transmit for Slip packet of size %d.\n",packetLength);
      }
}

int ocps_slip_init(CallbackHandler cb_bsp)
{
    int32_t rc = 0;

    cb_bsp_handler = cb_bsp;

    rc = pthread_create(&sliprxthread, NULL, ocps_slip_recv_msg_hndlr,
                                                                  NULL);
    if (rc != 0) {
        logerr("pthread creation failed [%d-%s] for slip recieve handling.", errno, strerror(errno));
     }

    rc = pthread_create(&sliptxthread, NULL, ocps_slip_trans_msg_hndlr,
                                                                    NULL);
    if (rc != 0) {
        logerr("pthread creation failed [%d-%s] for slip tarnsmit handling", errno, strerror(errno));
    }
    logger("Slip::Initializing Slip protocol.\n");
}
