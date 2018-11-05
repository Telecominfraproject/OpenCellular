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
PATHC = build/coverage/
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
COVERAGE = $(patsubst %,$(PATHC)%.info,$(TESTS))

# List of standard files that every test will build (unity, test, test runner)
STD_FILES=$(PATHB)Test_%$(TARGET_EXTENSION):$(UNITY_ROOT)/src/unity.c $(PATHT)/Test_%.c  build/runners/Test_%_Runner.c

# Create the flags to add all individual coverage files
LCOV_ADD = $(patsubst %,-a %, $(COVERAGE))
LCOV_ADD += -a $(PATHC)test-base.info
# The directories to ignore in coverage
LCOV_IGNORE = "*/test/*" "*/Unity/*" "*/ti/*"

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

junit: test
	@ruby $(UNITY_ROOT)/auto/stylize_as_junit.rb -r $(PATHR) -o $(PATHR)/unit-test-results.xml

# Note: we force the rebuild of this target each run to ensure fresh results
$(PATHR)%.testresults: $(PATHB)%$(TARGET_EXTENSION) FORCE
	-./$< > $@ 2>&1

$(RUNNER_PATH)Test_%_Runner.c: $(PATHT)Test_%.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb --suite_setup="extern void suite_setUp(void); suite_setUp();" --suite_teardown="extern void suite_tearDown(void); suite_tearDown(); return num_failures;" $< $@

$(PATHC)test-base.info:
	@lcov --rc lcov_branch_coverage=1 -c -i -d ../ -o $(PATHC)test-base.info

# Get coverage statistics for every test
$(COVERAGE): $(PATHC)%.info: $(PATHC)/% $(PATHB)%$(TARGET_EXTENSION) $(PATHC) $(BUILD_PATHS) $(RESULTS)
	@lcov --rc lcov_branch_coverage=1 -c -d $< -o $@ > /dev/null
# Combine all test suites coverage info and remove all files that are not src
coverage: $(COVERAGE) $(PATHC) $(PATHC)test-base.info
	@lcov --rc lcov_branch_coverage=1 $(LCOV_ADD) -o $(PATHC)/test-coverage-all.info > /dev/null
	@lcov --rc lcov_branch_coverage=1 --remove $(PATHC)/test-coverage-all.info  $(LCOV_IGNORE) -o $(PATHC)/test-coverage.info
	
# Test-specific build rules (add entries here for each new test suite)
# =============================================================================
INC_M =-lm
TEST_PCA9557_SRC=$(OCWARE_ROOT)/src/devices/pca9557.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c
$(PATHB)Test_pca9557$(TARGET_EXTENSION): $(STD_FILES) $(TEST_PCA9557_SRC)

TEST_SE98A_SRC=$(OCWARE_ROOT)/src/devices/se98a.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_se98a$(TARGET_EXTENSION): $(STD_FILES) $(TEST_SE98A_SRC) $(INC_M)

TEST_GpioPCA9557_SRC=$(OCWARE_ROOT)/src/devices/pca9557.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c $(OCWARE_ROOT)/src/drivers/GpioPCA9557.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c
$(PATHB)Test_GpioPCA9557$(TARGET_EXTENSION): $(STD_FILES) $(TEST_GpioPCA9557_SRC)

TEST_INA226_SRC=$(OCWARE_ROOT)/src/devices/ina226.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ina226$(TARGET_EXTENSION): $(STD_FILES) $(TEST_INA226_SRC)

TEST_SX1509_SRC=$(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c
$(PATHB)Test_sx1509$(TARGET_EXTENSION): $(STD_FILES) $(TEST_SX1509_SRC)

TEST_GpioSX1509_SRC=$(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/drivers/GpioSX1509.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c
$(PATHB)Test_GpioSX1509$(TARGET_EXTENSION): $(STD_FILES) $(TEST_GpioSX1509_SRC)

TEST_LTC4015_SRC=$(OCWARE_ROOT)/src/devices/ltc4015.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ltc4015$(TARGET_EXTENSION): $(STD_FILES) $(TEST_LTC4015_SRC) $(INC_M)

TEST_powerSource_SRC=$(OCWARE_ROOT)/src/devices/powerSource.c $(OCWARE_ROOT)/src/drivers/GpioSX1509.c $(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/helpers/memory.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c
$(PATHB)Test_powerSource$(TARGET_EXTENSION): $(STD_FILES) $(TEST_powerSource_SRC) $(INC_M)

TEST_EEPROM_SRC=$(OCWARE_ROOT)/src/devices/eeprom.c $(OCWARE_ROOT)/src/drivers/GpioSX1509.c $(OCWARE_ROOT)/src/devices/sx1509.c $(OCWARE_ROOT)/src/helpers/memory.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c
$(PATHB)Test_eeprom$(TARGET_EXTENSION): $(STD_FILES) $(TEST_EEPROM_SRC) $(INC_M)

TEST_LTC4275_SRC=$(OCWARE_ROOT)/src/devices/ltc4275.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c
$(PATHB)Test_ltc4275$(TARGET_EXTENSION): $(STD_FILES) $(TEST_LTC4275_SRC) $(INC_M)

TEST_OCMP_ADT7481_SRC=$(OCWARE_ROOT)/src/devices/ocmp_wrappers/ocmp_adt7481.c $(OCWARE_ROOT)/src/devices/adt7481.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ocmp_adt7481$(TARGET_EXTENSION): $(STD_FILES) $(TEST_OCMP_ADT7481_SRC) $(INC_M)

TEST_OCMP_LTC4274_SRC=$(OCWARE_ROOT)/src/devices/ocmp_wrappers/ocmp_ltc4274.c $(OCWARE_ROOT)/src/devices/ltc4274.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_GPIO.c fake/fake_I2C.c fake/fake_ThreadedISR.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/post/post_util.c
$(PATHB)Test_ocmp_ltc4274$(TARGET_EXTENSION): $(STD_FILES) $(TEST_OCMP_LTC4274_SRC) $(INC_M)

TEST_PINGROUP_SRC=$(OCWARE_ROOT)/src/drivers/PinGroup.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c $(OCWARE_ROOT)/src/drivers/GpioPCA9557.c $(OCWARE_ROOT)/src/devices/pca9557.c
$(PATHB)Test_PinGroup_driver$(TARGET_EXTENSION): $(STD_FILES) $(TEST_PINGROUP_SRC)

TEST_OCGPIO_SRC=$(OCWARE_ROOT)/src/drivers/OcGpio.c $(OCWARE_ROOT)/src/devices/i2cbus.c fake/fake_I2C.c fake/fake_GPIO.c fake/fake_ThreadedISR.c $(OCWARE_ROOT)/src/helpers/memory.c stub/stub_GateMutex.c
$(PATHB)Test_OcGpio$(TARGET_EXTENSION): $(STD_FILES) $(TEST_OCGPIO_SRC)

$(PATHB)%$(TARGET_EXTENSION):
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $^ -o $@    
	$(COV_CMDS)

# Dummy target to allow us to force rebuild of testresults every time
FORCE:

# Create build directories if needed
$(PATHB):
	$(MKDIR) $(PATHB)

$(PATHR):
	$(MKDIR) $(PATHR)
	
$(PATHC):
	$(MKDIR) $(PATHC)

$(RUNNER_PATH):
	$(MKDIR) $(RUNNER_PATH)
	
clean:
	rm -rf $(PATHB)

.PHONY: clean
.PHONY: test
.PHONY: junit
.PHONY: coverage
.PHONY: FORCE

# Continuous integration
ci: CFLAGS += -Werror
ci: CFLAGS += -ftest-coverage -fprofile-arcs -fprofile-dir=$(PATHC)$*/
ci: COV_CMDS = @mkdir -p $(PATHC)$* && mv *.gcno $(PATHC)$*
ci: clean junit coverage

# Test Coverage
cov: CFLAGS += -ftest-coverage -fprofile-arcs -fprofile-dir=$(PATHC)$*/
cov: COV_CMDS = @mkdir -p $(PATHC)$* && mv *.gcno $(PATHC)$*
cov: clean test coverage
