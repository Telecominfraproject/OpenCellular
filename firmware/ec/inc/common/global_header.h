/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef GLOBAL_HEADER_H_
#define GLOBAL_HEADER_H_

#define _FW_REV_MAJOR_ 0
#define _FW_REV_MINOR_ 4
#define _FW_REV_BUGFIX_ 0
#define _FW_REV_TAG_ __COMMIT_HASH__

/* xdc/runtime/System.h is poorly written so this must be included first */
#include <stdbool.h>

/* XDCtools Header files */
#include <xdc/runtime/System.h> /* For System_printf */

#if 1
#define DEBUG(...)                  \
    {                               \
        System_printf(__VA_ARGS__); \
        System_flush();             \
    }

#define LOGGER(...)                 \
    {                               \
        System_printf(__VA_ARGS__); \
        System_flush();             \
    }
#define LOGGER_WARNING(...)         \
    {                               \
        System_printf(__VA_ARGS__); \
        System_flush();             \
    }
#define LOGGER_ERROR(...)           \
    {                               \
        System_printf(__VA_ARGS__); \
        System_flush();             \
    }
#ifdef DEBUG_LOGS
#define LOGGER_DEBUG(...)           \
    {                               \
        System_printf(__VA_ARGS__); \
        System_flush();             \
    }

#define NOP_DELAY()               \
    {                             \
        uint32_t delay = 7000000; \
        while (delay--)           \
            ;                     \
    }
#else
#define LOGGER_DEBUG(...)
#define NOP_DELAY()
#endif
#else
#define DEBUG(...) //

#define LOGGER(...) //
#define LOGGER_WARNING(...) //
#define LOGGER_ERROR(...) //
#ifdef DEBUG_LOGS
#define LOGGER_DEBUG(...) //
#endif
#define NOP_DELAY()               \
    {                             \
        uint32_t delay = 7000000; \
        while (delay--)           \
            ;                     \
    }
#endif
#define RET_OK 0
#define RET_NOT_OK 1

typedef enum {
    RETURN_OK = 0x00,
    RETURN_NOTOK = 0x01,
    RETURN_OCMP_INVALID_SS_TYPE = 0x02,
    RETURN_OCMP_INVALID_MSG_TYPE = 0x03,
    RETURN_OCMP_INVALID_COMP_TYPE = 0x04,
    RETURN_OCMP_INVALID_AXN_TYPE = 0x05,
    RETURN_OCMP_INVALID_PARAM_INFO = 0x06,
    RETURN_OCMP_INVALID_CMD_INFO = 0x07,
    RETURN_OCMP_INVALID_IFACE_TYPE = 0x08,
    RETURN_DEV_VALUE_TOO_LOW = 0x09,
    RETURN_DEV_VALUE_TOO_HIGH = 0x0A,
    RETURN_DEV_I2C_BUS_FAILURE = 0x0B,
    RETURN_SS_NOT_READY = 0x0C,
    RETURN_SS_NOT_RESET_STATE = 0x0D
} ReturnStatus;

#endif /* GLOBAL_HEADER_H_ */
