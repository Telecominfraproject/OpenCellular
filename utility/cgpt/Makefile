# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TOP ?= ../../
CC ?= cc
INCLUDES += -I$(TOP)/common/include
CFLAGS += -Wall -Werror -ggdb
LIBS += $(TOP)/common/libcommon.a \
        $(TOP)/cgptlib/libcgpt.a

all: cgpt

cgpt: cgpt.o cgpt_options.o cgpt_show.o cgpt_attribute.o $(LIBS)
	$(CC) -o cgpt $(CFLAGS) $^

.c.o: $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f cgpt *.o *~
