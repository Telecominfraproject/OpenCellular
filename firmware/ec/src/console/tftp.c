/*
 * Copyright (c) 2012, Texas Instruments Incorporated
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
 * ======== tftp.c ========
 *
 * TFTP
 *
 */

#include "tftpinc.h"

static int  tftpGetFile( TFTP *pTftp);
static int  tftpSocketSetup( TFTP *pTftp );
static void tftpRRQBuild(TFTP *pTftp);
static void tftpACKBuild(TFTP *pTftp);
static void tftpFlushPackets(TFTP *pTftp);
static int  tftpReadPacket( TFTP *pTftp );
static int  tftpProcessPacket( TFTP *pTftp );
static int  tftpSend( TFTP *pTftp);

/* 
 * tftpSocketSetup()
 *
 * Setup initial socket for TFTP transfer
 */
static int tftpSocketSetup( TFTP *pTftp )
{
    int            rc;   /* Return Code */
    struct timeval timeout;

    /* Create UDP socket */
    pTftp->Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if( pTftp->Sock == INVALID_SOCKET )
    {
        rc = TFTPERROR_SOCKET;
        goto ABORT;
    }

    /* Initialize the bind the local address */
    bzero( &pTftp->tmpaddr, sizeof(struct sockaddr_in) );
    pTftp->tmpaddr.sin_family  = AF_INET;

    /* Assign local name to the unnamed socket */
    /* Do not care about local Host Address or Port (both 0) */
    /* If no error, bind returns 0 */
    if( bind( pTftp->Sock, (PSA)&pTftp->tmpaddr, sizeof(pTftp->tmpaddr) ) < 0 )
    {
        rc = TFTPERROR_SOCKET;
        goto ABORT;
    }

    /* Set the socket IO timeout */
    timeout.tv_sec  = TFTP_SOCK_TIMEOUT;
    timeout.tv_usec = 0;
    if( setsockopt( pTftp->Sock, SOL_SOCKET, SO_SNDTIMEO,
        &timeout, sizeof(timeout)) < 0 )
    {
        rc = TFTPERROR_SOCKET;
        goto ABORT;
    }
    if( setsockopt( pTftp->Sock, SOL_SOCKET, SO_RCVTIMEO,
                    &timeout, sizeof(timeout)) < 0 )
    {
        rc = TFTPERROR_SOCKET;
        goto ABORT;
    }

    /* Initialize the foreign address - we don't use "connect", since */
    /* the peer port will change. We use sendto() */
    bzero( &pTftp->peeraddr, sizeof(struct sockaddr_in) );
    pTftp->peeraddr.sin_family      = AF_INET;
    pTftp->peeraddr.sin_addr.s_addr = pTftp->PeerAddress;
    pTftp->peeraddr.sin_port        = htons(PORT_TFTP);
    rc = 0;

ABORT:
    return(rc);
}

/*
 * tftpRRQBuild()
 * Build an initial file request packet
 */
static void tftpRRQBuild(TFTP *pTftp)
{
    struct tftphdr *RRQ_Packet;
    static char *pszOctet = "octet";
    char   *pRRQ_Data;

    RRQ_Packet = (struct tftphdr *)pTftp->PacketBuffer;

    /* A request packet consists of an opcode (RRQ) followed */
    /* by a NULL terminated filename and mode ("octet") */

    /* Opcode = RRQ */
    RRQ_Packet->opcode = htons(RRQ);

    /* Get a pointer to the rest of the packet */
    pRRQ_Data = (char *)&RRQ_Packet->block;

    /* Copy NULL terminated filename string to request */
    /* increment data pointer by length of Filename (and terminating '0') */
    strcpy(pRRQ_Data, pTftp->szFileName);
    pRRQ_Data += strlen(pTftp->szFileName) + 1;

    /* Copy NULL terminated mode string to request */
    /* increment data pointer by length of mode (and terminating '0') */
    strcpy(pRRQ_Data, pszOctet);
    pRRQ_Data += strlen(pszOctet) + 1;

    /*  calculate length of packet */
    pTftp->Length = (int)(pRRQ_Data - (char *)RRQ_Packet);

    return;
}

/* 
 * tftpACKBuild()
 * Bluild TFTP block ACK packet
 */
static void tftpACKBuild(TFTP *pTftp)
{
    struct tftphdr *ACK_Packet;

    ACK_Packet = (struct tftphdr *)pTftp->PacketBuffer;

    ACK_Packet->opcode = htons(ACK);

    /* Build the rest of the ACK packet */

    /* Block Number */
    ACK_Packet->block = htons( pTftp->NextBlock );

    /*  Set length of packet */
    pTftp->Length = 4;  /*  Opcode + Block */
}

/* 
 * tftpSend()
 * Send pre-built packet to peer
 */
static int tftpSend( TFTP *pTftp)
{
    int rc = 0;
    int BytesSent;

    BytesSent = sendto(pTftp->Sock, pTftp->PacketBuffer,
                       (int)pTftp->Length, 0,(struct sockaddr *)&pTftp->peeraddr,
                       sizeof(pTftp->peeraddr));

    if ( BytesSent != (int)pTftp->Length )
        rc = TFTPERROR_SOCKET;

    return(rc);
}

/*
 * tftpReadPacket()
 * Read a data packet from the peer socket
 * Sets bytes read to ZERO on a timeout
 */
static int tftpReadPacket( TFTP *pTftp )
{
    int                rc = 0;
    int                addrLength;
    struct tftphdr     *ReadBuffer;
    INT32              BytesRead;
    UINT32             TimeStart;

    ReadBuffer = (struct tftphdr *)pTftp->PacketBuffer;
    TimeStart = llTimerGetTime(0);

RETRY:
    /* Don't allow stray traffic to keep us alive */
    if( (TimeStart+TFTP_SOCK_TIMEOUT) <= llTimerGetTime(0) )
    {
        BytesRead = 0;
        goto ABORT;
    }

    /* Attempt to read data */
    addrLength = sizeof(pTftp->tmpaddr);
    BytesRead  = recvfrom( pTftp->Sock, ReadBuffer, DATA_SIZE, 0,
                           (struct sockaddr *)&pTftp->tmpaddr, &addrLength );

    /* Handle read errors first */
    if( BytesRead < 0 )
    {
        /* On a timeout error, ABORT with no error */
        /* Else report error */
        if( fdError() == EWOULDBLOCK )
            BytesRead = 0;
        else
            rc = TFTPERROR_SOCKET;
        goto ABORT;
    }

    /* If this packet is not from our peer, retry */
    if( pTftp->tmpaddr.sin_addr.s_addr != pTftp->PeerAddress )
        goto RETRY;

    /* If the peer port is NOT TFTP, then it must match our expected */
    /* peer. */
    if( pTftp->peeraddr.sin_port != htons(PORT_TFTP) )
    {
        if( pTftp->tmpaddr.sin_port != pTftp->peeraddr.sin_port )
            goto RETRY;
    }

ABORT:
    pTftp->Length = (UINT32)BytesRead;  /*  Store number of bytes read */
    return(rc);
}

/* 
 * tftpFlushPackets()
 * Flush all input from socket
 */
static void tftpFlushPackets(TFTP *pTftp)
{
    int bytesread;

    /*  Sleep for a second */
    TaskSleep( 1000 );

    do
    {
        bytesread = recv( pTftp->Sock, pTftp->PacketBuffer,
                          DATA_SIZE, MSG_DONTWAIT );
    } while( bytesread > 0 );
}

/*
 * tftpReSync()
 *
 * Attempt to ReSync a failed transfer
 */
int tftpReSync( TFTP *pTftp )
{
    int rc = 0;

    /* Fluch pending input packets */
    tftpFlushPackets( pTftp );

    /* Abort if too many Sync errors */
    pTftp->MaxSyncError--;
    if( pTftp->MaxSyncError == 0 )
    {
        rc = TFTPERROR_FAILED;
        goto ABORT;   /* Too Many Sync Errors */
    }

    /* Back up expected block */
    pTftp->NextBlock--;

    /* Resend last packet - if we're on block ZERO, resend the initial */
    /* request. */
    if( !pTftp->NextBlock )
        tftpRRQBuild( pTftp );
    else
        tftpACKBuild( pTftp );

    /* Send the packet */
    rc = tftpSend( pTftp );
    if( rc < 0 )
        goto ABORT;

    pTftp->NextBlock++;  /*  Increment next expected BLOCK */

ABORT:
    return(rc);
}

/*
 * tftpProcessPacket()
 *
 * Processes a data packet obtained from ReadPacket()
 *
 * Returns:
 *      1 - Operation Complete
 *      0 - Operation Progressing
 *     <0 - Error Condition
 */
static int tftpProcessPacket( TFTP *pTftp )
{
    int    rc = 0, done = 0;
    UINT16 OpCode;
    UINT16 ServerBlock;
    UINT32 CopySize;
    struct tftphdr *ReadBuffer;

    ReadBuffer = (struct tftphdr *)pTftp->PacketBuffer;

    /* Check for a bad packet - resynch on error */
    if( !pTftp->Length )
        return( tftpReSync( pTftp ) );

    OpCode = (UINT16) ntohs(ReadBuffer->opcode);
    switch (OpCode)
    {
    case ERROR :
        /* Copy the error code */
        pTftp->ErrorCode = (UINT16) ntohs(ReadBuffer->block);

        /* If the buffer is large enough, copy the error message */
        pTftp->Length -= TFTP_HEADER;          /* Get payload length */

        /* Copy file data if room left in buffer is large enough */
        if( pTftp->BufferSize >= pTftp->Length )
        {
            bcopy( ReadBuffer->data, pTftp->Buffer, (int)pTftp->Length );
            pTftp->BufferUsed = pTftp->Length;
        }
        else
            pTftp->BufferUsed = 0;

        rc = TFTPERROR_ERRORREPLY;
        break;

   case DATA :
        /* Received Data, verify BLOCK correct */
        ServerBlock = (UINT16) ntohs(ReadBuffer->block);

        /* If this is not the block we're expecting, resync */
        if (pTftp->NextBlock != ServerBlock)
        {
            rc = tftpReSync( pTftp );
            pTftp->Length = 0;
            break;
        }

        /* If this is the first block, reset our port number. */
        if( pTftp->NextBlock == 1 )
            pTftp->peeraddr.sin_port = pTftp->tmpaddr.sin_port;  /* Update TID */

        /*  Block is for me! */
        pTftp->MaxSyncError = MAX_SYNC_TRIES;  /* reset Sync Counter */
        pTftp->Length -= TFTP_HEADER;          /* Get payload length */
        CopySize = CopySize;              /* Copy length */
        pTftp->FileSize += CopySize;           /* Track the file length */

        if( (pTftp->BufferUsed+CopySize)<pTftp->BufferSize)
        {
            //Copy the data from buffer to Flash.

            //Send data to External flash.


            //Set buffer used bytes to zero
            pTftp->BufferUsed = 0;
        }
        /* Copy file data if room left in buffer is large enough */
        if( pTftp->BufferSize > pTftp->BufferUsed )
        {
            if( (pTftp->BufferSize - pTftp->BufferUsed) < CopySize)
               CopySize = pTftp->BufferSize - pTftp->BufferUsed;

            if( CopySize )
            {
                /* Add it to our receive buffer */
                bcopy( ReadBuffer->data, (pTftp->Buffer+pTftp->BufferUsed),
                       (int)CopySize );

                /* Track the number of bytes used */
                pTftp->BufferUsed += CopySize;
            }
        }

        /* If we received a partial block, we're done */
        if( pTftp->Length < SEGSIZE )
            done = 1;

        /* Need to acknowledge this block */
        tftpACKBuild( pTftp );
        rc = tftpSend( pTftp );
        if( rc < 0 )
            break;

        /*  Increment next expected BLOCK */
        pTftp->NextBlock++;

        /* Our done flag is the return code on success */
        rc = done;
        break;

    default:
        rc = TFTPERROR_FAILED;
        break;
    }

    return(rc);
}

/* tftpGetFile() */
/* Called to receive a file using TFTP. Called with TFTP parameter */
/* structure initialized. */
/* Returns: */
/*      1 - If file was sucessfully transferred */
/*      0 - If the file was transferred but too large for the buffer */
/*     <0 - Error */
static int tftpGetFile( TFTP *pTftp )
{
    int rc = 0;

    /* Build the request packet */
    tftpRRQBuild( pTftp );

    /* Send the request packet */
    rc = tftpSend( pTftp );
    if( rc < 0 )
       goto ABORT;

    /* Now look for response packets */
    pTftp->MaxSyncError   = MAX_SYNC_TRIES;  /* set sync error counter */
    pTftp->NextBlock      = 1;               /* first block expected is "1" */

    for(;;)
    {
        /* Try and get a reply packet */
        rc = tftpReadPacket( pTftp );
        if( rc < 0 )
            goto ABORT;

        /* Process the reply packet */
        rc = tftpProcessPacket( pTftp );
        if( rc < 0 )
            goto ABORT;

        /* If done, break out of loop */
        if( rc == 1 )
            break;
    }

    /* If the receive buffer was too small, return 0, else return 1 */
    if( pTftp->BufferUsed < pTftp->FileSize )
       rc = 0;
    else
       rc = 1;

ABORT:
    return(rc);
}

/* int NtTftpRecv() */
/* Retrieve a file using TFTP */
/* Return Conditions: */
/* In the following cases, FileSize is set to the actual file size: */
/*      1 - If file was sucessfully transferred */
/*      0 - If the file was transferred but too large for the buffer */
/* In the following cases, FileSize is set to the actual number of */
/* bytes copied. */
/*     <0 - Error */
/*        TFTPERROR_ERRORCODE: TFTP server error code. The error code */
/*          is written to pErrorCode, and an error message is */
/*          written to FileBuffer. The length of the error message */
/*          is written to FileSize. */
int NtTftpRecv( UINT32 TftpIP, char *szFileName, char *FileBuffer,
                UINT32 *FileSize, UINT16 *pErrorCode )
{
    TFTP *pTftp;
    int rc;          /* Return Code */

    /* Quick parameter validation */
    if( !szFileName || !FileSize || (*FileSize != 0 && !FileBuffer) )
        return( TFTPERROR_BADPARAM );

    /* Malloc Parameter Structure */
    if( !(pTftp = mmAlloc( sizeof(TFTP) )) )
        return( TFTPERROR_RESOURCES );

    /* Initialize parameters to "NULL" */
    bzero( pTftp, sizeof(TFTP) );
    pTftp->Sock = INVALID_SOCKET;

    /* Malloc Packet Data Buffer */
    if( !(pTftp->PacketBuffer = mmAlloc( DATA_SIZE )) )
    {
        rc = TFTPERROR_RESOURCES;
        goto ABORT;
    }

    /* store IP in network byte order */
    pTftp->PeerAddress = TftpIP;

    /* Setup initial socket */
    rc = tftpSocketSetup( pTftp );
    if( rc < 0 )
        goto ABORT;

    /*  Socket now registered and available for use. Get the data */
    pTftp->szFileName  = szFileName;
    pTftp->Buffer      = FileBuffer;
    pTftp->BufferSize  = *FileSize;   /* Do not read more than can fit in file */

    /* Get the requested file */
    rc = tftpGetFile( pTftp );
    if( rc < 0 )
    {
        /* ERROR CONDITION */

        /* Set the "FileSize" to the actual number of bytes copied */
        *FileSize = pTftp->BufferUsed;

        /* If the ErrorCode pointer is valid, copy it */
        if( pErrorCode )
            *pErrorCode = pTftp->ErrorCode;
    }
    else
    {
        /* SUCCESS CONDITION */

        /* Set the "FileSize" to the file size (regardless of bytes copied) */
        *FileSize = pTftp->FileSize;
    }

ABORT:
    if( pTftp->Sock != INVALID_SOCKET )
        fdClose( pTftp->Sock );
    if( pTftp->PacketBuffer )
        mmFree( pTftp->PacketBuffer );
    mmFree( pTftp );

    return(rc);
}

