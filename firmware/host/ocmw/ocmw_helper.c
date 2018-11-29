/**

 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* OC includes */
#include <ocmw_core.h>
#include <ocmw_eth_comm.h>
#include <ocmw_uart_comm.h>
#include <ocwdg_daemon.h>
#include <logger.h>
#include <ocmw_occli_comm.h>

#define SIZE_INT sizeof(int32_t)

int8_t eepromStatusFlag = 0;

/******************************************************************************
 * Function Name    : ocmw_sem_wait_nointr
 * Description      : static inline function for avoiding interruption caused
 *                    to sem_wait() library call
 * Input(s)         : sem
 * Output(s)        :
 ******************************************************************************/
inline int32_t ocmw_sem_wait_nointr(sem_t *sem)
{
    while (sem_wait(sem))
        if (errno == EINTR)
            errno = 0;
        else
            return -1;
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_sem_timedwait_nointr
 * Description      : static inline function for avoiding interruption caused
 *                    to sem_timedwait() library call
 * Input(s)         : sem, timeout
 * Output(s)        :
 ******************************************************************************/
inline int32_t ocmw_sem_timedwait_nointr(sem_t *sem,
                                         const struct timespec *timeout)
{
    while (sem_timedwait(sem, timeout))
        if (errno == EINTR)
            errno = 0;
        else
            return -1;
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_dataparsing_from_db
 * Description      : This Function used to parase the data from db
 * Input(s)         : paramIndex, paramSizebuf
 * Output(s)        : dataSize, pos
 ******************************************************************************/
void ocmw_dataparsing_from_db(int32_t paramIndex, int32_t *paramSizebuf,
                              int32_t *dataSize, int32_t *pos)
{
    int32_t count;
    int32_t paramIdPresence;
    int32_t loc;

    count = 0;
    paramIdPresence = 0;
    loc = 0;

    if ((paramSizebuf == NULL) || (dataSize == NULL) || (pos == NULL)) {
        return;
    }

    *dataSize = 0;
    *pos = 0;

    for (count = 0; count < MAX_PARM_COUNT; count++) {
        paramIdPresence = (paramIndex) / ((int)pow(2, count));
        if (paramIdPresence == 1) {
            break;
        }

        else {
            loc = loc + paramSizebuf[count];
        }
    }

    *dataSize = paramSizebuf[count];
    *pos = loc;
}

/******************************************************************************
 * Function Name    : ocmw_dataparsing_from_ec
 * Description      : This Function used to parse the uart message from ec
 * Input(s)         : input, bufParamStruct
 * Output(s)        :
 ******************************************************************************/
void ocmw_dataparsing_from_ec(ocmwSendRecvBuf *input, bufParam *bufParamStruct)
{
    int32_t count = 0;
    int32_t paramIdPresence = 0;
    int32_t pos = 0;
    int32_t paramidSize = 0;
    int32_t index = 0;

    if ((input == NULL) || (bufParamStruct == NULL)) {
        printf("Memory address is invalid  : %s()\n", __func__);
        return;
    }

    int16_t paramInfo = input->paramInfo;
    int32_t numOfele = input->numOfele;
    int8_t *pbuf = input->pbuf;
    int32_t *paramSizebuf = input->paramSizebuf;

    for (count = 0; count < numOfele; count++) {
        bufParamStruct[count].paramindex =
            (paramInfo & ((int32_t)pow(2, count + index)));

        paramIdPresence =
            bufParamStruct[count].paramindex / pow(2, count + index);

        if (paramIdPresence == 0) {
            paramidSize = 0;
        } else {
            paramidSize = paramSizebuf[count];
        }

        switch (paramidSize) {
            case VOID:
                bufParamStruct[count].paramval = 0;
                break;
            case CHAR:
                bufParamStruct[count].paramval = *((int8_t *)&pbuf[pos]);
                break;

            case SHORT:
                bufParamStruct[count].paramval = *((int16_t *)&pbuf[pos]);
                break;

            case INT:
                bufParamStruct[count].paramval = *((int32_t *)&pbuf[pos]);
                break;

            default:
                return;
                break;
        }
    }
}
