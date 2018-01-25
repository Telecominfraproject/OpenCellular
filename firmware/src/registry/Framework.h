/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef _SYS_CFG_FRAMEWORK_H
#define _SYS_CFG_FRAMEWORK_H

#include "SSRegistry.h" /* Temporary - for ss registry structs */
#include "inc/common/post_frame.h" /* Just for post code */

#include <stdbool.h>
#include <stdlib.h>

/* TODO: move these to common header file */
typedef enum DataType {
    TYPE_NULL = 0, /* No data is passed (used for simple GPIO-based alerts) */
    TYPE_INT8,
    TYPE_UINT8,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32,
    TYPE_UINT32,
    TYPE_INT64,
    TYPE_UINT64,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_ENUM,

    COUNT_TYPE,
} DataType;

typedef struct Enum_Map {
    int value;
    const char *name;
} Enum_Map;

typedef struct Parameter {
    const char *name;
    DataType type;
    union {
        Enum_Map *values;
        size_t size;
    };
} Parameter;

typedef bool (*CB_Command) (void *driver, void *params);

typedef struct Command {
    const char *name;
    const Parameter *parameters;
    const CB_Command cb_cmd;
} Command;

// To avoid the awkward situation of not knowing how much to allocate for the return value (think
// string returns), we instead rely on the 'get' and 'set' functions to allocate and return a
// pointer to the value it wants to return via OCMP
typedef bool (*StatusGet_Cb) (void *driver, unsigned int param_id,
                              void *return_buf);
typedef bool (*ConfigGet_Cb) (void *driver, unsigned int param_id,
                              void *return_buf);
typedef bool (*ConfigSet_Cb) (void *driver, unsigned int param_id,
                              const void *data);

typedef ePostCode (*CB_Probe) (void *driver);
typedef ePostCode (*CB_Init) (void *driver, const void *config,
                              const void *alert_token);

typedef bool (*ssHook_Cb) (void *return_buf);

typedef struct Driver {
    const char *name;
    const Parameter *status;
    const Parameter *config;
    const Parameter *alerts;
    const Parameter *argList;
    const Command *commands[OCMP_NUM_ACTIONS];

    // TODO: These callbacks are a bit rough. They'll get the job done, but we should revisit other
    // options (per-parameter callbacks for example)
    StatusGet_Cb cb_get_status;
    ConfigGet_Cb cb_get_config;
    ConfigSet_Cb cb_set_config;
    CB_Probe cb_probe;
    CB_Init cb_init;

    bool payload_fmt_union; /* TODO: hack to account for OBC/Testmodule payload
                               being packed as a union instead of a struct */
} Driver;

typedef struct SSHookSet {
 ssHook_Cb preInitFxn ;/* Function will run before post is executed */
 ssHook_Cb postInitFxn; /* Function will run after post is executed */
}SSHookSet;

typedef void (*Component_InitCb) (void);

typedef struct Component {
    const char *name;
    const struct Component *components;
    const Driver *driver;
    void *driver_cfg; // TODO: this could be turned into a standard polymorphism struct to hold the
                      // driver, hw config & driver object data (like we did for GPIO)
    const void *factory_config; /* Factory defaults for the device */
    const Command *commands[OCMP_NUM_ACTIONS]; /* TODO: super gross hack to fit into current CLI */
    const SSHookSet *ssHookSet;
    OCSubsystem *ss;
} Component;

/* TODO: consider moving struct into c file - only need pointer externally */
typedef struct AlertData {
    OCMPSubsystem subsystem;
    uint8_t componentId;
    uint8_t deviceId;
} AlertData;

void OCMP_GenerateAlert(const AlertData *alert_data,
                        unsigned int alert_id,
                        const void *data);

#endif /* _SYS_CFG_FRAMEWORK_H */
