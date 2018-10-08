/*******************************************************************************
  Filename:       util.h
  Revised:        $Date: 2015-07-07 13:30:52 -0700 (Tue, 07 Jul 2015) $
  Revision:       $Revision: 44319 $

  Description:    This file contains function declarations common to CC26xx
                  TIRTOS Applications.

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

#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>

/*********************************************************************
*  EXTERNAL VARIABLES
*/

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    uint16_t event; // Event type.
    uint8_t state; // Event state;
} appEvtHdr_t;

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * API FUNCTIONS
 */

/*********************************************************************
 * @fn      Util_constructClock
 *
 * @brief   Initialize a TIRTOS Clock instance.
 *
 * @param   pClock        - pointer to clock instance structure.
 * @param   clockCB       - callback function upon clock expiration.
 * @param   clockDuration - longevity of clock timer in milliseconds
 * @param   clockPeriod   - duration of a periodic clock, used continuously
 *                          after clockDuration expires.
 * @param   startFlag     - TRUE to start immediately, FALSE to wait.
 * @param   arg           - argument passed to callback function.
 *
 * @return  Clock_Handle  - a handle to the clock instance.
 */
Clock_Handle Util_constructClock(Clock_Struct *pClock, Clock_FuncPtr clockCB,
                                 uint32_t clockDuration, uint32_t clockPeriod,
                                 uint8_t startFlag, UArg arg);

/*********************************************************************
 * @fn      Util_startClock
 *
 * @brief   Start a clock.
 *
 * @param   pClock - pointer to clock struct
 *
 * @return  none
 */
void Util_startClock(Clock_Struct *pClock);

/*********************************************************************
 * @fn      Util_setClockTimeout
 *
 * @brief   Restart a clock by changing the timeout.
 *
 * @param   pClock - pointer to clock struct
 * @param   clockTimeout - longevity of clock timer in milliseconds
 *
 * @return  none
 */
void Util_restartClock(Clock_Struct *pClock, uint32_t clockTimeout);

/*********************************************************************
 * @fn      Util_isActive
 *
 * @brief   Determine if a clock is currently active.
 *
 * @param   pClock - pointer to clock struct
 *
 * @return  TRUE or FALSE
 */
bool Util_isActive(Clock_Struct *pClock);

/*********************************************************************
 * @fn      Util_stopClock
 *
 * @brief   Stop a clock.
 *
 * @param   pClock - pointer to clock struct
 *
 * @return  none
 */
void Util_stopClock(Clock_Struct *pClock);

/*********************************************************************
 * @fn      Util_rescheduleClock
 *
 * @brief   Reschedule a clock by changing the timeout and period values.
 *
 * @param   pClock - pointer to clock struct
 * @param   clockPeriod - longevity of clock timer in milliseconds
 * @return  none
 */
void Util_rescheduleClock(Clock_Struct *pClock, uint32_t clockPeriod);

/*********************************************************************
 * @fn      Util_constructQueue
 *
 * @brief   Initialize an RTOS queue to hold messages from profile to be
 *          processed.
 *
 * @param   pQueue - pointer to queue instance structure.
 *
 * @return  A queue handle.
 */
Queue_Handle Util_constructQueue(Queue_Struct *pQueue);

/*********************************************************************
 * @fn      Util_enqueueMsg
 *
 * @brief   Creates a queue node and puts the node in RTOS queue.
 *
 * @param   msgQueue - queue handle.
 *
 * @param   sem - the thread's event processing semaphore that this queue is
 *                associated with.
 *
 * @param   pMsg - pointer to message to be queued
 *
 * @return  TRUE if message was queued, FALSE otherwise.
 */
uint8_t Util_enqueueMsg(Queue_Handle msgQueue, Semaphore_Handle sem,
                        uint8_t *pMsg);

/*********************************************************************
 * @fn      Util_dequeueMsg
 *
 * @brief   Dequeues the message from the RTOS queue.
 *
 * @param   msgQueue - queue handle.
 *
 * @return  pointer to dequeued message, NULL otherwise.
 */
uint8_t *Util_dequeueMsg(Queue_Handle msgQueue);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* UTIL_H */
