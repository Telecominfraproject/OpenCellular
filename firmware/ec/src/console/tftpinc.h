/*
 * Copyright (c) 2012-2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * */
/*
 * ======== tftpinc.h ========
 *
 * TFTP include
 *
 */

#ifndef _TFTPINC_H
#define _TFTPINC_H

#include <netmain.h>
#include <_stack.h>
#include "tftp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TFTP_HEADER 4
#define DATA_SIZE (SEGSIZE + TFTP_HEADER) /* SEGSIZE declared in TFTP.H as 512 */
#define PORT_TFTP 69

/* structure of a TFTP instance */
typedef struct _tftp
{
    IPN    PeerAddress;             /* Peer address supplied by caller */
    char   *szFileName;             /* Filename supplied by caller */
    char   *Buffer;                 /* Buffer supplied by caller */
    UINT32 BufferSize;              /* Buffer size supplied by caller */
    SOCKET Sock;                    /* Socket used for transfer */
    char   *PacketBuffer;           /* Packet Buffer */
    UINT32 Length;                  /* Length of packet send and reveive */
    UINT32 BufferUsed;              /* Amount of "Buffer" used */
    UINT32 FileSize;                /* Size of specified file */
    UINT16 NextBlock;               /* Next expected block */
    UINT16 ErrorCode;               /* TFTP error code from server */
    int    MaxSyncError;            /* Max SYNC errors remaining */
    struct sockaddr_in tmpaddr;     /* inaddr for RECV */
    struct sockaddr_in peeraddr;    /* inaddr for SEND */

#ifdef _INCLUDE_IPv6_CODE
    IP6N   Peer6Address;            /* Peer address supplied by caller */
    struct sockaddr_in6 tmp6addr;   /* inaddr for RECV */
    struct sockaddr_in6 peer6addr;  /* inaddr for SEND */
#endif
} TFTP;

#define MAX_SYNC_TRIES          4       /* Max retries */
#define TFTP_SOCK_TIMEOUT       10      /* Packet Timeout in Seconds */

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
