/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_TIMER_UTILS_H_
#define VBOOT_REFERENCE_TIMER_UTILS_H_

#include <inttypes.h>

#include <time.h>

typedef struct ClockTimer {
  struct timespec start_time;
  struct timespec end_time;
} ClockTimerState;

/* Start timer and update [ct]. */
void StartTimer(ClockTimerState* ct);

/* Stop timer and update [ct]. */
void StopTimer(ClockTimerState* ct);

/* Get duration in milliseconds. */
uint32_t GetDurationMsecs(ClockTimerState* ct);

#endif  /* VBOOT_REFERENCE_TIMER_UTILS_H_ */
