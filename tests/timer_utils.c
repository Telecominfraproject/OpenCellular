/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "timer_utils.h"

void StartTimer(ClockTimerState* ct) {
  clock_gettime(CLOCK_REALTIME, &ct->start_time);
}

void StopTimer(ClockTimerState* ct) {
  clock_gettime(CLOCK_REALTIME, &ct->end_time);
}

uint32_t GetDurationMsecs(ClockTimerState* ct) {
  uint64_t start = ((uint64_t) ct->start_time.tv_sec * 1000000000 +
                    (uint64_t) ct->start_time.tv_nsec);
  uint64_t end = ((uint64_t) ct->end_time.tv_sec * 1000000000 +
                  (uint64_t) ct->end_time.tv_nsec);
  uint64_t duration_msecs = (end - start) / 1000000U;  /* Nanoseconds ->
                                                        * Milliseconds. */
  return (uint32_t) duration_msecs;
}
