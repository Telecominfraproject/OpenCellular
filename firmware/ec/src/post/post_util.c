/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/common/post_util.h"

#include "inc/common/post.h"
#include "inc/common/system_states.h"
#include "platform/oc-sdr/schema/schema.h"
#include "src/registry/SSRegistry.h"

extern const Component sys_schema[OC_SS_MAX_LIMIT];
extern OCSubsystem *ss_reg[SUBSYSTEM_COUNT];
POSTData PostResult[POST_RECORDS] = { { 0 } };

#ifdef UT_POST
/*
 * TODO:  Duplicating the definition of the following three functions from post.c for the UT framework 
 * If we include post.c in the UT framework , we are exposing a lot of OS dependent APIs like create_task ,
 * util_queue etc to the Windows Cygwin environment which will create linking issues. 
 * This will get fixed as part of #419 
 */

void post_update_POSTStatus(POSTData *pData, ePostCode status)
{
    pData->status = status;
}

void post_init_POSTData(POSTData *pData, OCMPSubsystem subsystem,
                        uint8_t devSno)
{
    pData->subsystem = subsystem;
    pData->devSno = devSno;
    pData->i2cBus = 0xFF;
    pData->devAddr = 0xFF;
    pData->manId = 0xFFFF;
    pData->devId = 0xFFFF;
    pData->status = POST_DEV_MISSING;
}

void post_update_POSTData(POSTData *pData, uint8_t I2CBus, uint8_t devAddress,
                          uint16_t manId, uint16_t devId)
{
    pData->i2cBus = I2CBus;
    pData->devAddr = devAddress;
    pData->manId = manId;
    pData->devId = devId;
}
#else
static uint8_t deviceCount = 0;

/* Execute POST for a given device driver (performs deep copy of alert_data) */
static ePostCode _postDriver(const Component *subsystem, const Component *dev,
                             const AlertData *alert_data, POSTData *postData,
                             OCSubsystem *ss)
{
#if 0
    if (!dev->driver) {
        return POST_DEV_NO_DRIVER_EXIST;
    }
#endif
    ePostCode postcode = POST_DEV_FOUND;
    if (dev->driver->fxnTable->cb_probe) {
        postcode = dev->driver->fxnTable->cb_probe(dev->driver_cfg, postData);
        post_update_POSTStatus(postData, postcode);
    }
    LOGGER_DEBUG("%s:INFO:: %s (%s) %s\n", subsystem->name, dev->name,
                 dev->driver->name,
                 (postcode == POST_DEV_FOUND) ? "found" : "not found");

    if (postcode == POST_DEV_FOUND) {
        if (ss->state == SS_STATE_INIT) {
            if (dev->driver->fxnTable->cb_init) {
                AlertData *alert_data_cp = malloc(sizeof(AlertData));
                *alert_data_cp = *alert_data;
                postcode = dev->driver->fxnTable->cb_init(
                        dev->driver_cfg, dev->factory_config, alert_data_cp);
            } else {
                postcode = POST_DEV_NO_CFG_REQ;
            }
            post_update_POSTStatus(postData, postcode);
            LOGGER_DEBUG("%s:INFO:: Configuration for %s (%s) is %s\n",
                         subsystem->name, dev->name, dev->driver->name,
                         (postcode == POST_DEV_CFG_DONE) ?
                                 "ok" :
                                 (postcode == POST_DEV_NO_CFG_REQ) ?
                                 "not required." :
                                 "failed.");
        }
    }
}

ReturnStatus _execPost(OCMPMessageFrame *pMsg, unsigned int subsystem_id)
{
    const Component *subsystem = &sys_schema[subsystem_id];
    OCSubsystem *ss = ss_reg[subsystem_id];
    /* TODO: this is messy and assumes we have a pointer to the subsystem -
     * we'll want to change this once the framework is more mature */
    if (ss->state == SS_STATE_PWRON) {
        ss->state = SS_STATE_INIT;
    }

    /* Iterate over each component & device within the subsystem, calling
     * its post callback */
    ReturnStatus status = RETURN_OK;
    if ((subsystem->ssHookSet) && (ss->state == SS_STATE_INIT)) {
        if (subsystem->ssHookSet->preInitFxn) {
            if (!(subsystem->ssHookSet->preInitFxn(subsystem->driver_cfg,
                                                   &(ss->state)))) {
                status = RETURN_NOTOK;
                return status;
            }
        }
    }
    POSTData postData;
    ePostCode postcode = POST_DEV_MISSING;
    uint8_t devSno = 0;
    const Component *comp = &subsystem->components[0];
    for (uint8_t comp_id = 0; (comp && comp->name);
         ++comp, ++comp_id) { /* Component level (ec, ap, ch1, etc.) */
        /* If we have a driver at the component level, init */
        AlertData alert_data = {
            .subsystem = (OCMPSubsystem)subsystem_id,
            .componentId = comp_id,
            .deviceId = 0,
        };
        if (!comp->components) {
            if (comp->postDisabled == POST_DISABLED) {
                continue;
            }
            devSno++;
            post_init_POSTData(&postData, subsystem_id, devSno);
            //TODO: If postcode is configuration failure what should beth recovery action.
            if (_postDriver(subsystem, comp, &alert_data, &postData, ss) ==
                POST_DEV_NO_DRIVER_EXIST) {
                devSno--;
            } else {
                post_update_POSTresult(&postData);
            }
        } else {
            const Component *dev = &comp->components[0];
            for (uint8_t dev_id = 0; (dev && dev->name);
                 ++dev, ++dev_id) { /* Device level (ts, ina, etc) */
                AlertData alert_data = {
                    .subsystem = (OCMPSubsystem)subsystem_id,
                    .componentId = comp_id,
                    .deviceId = dev_id,
                };
                if (dev->postDisabled == POST_DISABLED) {
                    continue;
                }
                devSno++;
                post_init_POSTData(&postData, subsystem_id, devSno);
                if (_postDriver(subsystem, dev, &alert_data, &postData, ss) ==
                    POST_DEV_NO_DRIVER_EXIST) {
                    devSno--;
                } else {
                    post_update_POSTresult(&postData);
                }
            }
        }
    }
    if ((subsystem->ssHookSet) && (ss->state == SS_STATE_INIT)) {
        if (subsystem->ssHookSet->postInitFxn) {
            if (!(subsystem->ssHookSet->postInitFxn(subsystem->driver_cfg,
                                                    &(ss->state)))) {
                ss->state = SS_STATE_FAULTY;
            }
        }
    }
    if (ss->state == SS_STATE_INIT && status == RETURN_OK) {
        ss->state = SS_STATE_CFG;
    } else {
        ss->state = SS_STATE_FAULTY;
    }

    LOGGER("%s:INFO:: Modules and sensors are %s.\n", subsystem->name,
           ((status == RETURN_OK) ? "initialized." : "not initialized."));
    return status;
}

/* *****************************************************************************
 **    FUNCTION NAME   : post_update_POSTresult
 **
 **    DESCRIPTION     : save post result to flash
 **
 **    ARGUMENTS       : a0, a1 - not used
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
void post_update_POSTresult(POSTData *postData)
{
    /* Write a device info to flash but use a dummy function for REV B boards.*/
    uint8_t iter = 0;
    /* Dump structure at particular location*/
    if ((postData->subsystem == OC_SS_SYS) && (postData->devSno == 1)) {
        deviceCount = 0;
        memset(PostResult, '\0', (POST_RECORDS * sizeof(POSTData)));
    } else {
        deviceCount++;
    }
    //LOGGER_DEBUG("POST:INFO:: Updating POST results for the Subsystem %d , Device Serial offset %d , Total Number of records %d.\n",
    //             postData->subsystem,postData->devSno,deviceCount+1);
    memcpy(&PostResult[deviceCount], postData, sizeof(POSTData));
}
#endif
