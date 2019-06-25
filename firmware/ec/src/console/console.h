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
 * ======== console.h ========
 *
 * Example router console include
 *
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

/* Functions Defined in Console routines */
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSOLE.C */
extern SOCKET ConsoleOpen( PSA pSinClient );
extern void ConsoleClose();

/* The folloing io routines are used by console functions */
extern int  ConPrintf(const char *format, ...);
extern void ConPrintIPN( IPN dwIP );
extern char ConGetCh();
extern int  ConGetString( char *buf, int max, int echo );
#define CGSECHO_NONE     0
#define CGSECHO_INPUT    1
#define CGSECHO_PASSWORD 2
extern IPN  ConGetIP();

/* CONROUTE.C */
extern void ConCmdRoute( int ntok, char *tok1, char *tok2,
                                   char *tok3, char *tok4 );

/* CONACCT.C */
extern void ConCmdAcct( int ntok, char *tok1, char *tok2,
                                  char *tok3, char *tok4 );

/* CONNAT.C */
extern void ConCmdNat( int ntok, char *, char *, char *, char *, char * );

/* CONSTAT.C */
extern void ConCmdStat( int ntok, char *tok1 );

/* CONDNS.C */
extern void ConCmdLookup( int ntok, char *tok1 );
extern int  ConStrToIPN( char *str, IPN *pIPN );

/* CONPING.C */
extern void ConCmdPing( int ntok, char *tok1, char *tok2 );

/* CONECHO.C */
extern void ConCmdEcho( int ntok, char *tok1, char *tok2 );

/* CONSOCK.C */
extern void ConCmdSocket( int ntok, char *tok1 );

/* CONTFTP.C */
extern void ConCmdTFTP( int ntok, char *tok1, char *tok2 );

/* CONTEST.C */
extern void ConCmdTest( int ntok, char *tok1, char *tok2 );

/* VLAN Support is available only if NIMU support is available. */
extern void ConCmdVLAN( int ntok, char *tok1, char *tok2, char *tok3, char* tok4 );
extern void ConCmdIPAddr( int ntok, char *tok1, char *tok2, char *tok3, char* tok4 );

#ifdef _INCLUDE_IPv6_CODE
extern void ConCmdIPv6(int ntok, char *tok1, char *tok2, char *tok3, char* tok4, char* tok5, char* tok6, char* tok7);
extern void ConCmdPing6 ( int ntok, char *tok1, char *tok2 );
extern void ConCmdLookupIPv6( int ntok, char *tok1 );
extern void ConIPv6DisplayIPAddress (IP6N address);
#endif

/* CONLLI.C */
void ConCmdLLI( int ntok, char *tok1, char *tok2, char *tok3 );

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CONSOLE_H */
