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
 * ======== tftp.h ========
 *
 * TFTP includes
 *
 */

#ifndef _TFTP_H_
#define _TFTP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Trivial File Transfer Protocol */
#define SEGSIZE         512     /* data segment size */

/* TFTP Packet types. */
#define RRQ     1               /* read request */
#define WRQ     2               /* write request */
#define DATA    3               /* data packet */
#define ACK     4               /* acknowledgement */
#define ERROR   5               /* error code */

struct tftphdr
{
    short   opcode;             /* packet type */
    short   block;              /* block # */
    char    data[1];            /* data or error string */
};

/* Error codes. */
#define EUNDEF          0       /* not defined */
#define ENOTFOUND       1       /* file not found */
#define EACCESS         2       /* access violation */
#define ENOSPACE        3       /* disk full or allocation exceeded */
#define EBADOP          4       /* illegal TFTP operation */
#define EBADID          5       /* unknown transfer ID */
#define EEXISTS         6       /* file already exists */
#define ENOUSER         7       /* no such user */

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
