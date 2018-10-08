/*******************************************************************************
  Filename:       util.c
  Revised:        $Date: 2015-06-02 11:18:40 -0700 (Tue, 02 Jun 2015) $
  Revision:       $Revision: 43957 $

  Description:    This file contains utility functions.

  Copyright 2014 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "inc/utils/util.h"

#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

#include <stdbool.h>
#include <stdlib.h>

/*********************************************************************
 * TYPEDEFS
 */

// RTOS queue for profile/app messages.
typedef struct _queueRec_ {
    Queue_Elem _elem; // queue element
    uint8_t *pData; // pointer to app data
} queueRec_t;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*******************************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Util_constructClock
 *
 * @brief   Initialize a TIRTOS Clock instance.
 *
 * @param   pClock        - pointer to clock instance structure.
 * @param   clockCB       - callback function upon clock expiration.
 * @param   clockDuration - longevity of clock timer in milliseconds
 * @param   clockPeriod   - if set to a value other than 0, the first
 *                          expiry is determined by clockDuration.  All
 *                          subsequent expiries use the clockPeriod value.
 * @param   startFlag     - TRUE to start immediately, FALSE to wait.
 * @param   arg           - argument passed to callback function.
 *
 * @return  Clock_Handle  - a handle to the clock instance.
 */
Clock_Handle Util_constructClock(Clock_Struct *pClock, Clock_FuncPtr clockCB,
                                 uint32_t clockDuration, uint32_t clockPeriod,
                                 uint8_t startFlag, UArg arg)
{
    Clock_Params clockParams;

    // Convert clockDuration in milliseconds to ticks.
    uint32_t clockTicks = clockDuration * (1000 / Clock_tickPeriod);

    // Setup parameters.
    Clock_Params_init(&clockParams);

    // Setup argument.
    clockParams.arg = arg;

    // If period is 0, this is a one-shot timer.
    clockParams.period = clockPeriod * (1000 / Clock_tickPeriod);

    // Starts immediately after construction if true, otherwise wait for a call
    // to start.
    clockParams.startFlag = startFlag;

    // Initialize clock instance.
    Clock_construct(pClock, clockCB, clockTicks, &clockParams);

    return Clock_handle(pClock);
}

/*********************************************************************
 * @fn      Util_startClock
 *
 * @brief   Start a clock.
 *
 * @param   pClock - pointer to clock struct
 *
 * @return  none
 */
void Util_startClock(Clock_Struct *pClock)
{
    Clock_Handle handle = Clock_handle(pClock);

    // Start clock instance
    Clock_start(handle);
}

/*********************************************************************
 * @fn      Util_restartClock
 *
 * @brief   Restart a clock by changing the timeout.
 *
 * @param   pClock - pointer to clock struct
 * @param   clockTimeout - longevity of clock timer in milliseconds
 *
 * @return  none
 */
void Util_restartClock(Clock_Struct *pClock, uint32_t clockTimeout)
{
    uint32_t clockTicks;
    Clock_Handle handle;

    handle = Clock_handle(pClock);

    if (Clock_isActive(handle)) {
        // Stop clock first
        Clock_stop(handle);
    }

    // Convert timeout in milliseconds to ticks.
    clockTicks = clockTimeout * (1000 / Clock_tickPeriod);

    // Set the initial timeout
    Clock_setTimeout(handle, clockTicks);

    // Start clock instance
    Clock_start(handle);
}

/*********************************************************************
 * @fn      Util_isActive
 *
 * @brief   Determine if a clock is currently active.
 *
 * @param   pClock - pointer to clock struct
 *
 * @return  TRUE or FALSE
 */
bool Util_isActive(Clock_Struct *pClock)
{
    Clock_Handle handle = Clock_handle(pClock);

    // Start clock instance
    return Clock_isActive(handle);
}

/*********************************************************************
 * @fn      Util_stopClock
 *
 * @brief   Stop a clock.
 *
 * @param   pClock - pointer to clock struct
 *
 * @return  none
 */
void Util_stopClock(Clock_Struct *pClock)
{
    Clock_Handle handle = Clock_handle(pClock);

    // Stop clock instance
    Clock_stop(handle);
}

/*********************************************************************
 * @fn      Util_rescheduleClock
 *
 * @brief   Reschedule a clock by changing the timeout and period values.
 *
 * @param   pClock - pointer to clock struct
 * @param   clockPeriod - longevity of clock timer in milliseconds
 * @return  none
 */
void Util_rescheduleClock(Clock_Struct *pClock, uint32_t clockPeriod)
{
    bool running;
    uint32_t clockTicks;
    Clock_Handle handle;

    handle = Clock_handle(pClock);
    running = Clock_isActive(handle);

    if (running) {
        Clock_stop(handle);
    }

    // Convert period in milliseconds to ticks.
    clockTicks = clockPeriod * (1000 / Clock_tickPeriod);

    Clock_setTimeout(handle, clockTicks);
    Clock_setPeriod(handle, clockTicks);

    if (running) {
        Clock_start(handle);
    }
}

/*********************************************************************
 * @fn      Util_constructQueue
 *
 * @brief   Initialize an RTOS queue to hold messages to be processed.
 *
 * @param   pQueue - pointer to queue instance structure.
 *
 * @return  A queue handle.
 */
Queue_Handle Util_constructQueue(Queue_Struct *pQueue)
{
    // Construct a Queue instance.
    Queue_construct(pQueue, NULL);

    return Queue_handle(pQueue);
}

/*********************************************************************
 * @fn      Util_enqueueMsg
 *
 * @brief   Creates a queue node and puts the node in RTOS queue.
 *
 * @param   msgQueue - queue handle.
 * @param   sem - thread's event processing semaphore that queue is
 *                associated with.
 * @param   pMsg - pointer to message to be queued
 *
 * @return  TRUE if message was queued, FALSE otherwise.
 */
uint8_t Util_enqueueMsg(Queue_Handle msgQueue, Semaphore_Handle sem,
                        uint8_t *pMsg)
{
    queueRec_t *pRec;

    // Allocated space for queue node.

    if (pRec = (queueRec_t *)malloc(sizeof(queueRec_t))) {
        pRec->pData = pMsg;

        Queue_enqueue(msgQueue, &pRec->_elem);

        // Wake up the application thread event handler.
        if (sem) {
            Semaphore_post(sem);
        }

        return TRUE;
    }

    // Free the message.

    free(pMsg);

    return FALSE;
}

/*********************************************************************
 * @fn      Util_dequeueMsg
 *
 * @brief   Dequeues the message from the RTOS queue.
 *
 * @param   msgQueue - queue handle.
 *
 * @return  pointer to dequeued message, NULL otherwise.
 */
uint8_t *Util_dequeueMsg(Queue_Handle msgQueue)
{
    if (!Queue_empty(msgQueue)) {
        queueRec_t *pRec = Queue_dequeue(msgQueue);
        uint8_t *pData = pRec->pData;

        // Free the queue node
        // Note:  this does not free space allocated by data within the node.
        free(pRec);

        return pData;
    }

    return NULL;
}
