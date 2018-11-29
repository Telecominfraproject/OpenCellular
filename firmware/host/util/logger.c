/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* OC includes */
#include <logger.h>

/**************************************************************************
 * Function Name    : initlog
 * Description      : This Function used to initialize the logging
 *                    functionality
 * Input(s)         : ident
 * Output(s)        :
 ***************************************************************************/
void initlog(const char *ident)
{
    openlog(ident, LOG_CONS | LOG_PID, LOG_USER);
}

/**************************************************************************
 * Function Name    : deinitlog
 * Description      : This Function used to uninitialize logging
 *                    functionality
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
void deinitlog(void)
{
    closelog();
}
