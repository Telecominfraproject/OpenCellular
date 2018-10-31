/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <syslog.h>
#include <libgen.h>
//TEST
#define __filename__    (basename(__FILE__))
#ifdef CONSOLE_LOG
#define logit(facility, fmt, ...)                           \
{                                                           \
    if(facility != LOG_DEBUG)                               \
        printf(fmt "\n", ##__VA_ARGS__);                    \
    else                                                    \
        printf ("[%s:%d, %s()]:" fmt  "\n", __filename__,   \
    __LINE__, __func__, ##__VA_ARGS__);                     \
}

#else /* syslog */

#define logit(facility, fmt, ...)                           \
{                                                           \
    if(facility != LOG_DEBUG)                               \
        syslog(facility, fmt "\n", ##__VA_ARGS__);          \
    else                                                    \
        syslog(facility, "<d> [%s:%d, %s()]: " fmt "\n",    \
     __filename__, __LINE__, __func__, ##__VA_ARGS__);      \
}

#endif

#define logemerg(fmt, ...)  logit(LOG_EMERG,   "<E> " fmt, ##__VA_ARGS__)
#define logalert(fmt, ...)  logit(LOG_ALERT,   "<A> " fmt, ##__VA_ARGS__)
#define logcrit(fmt, ...)   logit(LOG_CRIT,    "<C> " fmt, ##__VA_ARGS__)
#define logerr(fmt, ...)    logit(LOG_ERR,     "<e> " fmt, ##__VA_ARGS__)
#define logwarn(fmt, ...)   logit(LOG_WARNING, "<w> " fmt, ##__VA_ARGS__)
#define lognotice(fmt, ...) logit(LOG_NOTICE,  "<n> " fmt, ##__VA_ARGS__)
#define loginfo(fmt, ...)   logit(LOG_INFO,    "<i> " fmt, ##__VA_ARGS__)
#define logdebug(fmt, ...)  logit(LOG_DEBUG,          fmt, ##__VA_ARGS__)

/*
 * @param ident an input value (by pointer)
 *
 */
void initlog(const char* ident);
/*
 * deinitialize the logging routine
 *
 */
void deinitlog(void);

#endif /* __LOGGER_H__ */
