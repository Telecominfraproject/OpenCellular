
#
# Main makefile for OCWare.
#

######################
# Set proper path.
######################

OCWARE_DIR=/home/kashif/OpenCellular/OCWare/firmware
TIRTOS_DIR=/home/kashif/OpenCellular/OCWare/firmware/ti/tirtos_tivac_2_16_01_14/
XDCTOOLS_DIR=/home/kashif/OpenCellular/OCWare/firmware/ti/xdctools_3_32_01_22_core/
TOOLCHAIN = /usr

#######################
# Do not change anything below this.
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
	$(TIRTOS_DRIVER_DIR)/packages

TARGET = gnu.targets.arm.M4F
PLATFORM = ti.platforms.tiva:TM4C1294NCPDT:true
PART=TM4C1294NCPDT
ROV_XS_SUFFIX = pm4fg
CONFIGURO_OPTS = -v
CONFIG=bld
empty:=
space:= $(empty) $(empty)
XDCPATH = $(subst $(space),;,$(XDCPATH_LIST))

CONFIGURO = $(XDCTOOLS_DIR)/xs --xdcpath="$(XDCPATH)" \
		xdc.tools.configuro $(CONFIGURO_OPTS)

# Find all C source/object files.
SRC_FILE = $(shell find . -name '*.c')
MAIN_OBJS = $(patsubst %.c,%.o,$(wildcard *.c */*.c))

CC = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
CFLAGS = -Wall -mcpu=cortex-m4 -mthumb -mabi=aapcs -mapcs-frame @$(CONFIG)/compiler.opt -O3 -g
CFLAGS += -DPART_$(MCU) -c -I$(TIVAWARE_PATH) -I$(OCWARE_DIR)/src
CFLAGS += -I$(XDC_PATH) -I$(BIOS_PATH) -I$(TIDRIVER_PATH) -I./
CFLAGS += -I$(OCWARE_DIR)/src/inc
CFLAGS += -I./ -I$(NDK_PATH)
CFLAGS += -I$(TIVAWARE_DIR)

LD = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
LFLAGS = -nostartfiles -static -Wl,--gc-sections -Wl,-T,../OC_CONNECT1.cmd -Wl,-T,$(CONFIG)/linker.cmd

.PRECIOUS: %/compiler.opt %/linker.cmd

all: main.out

%/compiler.opt %/linker.cmd : %.cfg
	$(CONFIGURO) -c $(TOOLCHAIN) -t $(TARGET) -p $(PLATFORM) -r debug $<
	cp $*/package/cfg/$*_$(ROV_XS_SUFFIX).rov.xs .

%.o: $(SRC_FILE)

%.o: $(CONFIG)/compiler.opt %.c
	$(CC) $(CFLAGS) -c $<

%.out: $(CONFIG)/linker.cmd $(MAIN_OBJS)
	$(LD) $(LFLAGS) -o $@ $(MAIN_OBJS)

clean:
	-rm -rf *.o *.out *.d *.rov.xs $(CONFIG) $(MAIN_OBJS)

test:
	cd test && $(MAKE)

.PHONY: all clean test

# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS}, clean)
-include ${wildcard *.d} __dummy__
endif
