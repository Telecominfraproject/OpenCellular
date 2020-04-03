#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#

include ${PRJROOT}/Makefile.common

C_SRC += $(wildcard $(TARGET_PLATFORM)/*.c)

APP = paws
C_LIBS += -l4gclient -lm 


## if TARGET_PLATFORM = fap or native or unittest
## ifeq ($(TARGET_PLATFORM),$(filter $(TARGET_PLATFORM),fap native))     
	C_LIBS += -lsqlite3	-lssl -lcrypto
	INC_DIRS += -I./paws_db -I./lte_db -I./sctp_agent_db
	DB_SRC = $(wildcard paws_db/*.c)
	DB_SRC += $(wildcard lte_db/*.c)
	DB_SRC += $(wildcard sctp_agent_db/*.c)
	C_SRC += $(DB_SRC)
	DEPS = $(patsubst %.o,%.d,$(C_OBJ))
##endif

include ${PRJROOT}/rules.mk

.PHONY: db
db:
	make -C sql_db db

