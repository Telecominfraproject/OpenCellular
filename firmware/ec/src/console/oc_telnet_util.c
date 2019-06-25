/*
 * oc_telnet_util.c
 *
 *  Created on: Apr 26, 2019
 *      Author: vthakur
 */

#include <netmain.h>
#include <_stack.h>
#include "console.h"
#include <stdbool.h>
/*------------------------------------------------------------------------- */
/* ConCmdTFTP() */
/* Function to perform TFTP */
/*------------------------------------------------------------------------- */

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Queue.h>

#include "inc/common/global_header.h"
#include "src/filesystem/fs_wrapper.h"

void openCellular_banner(void)
{
    ConPrintf(
        "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    ConPrintf(
        "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    ConPrintf(
        "||||        ||||        ||||       |||||    ||||||  ||||       ||||       ||||  |||||||||  |||||||||  ||||  ||||  |||||||||        ||||        ||||\n");
    ConPrintf(
        "||||  ||||  ||||  ||||  ||||  ||||||||||  |  |||||  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  ||||  ||||\n");
    ConPrintf(
        "||||  ||||  ||||  ||||  ||||  ||||||||||  ||  ||||  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  ||||  ||||\n");
    ConPrintf(
        "||||  ||||  ||||        ||||       |||||  |||  |||  ||||  |||||||||       ||||  |||||||||  |||||||||  ||||  ||||  |||||||||        ||||        ||||\n");
    ConPrintf(
        "||||  ||||  ||||  ||||||||||  ||||||||||  ||||  ||  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  ||  ||||||\n");
    ConPrintf(
        "||||  ||||  ||||  ||||||||||  ||||||||||  |||||  |  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  |||  |||||\n");
    ConPrintf(
        "||||        ||||  ||||||||||       |||||  ||||||    ||||       ||||       ||||       ||||       ||||        ||||       ||||  ||||  ||||  ||||  ||||\n");
    ConPrintf(
        "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    ConPrintf(
        "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    ConPrintf("\nOCWare v" xstr(_FW_REV_MAJOR_) "." xstr(_FW_REV_MINOR_) "." xstr(_FW_REV_BUGFIX_) "-" xstr(_FW_REV_TAG_) "\n");
    ConPrintf("Build Date: " __DATE__ " " __TIME__ "\n\n");
}
void __dumpTask(Task_Handle task)
{
    Task_Stat stat;

    Task_stat(task, &stat);
    ConPrintf("0x%08x: %32s %12d %12d %12d %12d\n", task, Task_Handle_name(task), stat.priority,
                  stat.mode, stat.stackSize, stat.used);
}

void __listTasks()
{
    Task_Object * task;
    Int i;

    for (i = 0; i < Task_Object_count(); i++) {
        task = Task_Object_get(NULL, i);
        __dumpTask(task);
    }

    task = Task_Object_first();
    while (task) {
        __dumpTask(task);
        task = Task_Object_next(task);
    }
}

void __dumpSemaphore(Task_Handle task)
{
    Task_Stat stat;

    Task_stat(task, &stat);
    ConPrintf("0x%08x: %32s %12d %12d %12d %12d\n", task, Task_Handle_name(task), stat.priority,
                  stat.mode, stat.stackSize, stat.used);
}

void __dumpQueue(Queue_Object* queue)
{
    ConPrintf("0x%08 %32s %32s\n",queue, Queue_Handle_name(queue), (((Queue_empty(queue)==true)==true)?"Empty":"Elements pending for processing."));
}

void __listMessageQueue()
{
    Queue_Object * queue;
    Int i;

    for (i = 0; i < Queue_Object_count(); i++) {
        queue = Queue_Object_get(NULL, i);
        __dumpQueue(queue);
    }

    queue = Queue_Object_first();
    while (queue) {
        __dumpQueue(queue);
        queue = Queue_Object_next(queue);
    }
}

void __listSemaphore()
{
    Task_Object * task;
    Int i;

    for (i = 0; i < Task_Object_count(); i++) {
        task = Task_Object_get(NULL, i);
        __dumpTask(task);
    }

    task = Task_Object_first();
    while (task) {
        __dumpTask(task);
        task = Task_Object_next(task);
    }
}

/*------------------------------------------------------------------------- */
/* __load_file */
/*------------------------------------------------------------------------- */
static void __load_file( IPN IPAddr, char *File )
{
    int    rc;
    char   *buffer;
    UINT16 ErrorCode;
    UINT32 Size;

    buffer = mmAlloc(3000);
    if( !buffer )
    {
        ConPrintf("\nFailed allocating temp buffer\n");
        return;
    }

    Size = 3000;
    rc = NtTftpRecv( IPAddr, File, buffer, &Size, &ErrorCode );

    if( rc >= 0 )
    {
        UINT32 i;
        int    c;

        ConPrintf("\nFile Retrieved: Size is %d\n",Size);

        if( !rc )
            Size = 3000;

        ConPrintf("\nDisplay (%d bytes) (y/n)\n",Size);
        do { c=ConGetCh(); }
            while( c != 'y' && c !='Y' && c != 'N' && c != 'n' );
        if( c=='Y' || c=='y' )
            for( i=0; i<Size; i++ )
                ConPrintf( "%c", *(buffer+i) );

        ConPrintf("\n");
    }
    else if( rc < 0 )
    {
        ConPrintf("\nTFTP Reported Error: %d\n",rc);
        if( rc == TFTPERROR_ERRORREPLY )
            ConPrintf("TFTP Server Error: %d (%s)\n",ErrorCode,buffer);
    }

    mmFree( buffer );
}


void util_show_ec_logs()
{
    ConPrintf("\n[ Show EC Logs ]\n");
}

void util_show_ec_alerts()
{
    ConPrintf("\n[ Show EC Alerts ]\n");
}

void util_show_task_stat( )
{
    ConPrintf("\n[ Show task statics ]\n");
    ConPrintf("%10s: %32s %12s %12s %12s %12s\n", "Address", "Name", "Priority",
                      "Mode", "StackSize", "Stack Used");
    __listTasks();
}

void util_show_msg_queue_stat( )
{
    ConPrintf("\n[ Show Message Queue statics ]\n");
    ConPrintf("%10s: %32s %32s\n", "Address", "Name", "Elements");
    __listMessageQueue();
}

void  util_show_ap_console_logs()
{
    ConPrintf("\n[ Console Logs ]\n");
    char dummypayload[1] = {'\0'};
    FILESystemStruct fileSysStruct = {
                                      "apConsoleLogs",          1,
                                      1,   dummypayload,
                                      100000, CONSOLE_LOG, 0
    };
    uint8_t *payload = malloc(sizeof(FILESystemStruct));
    if(payload) {
        memcpy(payload,&fileSysStruct,sizeof(fileSysStruct));
        Util_enqueueMsg(fsRxMsgQueue, semFilesysMsg, payload);
    } else {
        ConPrintf("\n[ Console Logs loading failed. Use showMem for memory. ]\n");
    }
}

void util_reset_ap()
{
    ConPrintf("\n[ Reset AP ]\n");


}
/*------------------------------------------------------------------------- */
/* ConCmdTFTP() */
/* Function to perform TFTP */
/*------------------------------------------------------------------------- */
void util_load_file( int ntok, char *tok1, char *tok2 )
{
    IPN    IPAddr;

    /* Check for 'stat ip' */
    if( ntok == 0 )
    {
        ConPrintf("\n[Upload Command]\n");
        ConPrintf("\nNeed a TFTP server IP/ hostname.\n\n");
        ConPrintf("uploadFirmware tftpserverIP myfile  - Retrieve 'myfile' from IP address\n");
        ConPrintf("uploadFirmware tftpserverHostname myfile - Resolve 'hostname' and retrieve 'myfile'\n\n");
    }
    else if( ntok == 2 )
    {
       if( !ConStrToIPN( tok1, &IPAddr ) )
           ConPrintf("Invalid address\n\n");
       else
           __load_file( IPAddr, tok2 );
    }
    else
        ConPrintf("\nIllegal argument. Type 'uploadFirmware' for help\n");
}

