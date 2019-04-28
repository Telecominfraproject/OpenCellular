/*
 * oc_watchdog.h
 *
 *  Created on: Apr 2, 2019
 *      Author: vthakur
 */

#include "Board.h"
#include "inc/common/global_header.h"
#include "inc/utils/util.h"



#include <stdio.h>
#include <stdlib.h>

#ifndef _OC_WATCHDOG_H_
#define _OC_WATCHDOG_H_

#define WD_TASK_PRIORITY 3
#define WD_TASK_STACK_SIZE 8096

void wd_createtask(void);

void wd_kick(Task_Handle task);


#endif /* _OC_WATCHDOG_H_ */
