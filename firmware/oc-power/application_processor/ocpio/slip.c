/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "slip.h"

#define MAX_LENGTH 4096 
static int32_t pktLength = 0;
static bool nextByteSpecial = false;
static bool rxPktStatus = false;

extern uint8_t* rxPkt;

void slip_send_packet(SlipTxChar send_char, uint8_t *packet, int len)
{
     /*Flush the slip packet if any.*/
     send_char(SLIP_END);
     /*Encode packet and send it.*/
     slip_encode(send_char, packet, len);
     /*Send end of packet.*/
     send_char(SLIP_END);
     logger("Transmission complete from slip.\n");
}

void slip_encode(SlipTxChar send_char, uint8_t *packet, int len)
{
      while (len--) {
		      switch (*packet) {
		      /*Send Escape and Escape end character.*/
		      case SLIP_END:
			        send_char(SLIP_ESC);
			        send_char(SLIP_ESC_END);
			        break;
			    /*Send Escape and Escape Escape character.*/
		      case SLIP_ESC:
			        send_char(SLIP_ESC);
			        send_char(SLIP_ESC_ESC);
			        break;
			    /* Send the character*/
		      default:
			        send_char(*packet);
		      }
		      packet++;
	   }
}

int slip_decode(uint8_t *ch)
{
    switch (*ch) {
        case SLIP_END:
	    if (pktLength) {
	        rxPktStatus = true;
            logger("Slip packet length is %d.\n",pktLength);
	    }
	    break;
	    
	case SLIP_ESC:
            nextByteSpecial = true;
	    break;

        default:
            if (nextByteSpecial == true) {
                if (*ch == SLIP_ESC_END) {
                    append_char(SLIP_END);
                } else if (*ch == SLIP_ESC_ESC) {
                    append_char(SLIP_ESC);
                } else {
                    //Do nothing;
                }
            } else {
                append_char(*ch);
            }
            nextByteSpecial = false;
            break;
    }
}

void append_char(uint8_t ch)
{
    if(pktLength < MAX_LENGTH) {
        rxPkt[pktLength++] = ch;
    } else {
        rxPkt = realloc(rxPkt, sizeof(char)*rxPkt[1]);
        if(rxPkt){
            return -1;
        }
    } 
}

void slip_init()
{
    pktLength = 0;
    nextByteSpecial = false;
    rxPktStatus = false;
}

bool slip_recvd_packet_status()
{
     return rxPktStatus;
}
