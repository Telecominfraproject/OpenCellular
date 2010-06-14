# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ALL_OBJS = $(ALL_SRCS:%.c=${BUILD_ROOT}/%.o)
ALL_DEPS = $(ALL_OBJS:%.o=%.o.d)

${BUILD_ROOT}/%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MF $@.d -c -o $@ $<

-include ${ALL_DEPS}
