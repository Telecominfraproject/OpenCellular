/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*
* This is wrapper file for at45db device contains wrapper functions like probe
* and function table of it. probe function calls device layer functions to
* complete post execution
*/

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_at45db.h"
#include "inc/devices/at45db.h"

/*****************************************************************************
 **    FUNCTION NAME   : _probe
 **
 **    DESCRIPTION     : Wrapper function for post execution
 **
 **    ARGUMENTS       : spi device configuration, post data pointer
 **
 **    RETURN TYPE     : ePostCode type status, can be found in post_frame.h
 **
 *****************************************************************************/
static ePostCode _probe(void *driver, POSTData *postData)
{
    return at45db_probe(driver,postData);
}

const Driver_fxnTable AT45DB641E_fxnTable = {
    /* Message handlers */
        .cb_probe = _probe,
};
