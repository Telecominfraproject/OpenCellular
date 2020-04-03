##
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
##

include ${PRJROOT}/Makefile.common

APP = sctp-agent
C_LIBS += -l4gclient -lm

INC_DIRS += -I./$(TARGET_PLATFORM)

## tcps1
C_SRC += $(wildcard tcps1/*.c)
INC_DIRS += -I./tcps1

# ifeq ($(TARGET_PLATFORM),native)     
# CFLAGS += -DTCPS1_TEST
# endif

## if TARGET_PLATFORM = fap or native
ifeq ($(TARGET_PLATFORM),$(filter $(TARGET_PLATFORM),fap native))     
	C_LIBS += -lsqlite3	
	INC_DIRS += -I./lte_db -I./sctp_agent_db
	C_SRC += $(wildcard $(TARGET_PLATFORM)/*.c)
	DB_SRC = $(wildcard sctp_agent_db/*.c)
	DB_SRC += $(wildcard lte_db/*.c)
	C_SRC += $(DB_SRC)
	DEPS = $(patsubst %.o,%.d,$(C_OBJ))
endif

include ${PRJROOT}/rules.mk

.PHONY: db
db:
	make -C sql_db db
