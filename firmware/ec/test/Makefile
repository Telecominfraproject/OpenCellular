# ==========================================
#   Unity Project - A Test Framework for C
#   Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
#   [Released under MIT License. Please refer to license.txt for details]
# ==========================================

UNITY_ROOT ?= ../../../../Unity
OCWARE_ROOT ?= ..

PATHT = suites/
PATHB = build/
PATHR = build/results/
RUNNER_PATH = build/runners/

# Compiler Options
# =============================================================================
C_COMPILER=gcc
ifeq ($(shell uname -s), Darwin)
C_COMPILER=clang
endif

CFLAGS  = -std=c99
CFLAGS += -Werror
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wno-missing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition
CFLAGS += -DTEST -DUT_POST -DUT_FRAMEWORK
CFLAGS += -g
CFLAGS += -Dxdc_target_types__="xdc_target_posix.h"
# Short enums are assumed throughout the code, need the -fshort-enums flag
CFLAGS += -fshort-enums

INC_DIRS=-I. -Isrc -I$(UNITY_ROOT)/src -I$(OCWARE_ROOT) -I$(OCWARE_ROOT)/src -Ixdc_stubs -I$(DIR_SYSBIOS) -I$(DIR_TIDRIVERS) -I$(DIR_XDC)

BUILD_PATHS = $(RUNNER_PATH) $(PATHB) $(PATHR)

# Discover all the tests, if test list not already provided
SRCT = $(wildcard $(PATHT)*.c)
TESTS ?= $(patsubst $(PATHT)Test%.c,Test%,$(SRCT))
RESULTS = $(patsubst %,$(PATHR)%.testresults,$(TESTS))

# List of standard files that every test will build (unity, test, test runner)
STD_FILES=$(PATHB)Test_%$(TARGET_EXTENSION):$(UNITY_ROOT)/src/unity.c $(PATHT)/Test_%.c  build/runners/Test_%_Runner.c

#We try to detect the OS we are running on, and adjust commands as needed
ifeq ($(OS),Windows_NT)
  DIR_XDC ?= C:/ti/xdctools_3_32_00_06_core/packages/
  DIR_SYSBIOS ?= C:/ti/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/
  DIR_TIDRIVERS ?= C:/ti/tirtos_tivac_2_16_01_14/products/tidrivers_tivac_2_16_01_13/packages/

  TARGET_EXTENSION=.exe

  ifeq ($(shell uname -s),) # not in a bash-like shell
    CLEANUP = del /F /Q
    MKDIR = mkdir
  else # in a bash-like shell, like msys
    CLEANUP = rm -f
    MKDIR = mkdir -p
  endif
else
  DIR_XDC ?= $(HOME)/ti/xdctools_3_32_00_06_core/packages/
  DIR_SYSBIOS ?= $(HOME)/ti/tirtos_tivac_2_16_01_14/products/bios_6_45_02_31/packages/
  DIR_TIDRIVERS ?= $(HOME)/ti/tirtos_tivac_2_16_01_14/products/tidrivers_tivac_2_16_01_13/packages/

  CLEANUP = rm -f
  MKDIR = mkdir -p
  TARGET_EXTENSION=.out
endif

# Make Targets
# =============================================================================
test: $(BUILD_PATHS) $(RESULTS)
	ruby $(UNITY_ROOT)/auto/unity_test_summary.rb $(PATHR)

# Note: we force the rebuild of this target each run to ensure fresh results
$(PATHR)%.testresults: $(PATHB)%$(TARGET_EXTENSION) FORCE
	-./$< > $@ 2>&1

$(RUNNER_PATH)Test_%_Runner.c: $(PATHT)Test_%.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb --suite_setup="extern void suite_setUp(void); suite_setUp();" --suite_teardown="extern void suite_tearDown(void); suite_tearDown(); return num_failures;" $< $@
	
# Test-specific build rules (add entries here for each new test suite)
# =============================================================================
TEST_PCA9557_SRC=$(OCWARE_ROOT)/src/devices/pca9557.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c
$(PATHB)Test_pca9557$(TARGET_EXTENSION): $(STD_FILES) $(TEST_PCA9557_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@

TEST_SE98A_SRC=$(OCWARE_ROOT)/src/devices/se98a.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_se98a$(TARGET_EXTENSION): $(STD_FILES) $(TEST_SE98A_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@

TEST_GpioPCA9557_SRC=$(OCWARE_ROOT)/src/devices/pca9557.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c $(OCWARE_ROOT)/src/drivers/GpioPCA9557.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c
$(PATHB)Test_GpioPCA9557$(TARGET_EXTENSION): $(STD_FILES) $(TEST_GpioPCA9557_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@

TEST_INA226_SRC=$(OCWARE_ROOT)/src/devices/ina226.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ina226$(TARGET_EXTENSION): $(STD_FILES) $(TEST_INA226_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@

TEST_SX1509_SRC=$(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c
$(PATHB)Test_sx1509$(TARGET_EXTENSION): $(STD_FILES) $(TEST_SX1509_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@

TEST_GpioSX1509_SRC=$(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/drivers/GpioSX1509.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c
$(PATHB)Test_GpioSX1509$(TARGET_EXTENSION): $(STD_FILES) $(TEST_GpioSX1509_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@
	
TEST_LTC4015_SRC=$(OCWARE_ROOT)/src/devices/ltc4015.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ltc4015$(TARGET_EXTENSION): $(STD_FILES) $(TEST_LTC4015_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@
	
TEST_powerSource_SRC=$(OCWARE_ROOT)/src/devices/powerSource.c $(OCWARE_ROOT)/src/drivers/GpioSX1509.c $(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/helpers/memory.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c
$(PATHB)Test_powerSource$(TARGET_EXTENSION): $(STD_FILES) $(TEST_powerSource_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@

TEST_EEPROM_SRC=$(OCWARE_ROOT)/src/devices/eeprom.c $(OCWARE_ROOT)/src/drivers/GpioSX1509.c $(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/helpers/memory.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c
$(PATHB)Test_eeprom$(TARGET_EXTENSION): $(STD_FILES) $(TEST_EEPROM_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@

TEST_LTC4275_SRC=$(OCWARE_ROOT)/src/devices/ltc4275.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c
$(PATHB)Test_ltc4275$(TARGET_EXTENSION): $(STD_FILES) $(TEST_LTC4275_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@

TEST_OCMP_ADT7481_SRC=$(OCWARE_ROOT)/src/devices/ocmp_wrappers/ocmp_adt7481.c $(OCWARE_ROOT)/src/devices/adt7481.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ocmp_adt7481$(TARGET_EXTENSION): $(STD_FILES) $(TEST_OCMP_ADT7481_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@

TEST_OCMP_LTC4274_SRC=$(OCWARE_ROOT)/src/devices/ocmp_wrappers/ocmp_ltc4274.c $(OCWARE_ROOT)/src/devices/ltc4274.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ocmp_ltc4274$(TARGET_EXTENSION): $(STD_FILES) $(TEST_OCMP_LTC4274_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -lm -o $@

TEST_PINGROUP_SRC=$(OCWARE_ROOT)/src/drivers/PinGroup.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/drivers/GpioPCA9557.c $(OCWARE_ROOT)/src/devices/pca9557.c
$(PATHB)Test_PinGroup_driver$(TARGET_EXTENSION): $(STD_FILES) $(TEST_PINGROUP_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@    

TEST_OCGPIO_SRC=$(OCWARE_ROOT)/src/drivers/OcGpio.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c fake/fake_GPIO.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c
$(PATHB)Test_OcGpio$(TARGET_EXTENSION): $(STD_FILES) $(TEST_OCGPIO_SRC)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@    

# Dummy target to allow us to force rebuild of testresults every time
FORCE:

# Create build directories if needed
$(PATHB):
	$(MKDIR) $(PATHB)

$(PATHR):
	$(MKDIR) $(PATHR)
	
$(RUNNER_PATH):
	$(MKDIR) $(RUNNER_PATH)
	
clean:
	rm -rf $(PATHB)

.PHONY: clean
.PHONY: test
.PHONY: FORCE

ci: CFLAGS += -Werror
ci: clean test
