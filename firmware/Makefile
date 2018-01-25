#
# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
# 
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#

#
# Main makefile for OCWare.
#

######################
# Set proper path.
######################

OCWARE_DIR=/Users/kashi/work/OpenCellular/review/firmware
TIRTOS_DIR=/Users/kashi/work/tools/tirtos/tirtos_tivac_2_16_01_14
XDCTOOLS_DIR=/Users/kashi/work/tools/tirtos/xdctools_3_32_00_06_core
TOOLCHAIN=/Users/kashi/work/tools/gcc-arm-none-eabi-4_7-2013q3

#######################
# Do not change anything below this, unless you know what you really want!
#######################

BIOS_DIR=$(TIRTOS_DIR)/products/bios_6_45_02_31/
TIVAWARE_DIR=$(TIRTOS_DIR)/products/TivaWare_C_Series-2.1.1.71b/
UIA_DIR=$(TIRTOS_DIR)/products/uia_2_00_05_50/
TIRTOS_DRIVER_DIR=$(TIRTOS_DIR)/products/tidrivers_tivac_2_16_01_13
NDK_PATH = $(TIRTOS_DIR)/products/ndk_2_25_00_09/packages/

XDCPATH_LIST = \
	$(BIOS_DIR)/packages \
	$(UIA_DIR)/packages \
	$(TIRTOS_DIR)/packages  \
	$(TIRTOS_DRIVER_DIR)/packages  \
	$(NDK_PATH)  \

TARGET = gnu.targets.arm.M4F
PLATFORM = ti.platforms.tiva:TM4C1294NCPDT:true
MCU=TM4C1294NCPDT
ROV_XS_SUFFIX = pm4fg
CONFIGURO_OPTS = -v
CONFIG=OpenCellular
empty:=
space:= $(empty) $(empty)
XDCPATH = $(subst $(space),;,$(XDCPATH_LIST))

CONFIGURO = $(XDCTOOLS_DIR)/xs --xdcpath="$(XDCPATH)" \
		xdc.tools.configuro $(CONFIGURO_OPTS)

# Find all C source/object files.
SRC_FILE = $(shell find . -name '*.c' -not -path "./OpenCellular/*" -not -path "*test/*")
MAIN_OBJS  = $(patsubst %.c, %.o, $(SRC_FILE))

CC = $(TOOLCHAIN)/bin/arm-none-eabi-gcc  -std=c99
CFLAGS = -mcpu=cortex-m4 -mthumb -mabi=aapcs -mapcs-frame 
CFLAGS += @$(CONFIG)/compiler.opt -O3 -g -Wconversion -Wno-implicit 
CFLAGS += -Wfatal-errors  -w -DPART_$(MCU) -c 
CFLAGS += -I$(TIVAWARE_PATH) -I$(OCWARE_DIR)/src -DPART_TM4C1294NCPDT -Dgcc 
CFLAGS += -DTIVAWARE -DDEBUG_LOGS -D_POSIX_SOURCE
CFLAGS += -I$(XDC_PATH) -I$(BIOS_PATH) -I$(TIDRIVER_PATH) -I./
CFLAGS += -I$(OCWARE_DIR)/inc -I$(OCWARE_DIR)/src/ -I$(OCWARE_DIR) 
CFLAGS += -DFW_REV_C -DSDR_CURRENT_SENSOR
CFLAGS += -I./ -I$(NDK_PATH) -I$(NDK_PATH)/ti/ndk/inc/bsd/ -I$(BIOS_DIR)/packages/ti/sysbios/posix 
CFLAGS += -I$(TIVAWARE_DIR)
CFLAGS += -ffunction-sections -fdata-sections -MD -pedantic

LD = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
LFLAGS = -Wl,-Map,$(CONFIG).map -Wl,-T,$(CONFIG)/linker.cmd -L$(TIVAWARE_DIR)/driverlib/gcc -L$(TIVAWARE_DIR)/grlib/gcc -L$(TIVAWARE_DIR)/usblib/gcc -lgr -lusb -ldriver -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -nostartfiles -static -Wl,--gc-sections -L$(BIOS_DIR)/gnu/targets/arm/libs/install-native/arm-none-eabi/lib/armv7e-m/fpu -lgcc -lc -lm -lrdimon

.PRECIOUS: %/compiler.opt %/linker.cmd

all: main.out

%/compiler.opt %/linker.cmd : %.cfg
	$(CONFIGURO) -c $(TOOLCHAIN) -t $(TARGET) -p $(PLATFORM) -r debug $<
	cp $*/package/cfg/$*_$(ROV_XS_SUFFIX).rov.xs .

%.o: $(SRC_FILE)

%.o: $(CONFIG)/compiler.opt %.c
	$(CC) $(CFLAGS) -c $<

%.out: $(CONFIG)/linker.cmd $(MAIN_OBJS)
	$(LD) $(MAIN_OBJS) $(LFLAGS) -o $@

clean:
	-rm -rf *.o *.out *.d *.map *.rov.xs $(CONFIG) $(MAIN_OBJS); \
	find . -name "*.d" -type f -delete; \
	find . -name "*.o" -type f -delete; 

test:
	cd test && $(MAKE)

.PHONY: all clean test

# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS}, clean)
-include ${wildcard *.d} __dummy__
endif
