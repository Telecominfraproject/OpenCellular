#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#

C_SRC = gps/gps.c \
	http-client/http.c \
	https-client/https.c \
	json-parser/json.c \
	json-parser/json_utils.c \
	state-machine/state-machine.c \
	state-machine/event_queue.c \
	string-buffer/string_buffer.c \
	timers/timer.c \
	utils/utils.c \
	logger/logger_common.c \
	logger/logger.c \
	logger/cloud_logger.c \
	lists/lists.c

include $(PRJROOT)/Makefile.common

LIB = $(PRJROOT)/libs/lib4gclient.a

include $(PRJROOT)/rules.mk
