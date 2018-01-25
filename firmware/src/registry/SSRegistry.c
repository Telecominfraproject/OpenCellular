/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "SSRegistry.h"
#include "Framework.h"

#include "helpers/array.h"
#include "inc/common/bigbrother.h" /* For sending msg back via BB */
#include "inc/common/post.h" /* For sending POST response */
#include "inc/common/global_header.h"
#include "inc/utils/ocmp_util.h"
#include "inc/utils/util.h"

/* TODO: configurable directory (allow us to target different platforms) */
#include "platform/oc-sdr/schema.h"

#include <ti/sysbios/BIOS.h>

extern const Component sys_schema[SUBSYSTEM_COUNT];
static OCSubsystem *ss_reg[SUBSYSTEM_COUNT] = {};

static const size_t PARAM_SIZE_MAP[] = {
    [TYPE_NULL] = 0,
    [TYPE_INT8] = sizeof(int8_t),
    [TYPE_UINT8] = sizeof(uint8_t),
    [TYPE_INT16] = sizeof(int16_t),
    [TYPE_UINT16] = sizeof(uint16_t),
    [TYPE_INT32] = sizeof(int32_t),
    [TYPE_UINT32] = sizeof(uint32_t),
    [TYPE_INT64] = sizeof(int64_t),
    [TYPE_UINT64] = sizeof(uint64_t),
    [TYPE_STR] = 1, /* TODO: properly handle strings */
    [TYPE_BOOL] = sizeof(bool),
    [TYPE_ENUM] = 1, /* TODO: this really depends on enum - param_size should
                        iterate over definition to determine size requirement*/
};

static unsigned int _subcompCount(const Component *comp) {
    unsigned int i = 0;
    if (comp) {
        while (comp->components[i].name) {
            ++i;
        }
    }
    return i;
}

static size_t _paramSize(const Parameter *param) {
    if (!param || (param->type >= ARRAY_SIZE(PARAM_SIZE_MAP))) {
        return 0;
    }

    if (param->type == TYPE_STR) {
        return param->size;
    }
    return PARAM_SIZE_MAP[param->type];
}

static bool _compIsValid(const Component *comp)
{
    return comp && comp->name;
}

static bool _paramIsValid(const Parameter *param)
{
    return param && param->name;
}

void OCMP_GenerateAlert(const AlertData *alert_data,
                        unsigned int alert_id,
                        const void *data)
{
    if (!alert_data) {
        return;
    }

    const Component *subsystem = &sys_schema[alert_data->subsystem];
    const Component *component = &subsystem->components[alert_data->componentId];
    const Driver *driver = component->components[alert_data->deviceId].driver;
    const Parameter *param = &driver->alerts[alert_id];

    /* Count all previous parameters before this component */
    unsigned int param_sum = 0;
    for (int i = 0; i < alert_data->deviceId; ++i) {
        const Parameter *param = &component->components[i].driver->alerts[0];
        for (; _paramIsValid(param); ++param) {
            param_sum += 1;
        }
    }

    uint16_t parameters = 0x01 << (param_sum + alert_id);

    /* Align to 4 byte boundary (bug in host) */
    size_t param_size = (_paramSize(param) + 3) & ~0x03;

    OCMPMessageFrame *pMsg = create_ocmp_msg_frame(alert_data->subsystem,
                                                   OCMP_MSG_TYPE_ALERT,
                                                   OCMP_AXN_TYPE_ACTIVE,
                                                   alert_data->componentId + 1, /* TODO: inconsistency indexing in host */
                                                   parameters,
                                                   param_size);
    if (pMsg) {
        memcpy(pMsg->message.ocmp_data, data, _paramSize(param));
        Util_enqueueMsg(bigBrotherTxMsgQueue, semBigBrotherMsg,
                        (uint8_t*) pMsg);
    } else {
        LOGGER_ERROR("ERROR::Unable to allocate alert packet\n");
    }
}

/* Execute POST for a given device driver (performs deep copy of alert_data) */
static ReturnStatus _postDriver(const Component *subsystem,
                                const Component *dev,
                                const AlertData *alert_data)
{
    if (!dev->driver) {
        return RETURN_OK;
    }

    ePostCode postcode = POST_DEV_FOUND;
    if (dev->driver->cb_probe) {
        dev->driver->cb_probe(dev->driver_cfg);
    }
    LOGGER_DEBUG("%s:INFO:: %s (%s) %s\n", subsystem->name,
           dev->name,  dev->driver->name,
           (postcode == POST_DEV_FOUND) ? "found" : "not found");

    if (subsystem->ss->state == SS_STATE_INIT) {
        if (postcode == POST_DEV_FOUND) {
            if (dev->driver->cb_init) {
                AlertData *alert_data_cp = malloc(sizeof(AlertData));
                *alert_data_cp = *alert_data;
                postcode = dev->driver->cb_init(dev->driver_cfg,
                                                dev->factory_config,
                                                alert_data_cp);
            } else {
                postcode = POST_DEV_CFG_DONE;
            }
            LOGGER_DEBUG("%s:INFO:: Configuration status for %s (%s) is %s\n",
                         subsystem->name,
                         dev->name,
                         dev->driver->name,
                         (postcode == POST_DEV_CFG_DONE) ? "OK" : "NOT OK");
        }
        return ((postcode == POST_DEV_CFG_DONE) ?
                RETURN_OK : RETURN_NOTOK);
    } else {
        return ((postcode == POST_DEV_FOUND) ?
                RETURN_OK : RETURN_NOTOK);
    }
}

static ReturnStatus _execPost(OCMPMessageFrame *pMsg,
                              unsigned int subsystem_id)
{
    const Component *subsystem = &sys_schema[subsystem_id];

    /* TODO: this is messy and assumes we have a pointer to the subsystem -
     * we'll want to change this once the framework is more mature */
    if (subsystem->ss->state == SS_STATE_PWRON) {
        subsystem->ss->state = SS_STATE_INIT;
    }

    /* Iterate over each component & device within the subsystem, calling
     * its post callback */
    ReturnStatus status = RETURN_OK;
    if(subsystem->ssHookSet) {
        if(subsystem->ssHookSet->preInitFxn) {
            if (!(subsystem->ssHookSet->preInitFxn(&(subsystem->ss->state)))) {
                status = RETURN_NOTOK;
                return status;
            }
        }
    }
    const Component *comp = &subsystem->components[0];
    for (uint8_t comp_id = 0; _compIsValid(comp); ++comp, ++comp_id) { /* Component level (ec, ap, ch1, etc.) */
        /* If we have a driver at the component level, init */
        AlertData alert_data = {
            .subsystem = (OCMPSubsystem)subsystem_id,
            .componentId = comp_id,
            .deviceId = 0,
        };
        _postDriver(subsystem, comp, &alert_data);

        const Component *dev = &comp->components[0];
        for (uint8_t dev_id = 0; _compIsValid(dev); ++dev, ++dev_id) { /* Device level (ts, ina, etc) */
            AlertData alert_data = {
                .subsystem = (OCMPSubsystem)subsystem_id,
                .componentId = comp_id,
                .deviceId = dev_id,
            };
            _postDriver(subsystem, dev, &alert_data);
        }
    }
    if(subsystem->ssHookSet) {
        if(subsystem->ssHookSet->postInitFxn){
            if (!(subsystem->ssHookSet->postInitFxn(&(subsystem->ss->state)))) {
                subsystem->ss->state = SS_STATE_FAULTY;
            }
        }
    }
    if (subsystem->ss->state == SS_STATE_INIT && status == RETURN_OK) {
        subsystem->ss->state = SS_STATE_CFG;
    } else {
        subsystem->ss->state = SS_STATE_FAULTY;
    }

    LOGGER("%s:INFO:: Modules and sensors are %s.\n", subsystem->name,
           ((status == RETURN_OK) ? "initialized." : "not initialized."));
    return status;
}

static bool _handleMsgTypeCmd(OCMPMessageFrame *pMsg, const Component *comp)
{
    if (pMsg->message.subsystem != OC_SS_DEBUG) {
        const Command *cmd = comp->commands[pMsg->message.action];
        if (cmd && cmd->cb_cmd) {
            cmd->cb_cmd(comp->driver_cfg, pMsg->message.ocmp_data);
            return true;
        }
    } else {
        const Component *subComp = &comp->components[pMsg->message.parameters];
        const Command *cmd = subComp->driver->commands[pMsg->message.action];
        if (cmd && cmd->cb_cmd) {
            cmd->cb_cmd(subComp->driver_cfg, pMsg->message.ocmp_data);
            return true;
        }
    }
    return false;
}

static bool _handle_cmd_get(OCMPMessageFrame *pMsg, const Component *comp,
                            unsigned int param_id, void *buf_ptr)
{
    switch (pMsg->message.msgtype) {
        case OCMP_MSG_TYPE_CONFIG:
            return (comp->driver->cb_get_config &&
                    comp->driver->cb_get_config(comp->driver_cfg, param_id,
                                                buf_ptr));
        case OCMP_MSG_TYPE_STATUS:
            return (comp->driver->cb_get_status &&
                    comp->driver->cb_get_status(comp->driver_cfg, param_id,
                                                buf_ptr));
        default:
            return false;
    }
}

static bool _handle_cmd_set(OCMPMessageFrame *pMsg, const Component *comp,
                            unsigned int param_id, const void *data)
{
    switch (pMsg->message.msgtype) {
        case OCMP_MSG_TYPE_CONFIG:
            return (comp->driver->cb_set_config &&
                    comp->driver->cb_set_config(comp->driver_cfg, param_id,
                                                data));
        default:
            return false;
    }
}

static bool _handleDevStatCfg(OCMPMessageFrame *pMsg, const Component *dev,
                              unsigned int *param_id, uint8_t **buf_ptr)
{
    if (!dev->driver) {
        return false;
    }

    const Parameter *param_list = NULL;
    switch (pMsg->message.msgtype) {
        case OCMP_MSG_TYPE_CONFIG:
            param_list = dev->driver->config;
            break;
        case OCMP_MSG_TYPE_STATUS:
            param_list = dev->driver->status;
            break;
        default:
            return false;
    }

    if (!param_list) {
        return false;
    }

    bool dev_handled = false;
    unsigned int normalized_id = 0;
    while (param_list[normalized_id].name) {
        if (pMsg->message.parameters & (1 << *param_id)) {
            switch (pMsg->message.action) {
                case OCMP_AXN_TYPE_GET:
                    if (_handle_cmd_get(pMsg, dev, normalized_id,
                                        *buf_ptr)) {
                        dev_handled = true;
                    } else {
                        pMsg->message.parameters &= ~(1 << *param_id);
                    }
                    break;
                case OCMP_AXN_TYPE_SET:
                    if (_handle_cmd_set(pMsg, dev, normalized_id,
                                        *buf_ptr)) {
                        dev_handled = true;
                    } else {
                        pMsg->message.parameters &= ~(1 << *param_id);
                    }
                    break;
                default:
                    pMsg->message.parameters &= ~(1 << *param_id);
                    break;
            }
        }
        if (!dev->driver->payload_fmt_union) {
            *buf_ptr += _paramSize(&param_list[normalized_id]);
        }
        (*param_id)++;
        normalized_id++;
    }
    return dev_handled;
}

static bool _handleMsgTypeStatCfg(OCMPMessageFrame *pMsg, const Component *comp)
{
    /* Determine driver & parameter */
    unsigned int param_id = 0;
    uint8_t *buf_ptr = pMsg->message.ocmp_data;
    bool dev_handled = false;

    /* Handle component-level driver */
    if (_handleDevStatCfg(pMsg, comp, &param_id, &buf_ptr)) {
        dev_handled = true;
    }

    /* Handle sub-components (devices) */
    const Component *dev = &comp->components[0];
    for (; _compIsValid(dev); ++dev) {
        if (_handleDevStatCfg(pMsg, dev, &param_id, &buf_ptr)) {
            dev_handled = true;
        }
    }
    return dev_handled;
}

static bool ocmp_route(OCMPMessageFrame *pMsg, unsigned int subsystem_id)
{
    const Component *subsystem = &sys_schema[subsystem_id];

    /* The main exception to the flow right now is POST - check for it first */
    if (pMsg->message.msgtype == OCMP_MSG_TYPE_POST) {
        ReturnStatus status = _execPost(pMsg, subsystem_id);

        memcpy((pMsg->message.ocmp_data), &status, 1);
        pMsg->message.action = OCMP_AXN_TYPE_REPLY;
        Util_enqueueMsg(postRxMsgQueue, semPOSTMsg, (uint8_t*) pMsg);

        return true;
    }

    /* Validate component ID */
    if (pMsg->message.componentID >= _subcompCount(subsystem)) {
        LOGGER_ERROR("Component %d out of bounds\n", pMsg->message.componentID);
        return false;
    }

    const Component *comp = &subsystem->components[pMsg->message.componentID];

    /* TODO: clean up special handling for commands */
    bool dev_handled = false;
    switch (pMsg->message.msgtype) {
        case OCMP_MSG_TYPE_COMMAND:
            dev_handled = _handleMsgTypeCmd(pMsg, comp);
            break;
        case OCMP_MSG_TYPE_CONFIG:
        case OCMP_MSG_TYPE_STATUS:
            dev_handled = _handleMsgTypeStatCfg(pMsg, comp);
            break;
        default:
            break;
    }

    /* If we couldn't handle this message, return error */
    if (!dev_handled) {
        pMsg->message.parameters = 0x00;
    }

    /* Send reply to the middleware */
    pMsg->message.action = OCMP_AXN_TYPE_REPLY;
    Util_enqueueMsg(bigBrotherTxMsgQueue, semBigBrotherMsg, (uint8_t *)pMsg);
    return true;
}

static void _subsystem_event_loop(UArg a0, UArg a1)
{
    OCSubsystem *ss = (OCSubsystem *)a0;
    if (!ss) {
        return;
    }

    //const Component *component = &sys_schema[a1];

    while (1) {
        if (Semaphore_pend(ss->sem, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(ss->msgQueue)) {
                OCMPMessageFrame *pMsg =
                        (OCMPMessageFrame *) Util_dequeueMsg(ss->msgQueue);

                if (pMsg) {
                    /* Attempt to route the message to the correct driver
                       (if successful, no need to clean up message here) */
                    if (!ocmp_route(pMsg, a1)) {
                        LOGGER_ERROR("ERROR:: Unable to route OCMP message\n");
                        free(pMsg);
                    }
                }
            }
        }
    }
}

static void subsystem_init(OCMPSubsystem ss_id) {
    OCSubsystem *ss = sys_schema[ss_id].ss;
    if (!ss) {
        return;
    }

    ss_reg[ss_id] = ss;

    ss->state = SS_STATE_PWRON;

    /* Create Semaphore for RX Message Queue */
    Semaphore_construct(&ss->semStruct, 0, NULL);
    ss->sem = Semaphore_handle(&ss->semStruct);
    if (!ss->sem) {
        LOGGER_DEBUG("SS REG:ERROR:: Failed in Creating RX Semaphore for "
                     "subsystem %d\n", ss_id);
    }

    /* Create Message Queue for RX Messages */
    ss->msgQueue = Util_constructQueue(&ss->queueStruct);
    if (!ss->msgQueue) {
        LOGGER_ERROR("SS REG:ERROR:: Failed in Constructing Message Queue for "
                     "RX Message for subsystem %d\n", ss_id);
    }

    /* Spin up the task */
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = ss->taskStack;
    taskParams.stackSize = ss->taskStackSize;
    taskParams.priority = ss->taskPriority;
    taskParams.arg0 = (UArg)ss;
    taskParams.arg1 = ss_id;

    Task_construct(&ss->taskStruct, _subsystem_event_loop, &taskParams, NULL);
    LOGGER_DEBUG("SS REG:DEBUG:: Creating Task for Subsystem %d\n", ss_id);
}

void SSRegistry_init(void) {
    for (OCMPSubsystem i = (OCMPSubsystem)0; i < SUBSYSTEM_COUNT; ++i) {
        subsystem_init(i);
    }
}

OCSubsystem* SSRegistry_Get(OCMPSubsystem ss_id) {
    if (ss_id >= SUBSYSTEM_COUNT) {
        return NULL;
    }
    return ss_reg[ss_id];
}

bool SSRegistry_sendMessage(OCMPSubsystem ss_id, void *pMsg) {
    OCSubsystem *ss = SSRegistry_Get(ss_id);
    if (!ss) {
        return false;
    }

    return Util_enqueueMsg(ss->msgQueue, ss->sem, (uint8_t*) pMsg);
}
