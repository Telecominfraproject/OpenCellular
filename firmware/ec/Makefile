
#
# Main makefile for OCWare.
#

######################
# Set proper path.
######################

OCWARE_DIR ?= $(shell pwd)
TIRTOS_DIR ?= $(HOME)/ti/tirtos_tivac_2_16_01_14
XDCTOOLS_DIR ?= $(HOME)/ti/xdctools_3_32_00_06_core
TOOLCHAIN ?= /usr

#######################
# Do not change anything below this.
#######################

OUT=bld

BIOS_DIR=$(TIRTOS_DIR)/products/bios_6_45_02_31
TIVAWARE_DIR=$(TIRTOS_DIR)/products/TivaWare_C_Series-2.1.1.71b
UIA_DIR=$(TIRTOS_DIR)/products/uia_2_00_05_50
TIRTOS_DRIVER_DIR=$(TIRTOS_DIR)/products/tidrivers_tivac_2_16_01_13
NDK_PATH = $(TIRTOS_DIR)/products/ndk_2_25_00_09/packages
NDK_INC = $(TIRTOS_DIR)/products/ndk_2_25_00_09/packages/ti/ndk/inc/bsd

BIOS_PATH=$(BIOS_DIR)/packages/ti/sysbios/posix
UIA_PATH_PATH=$(UIA_DIR)/packages
TIDRIVER_PATH=$(TIRTOS_DRIVER_DIR)/packages
XDC_PATH=$(XDCTOOLS_DIR)/packages

XDCPATH_LIST = \
	$(TIRTOS_DIR)/packages  \
	$(TIRTOS_DRIVER_DIR)/packages \
	$(BIOS_DIR)/packages \
	$(NDK_PATH) \
	$(UIA_DIR)/packages

TARGET = gnu.targets.arm.M4F
PLATFORM = ti.platforms.tiva:TM4C1294NCPDT
PART=TM4C1294NCPDT
ROV_XS_SUFFIX = pm4fg
CONFIG=OpenCellular
CONFIGURO_OPTS = -v -o $(OUT)/$(CONFIG)
empty:=
space:= $(empty) $(empty)
XDCPATH = $(subst $(space),;,$(XDCPATH_LIST))

CONFIGURO = $(XDCTOOLS_DIR)/xs --xdcpath="$(XDCPATH)" \
		xdc.tools.configuro $(CONFIGURO_OPTS)

# Find all C source/object files.
SRC_FILE = $(shell find . -name '*.c' ! -path './test/*' ! -path './$(OUT)*')
MAIN_OBJS = $(SRC_FILE:.c=.o)
COVERAGE_OBJS = $(SRC_FILE:.c=.gcno)

CC = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
CFLAGS = -Wall -mcpu=cortex-m4 -mthumb -mabi=aapcs -mapcs-frame @$(OUT)/$(CONFIG)/compiler.opt -O3
CFLAGS += -std=c99 -ffunction-sections -fdata-sections
CFLAGS += -DTIVAWARE -DPART_$(PART) -DTARGET_IS_TM4C129_RA1 -Dgcc
CFLAGS += -I$(OCWARE_DIR)/src/ -I$(OCWARE_DIR)/common/ -I$(OCWARE_DIR)/
CFLAGS += -isystem$(XDC_PATH) -isystem$(BIOS_PATH) -isystem$(TIDRIVER_PATH) -isystem$(NDK_INC) -isystem$(TIVAWARE_DIR)

LD = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
LFLAGS = -nostartfiles -static -Wl,--gc-sections -march=armv7e-m -Wl,-T,OC_CONNECT1.lds
LFLAGS += --specs=nosys.specs -mfloat-abi=hard -mfpu=fpv4-sp-d16
LFLAGS += -Wl,--print-memory-usage #Optional
LFLAGS += -Xlinker -Map=$(OUT)/main.map #Optional
LLIBS = -L$(TIVAWARE_DIR)/driverlib/gcc/
LLIBS += -L$(TIVAWARE_DIR)/grlib/gcc/
LLIBS += -L$(TIVAWARE_DIR)/usblib/gcc/
LLIBS += -Wl,-T,$(OUT)/$(CONFIG)/linker.cmd
LLIBS += -Wl,--start-group
LLIBS += -ldriver -lgr -lusb -lm -lnosys -lc
LLIBS += -Wl,--end-group

.PRECIOUS: $(OUT)%/compiler.opt $(OUT)%/linker.cmd

OBJCOPY = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy
ALL_FILE = $(shell find . -name '*.c' -o -name '*.h')
LINT = clang-format
LINT_FLAGS = -i -style=file -fallback-style=none

.PRECIOUS: %/compiler.opt %/linker.cmd

default: get_cfg all

all: oc_connect1 hash

get_cfg:
	cd ../utilities/build; ./envDetect.sh $(CC) $(TIRTOS_DIR)

hash:
	cd ../utilities/checksums; ./hashes.sh $(shell pwd)/$(OUT) -ec

gen_schema:
	cd ../utilities/schema; ./mk_schema.sh

oc_connect1: gen_schema $(OUT)/OpenCellular.bin

$(OUT)/%/compiler.opt $(OUT)/%/linker.cmd : %.cfg
	$(CONFIGURO) -c $(TOOLCHAIN) -t $(TARGET) -p $(PLATFORM) -r release $<
	cp $(OUT)/$(CONFIG)/package/cfg/$*_$(ROV_XS_SUFFIX).rov.xs .

$(MAIN_OBJS): %.o: %.c $(OUT)/$(CONFIG)/compiler.opt
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/OpenCellular.out: $(OUT)/$(CONFIG)/linker.cmd $(MAIN_OBJS)
	$(LD) $(LFLAGS) -o $@ $(MAIN_OBJS) $(LLIBS)

$(OUT)/OpenCellular.bin: $(OUT)/OpenCellular.out
	$(OBJCOPY) -S -O binary $< $@

lint:
	$(LINT) $(LINT_FLAGS) $(ALL_FILE)

clean:
	-rm -rf *.o *.out *.d *.rov.xs $(OUT) $(MAIN_OBJS) $(COVERAGE_OBJS)

test:
	cd test && $(MAKE) $(TESTFLAGS)

ci: TESTFLAGS = ci
ci: CFLAGS += -Wextra -Wpointer-arith -Wcast-align -Wwrite-strings -Wswitch-default -Wunreachable-code -Winit-self -Wno-missing-field-initializers -Wno-unknown-pragmas -Wstrict-prototypes -Wundef
ci: CFLAGS += -ftest-coverage
ci: all test

.PHONY: all oc_connect1 clean test

# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS}, clean)
-include ${wildcard *.d} __dummy__
endif
