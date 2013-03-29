/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Implementations of stateful memory operations.
 */

#include "sysincludes.h"

#include "stateful_util.h"
#include "utility.h"

void StatefulInit(MemcpyState *state, void *buf, uint64_t len)
{
	state->remaining_buf = buf;
	state->remaining_len = len;
	state->overrun = 0;
}

void *StatefulSkip(MemcpyState *state, uint64_t len)
{
	if (state->overrun)
		return NULL;
	if (len > state->remaining_len) {
		state->overrun = 1;
		return NULL;
	}
	state->remaining_buf += len;
	state->remaining_len -= len;
	return state; /* Must return something non-NULL. */
}

void *StatefulMemcpy(MemcpyState *state, void *dst, uint64_t len)
{
	if (state->overrun)
		return NULL;
	if (len > state->remaining_len) {
		state->overrun = 1;
		return NULL;
	}
	Memcpy(dst, state->remaining_buf, len);
	state->remaining_buf += len;
	state->remaining_len -= len;
	return dst;
}

const void *StatefulMemcpy_r(MemcpyState *state, const void *src, uint64_t len)
{
	if (state->overrun)
		return NULL;
	if (len > state->remaining_len) {
		state->overrun = 1;
		return NULL;
	}
	Memcpy(state->remaining_buf, src, len);
	state->remaining_buf += len;
	state->remaining_len -= len;
	return src;
}

const void *StatefulMemset_r(MemcpyState *state, const uint8_t val,
			     uint64_t len)
{
	if (state->overrun)
		return NULL;
	if (len > state->remaining_len) {
		state->overrun = 1;
		return NULL;
	}
	Memset(state->remaining_buf, val, len);
	state->remaining_buf += len;
	state->remaining_len -= len;
	return state; /* Must return something non-NULL. */
}
