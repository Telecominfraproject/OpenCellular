# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This Makefile normally builds in a 'build' subdir, but use
#
#    make BUILD=<dir>
#
# to put the output somewhere else.

##############################################################################
# Make variables come in two flavors, immediate or deferred.
#
#   Variable definitions are parsed like this:
#
#        IMMEDIATE = DEFERRED
#    or
#        IMMEDIATE := IMMEDIATE
#
#   Rules are parsed this way:
#
#        IMMEDIATE : IMMEDIATE
#           DEFERRED
#
# So you can assign variables in any order if they're only to be used in
# actions, but if you use a variable in either the target or prerequisite of a
# rule, the rule will be constructed using only the top-down, immediate value.
#
# So we'll try to define all the variables first. Then the rules.
#

##############################################################################
# Configuration variables come first.
#
# Our convention is that we only use := for variables that will never be
# changed or appended. They must be defined before being used anywhere.

# we should only run pwd once, not every time we refer to ${BUILD}.
SRCDIR := $(shell pwd)
BUILD ?= $(SRCDIR)/build
export BUILD

# Target for 'make install'
DESTDIR ?= /usr/bin
INSTALL ?= install

# Where to install the (exportable) executables for testing?
TEST_INSTALL_DIR = ${BUILD}/install_for_test

# Verbose? Use V=1
ifeq (${V},)
Q := @
endif

# Architecture detection
_machname := $(shell uname -m)
HOST_ARCH ?= ${_machname}

# ARCH and/or FIRMWARE_ARCH are defined by the Chromium OS ebuild.
# Pick a sane target architecture if none is defined.
ifeq (${ARCH},)
  ARCH := ${HOST_ARCH}
else ifeq (${ARCH},i386)
  override ARCH := x86
else ifeq (${ARCH},amd64)
  override ARCH := x86_64
endif

# FIRMWARE_ARCH is only defined by the Chromium OS ebuild if compiling
# for a firmware target (such as u-boot or depthcharge). It must map
# to the same consistent set of architectures as the host.
ifeq (${FIRMWARE_ARCH},i386)
  override FIRMWARE_ARCH := x86
else ifeq (${FIRMWARE_ARCH},amd64)
  override FIRMWARE_ARCH := x86_64
endif

# Provide default CC and CFLAGS for firmware builds; if you have any -D flags,
# please add them after this point (e.g., -DVBOOT_DEBUG).
#
# TODO(crosbug.com/16808) We hard-code u-boot's compiler flags here just
# temporarily. As we are still investigating which flags are necessary for
# maintaining a compatible ABI, etc. between u-boot and vboot_reference.
#
# As a first step, this makes the setting of CC and CFLAGS here optional, to
# permit a calling script or Makefile to set these.
#
# Flag ordering: arch, then -f, then -m, then -W
DEBUG_FLAGS := $(if ${DEBUG},-g -O0,-Os)
COMMON_FLAGS := -nostdinc -pipe \
	-ffreestanding -fno-builtin -fno-stack-protector \
	-Werror -Wall -Wstrict-prototypes ${DEBUG_FLAGS}

# Note: FIRMWARE_ARCH is defined by the Chromium OS ebuild.
ifeq (${FIRMWARE_ARCH}, arm)
CC ?= armv7a-cros-linux-gnueabi-gcc
CFLAGS ?= -march=armv5 \
	-fno-common -ffixed-r8 \
	-mfloat-abi=hard -marm -mabi=aapcs-linux -mno-thumb-interwork \
	${COMMON_FLAGS}
else ifeq (${FIRMWARE_ARCH}, x86)
CC ?= i686-pc-linux-gnu-gcc
# Drop -march=i386 to permit use of SSE instructions
CFLAGS ?= \
	-ffunction-sections -fvisibility=hidden -fno-strict-aliasing \
	-fomit-frame-pointer -fno-toplevel-reorder -fno-dwarf2-cfi-asm \
	-mpreferred-stack-boundary=2 -mregparm=3 \
	${COMMON_FLAGS}
else ifeq (${FIRMWARE_ARCH}, x86_64)
CFLAGS ?= ${COMMON_FLAGS} \
	-fvisibility=hidden -fno-strict-aliasing -fomit-frame-pointer
else
# FIRMWARE_ARCH not defined; assuming local compile.
CC ?= gcc
CFLAGS += -DCHROMEOS_ENVIRONMENT -Wall -Werror # HEY: always want last two?
endif

ifneq (${DEBUG},)
CFLAGS += -DVBOOT_DEBUG
endif

ifeq (${DISABLE_NDEBUG},)
CFLAGS += -DNDEBUG
endif

# Create / use dependency files
CFLAGS += -MMD -MF $@.d

# Code coverage
ifneq (${COV},)
  COV_FLAGS = -O0 --coverage
  CFLAGS += ${COV_FLAGS}
  LDFLAGS += ${COV_FLAGS}
  COV_INFO = ${BUILD}/coverage.info
endif

# And a few more default utilities
LD = ${CC}
CXX ?= g++ # HEY: really?
PKG_CONFIG ?= pkg-config

# Determine QEMU architecture needed, if any
ifeq (${ARCH},${HOST_ARCH})
  # Same architecture; no need for QEMU
  QEMU_ARCH :=
else ifeq (${HOST_ARCH}-${ARCH},x86_64-x86)
  # 64-bit host can run 32-bit targets directly
  QEMU_ARCH :=
else
  QEMU_ARCH := ${ARCH}
endif

# The top of the chroot for qemu must be passed in via the SYSROOT environment
# variable.  In the Chromium OS chroot, this is done automatically by the
# ebuild.

ifeq (${QEMU_ARCH},)
  # Path to build output for running tests is same as for building
  BUILD_RUN = ${BUILD}
  SRC_RUN = ${SRCDIR}
else
  $(info Using qemu for testing.)
  # Path to build output for running tests is different in the chroot
  BUILD_RUN = $(subst ${SYSROOT},,${BUILD})
  SRC_RUN = $(subst ${SYSROOT},,${SRCDIR})

  QEMU_BIN = qemu-${QEMU_ARCH}
  QEMU_RUN = ${BUILD_RUN}/${QEMU_BIN}
  export QEMU_RUN

  RUNTEST = tests/test_using_qemu.sh
endif

export BUILD_RUN

# Some things only compile inside the Chromium OS chroot.
# TODO: Those things should be in their own repo, not part of vboot_reference
# TODO: Is there a better way to detect this?
ifneq (${CROS_WORKON_SRCROOT},)
IN_CHROOT := yes
endif

# TODO: Move to separate repo.
ifneq (${IN_CHROOT},)
PC_BASE_VER ?= 125070
PC_DEPS := libchrome-${PC_BASE_VER}
PC_CFLAGS := $(shell ${PKG_CONFIG} --cflags ${PC_DEPS})
PC_LDLIBS := $(shell ${PKG_CONFIG} --libs ${PC_DEPS})
endif


##############################################################################
# Now we need to describe everything we might want or need to build

# TODO: This should go in its own repo.
AU_CGPTLIB = ${BUILD}/cgpt/libcgpt-cc.a
# This is just ... Gah. There's no good place for it.
DUMPKERNELCONFIGLIB = ${BUILD}/libdump_kernel_config.a

# Everything wants these headers.
INCLUDES += \
	-Ifirmware/include \
	-Ifirmware/lib/include \
	-Ifirmware/lib/cgptlib/include \
	-Ifirmware/lib/cryptolib/include \
	-Ifirmware/lib/tpm_lite/include

# If we're not building for a specific target, just stub out things like the
# TPM commands and various external functions that are provided by the BIOS.
ifeq (${FIRMWARE_ARCH},)
INCLUDES += -Ifirmware/stub/include
else
INCLUDES += -Ifirmware/arch/${FIRMWARE_ARCH}/include
endif

# Firmware library. TODO: Do we still need to export this?
FWLIB = ${BUILD}/vboot_fw.a

# find lib -iname '*.c' | sort
FWLIB_SRCS = \
	firmware/lib/cgptlib/cgptlib.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/crc8.c \
	firmware/lib/cryptolib/padding.c \
	firmware/lib/cryptolib/rsa.c \
	firmware/lib/cryptolib/rsa_utility.c \
	firmware/lib/cryptolib/sha1.c \
	firmware/lib/cryptolib/sha256.c \
	firmware/lib/cryptolib/sha512.c \
	firmware/lib/cryptolib/sha_utility.c \
	firmware/lib/stateful_util.c \
	firmware/lib/utility.c \
	firmware/lib/utility_string.c \
	firmware/lib/vboot_api_init.c \
	firmware/lib/vboot_api_firmware.c \
	firmware/lib/vboot_api_kernel.c \
	firmware/lib/vboot_audio.c \
	firmware/lib/vboot_common.c \
	firmware/lib/vboot_display.c \
	firmware/lib/vboot_firmware.c \
	firmware/lib/vboot_kernel.c \
	firmware/lib/vboot_nvstorage.c

# Support real TPM unless BIOS sets MOCK_TPM
ifeq (${MOCK_TPM},)
FWLIB_SRCS += \
	firmware/lib/rollback_index.c \
	firmware/lib/tpm_bootmode.c \
	firmware/lib/tpm_lite/tlcl.c
else
FWLIB_SRCS += \
	firmware/lib/mocked_rollback_index.c \
	firmware/lib/mocked_tpm_bootmode.c \
	firmware/lib/tpm_lite/mocked_tlcl.c
endif

ifeq (${FIRMWARE_ARCH},)
# Include BIOS stubs in the firmware library when compiling for host
FWLIB_SRCS += \
	firmware/stub/tpm_lite_stub.c \
	firmware/stub/utility_stub.c \
	firmware/stub/vboot_api_stub.c \
	firmware/stub/vboot_api_stub_disk.c
endif

FWLIB_OBJS = ${FWLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${FWLIB_OBJS}


# Library to build the utilities. "HOST" mostly means "userspace".
HOSTLIB = ${BUILD}/vboot_host.a

HOSTLIB_SRCS = \
	host/arch/${ARCH}/lib/crossystem_arch.c \
	host/lib/crossystem.c \
	host/lib/file_keys.c \
	host/lib/fmap.c \
	host/lib/host_common.c \
	host/lib/host_key.c \
	host/lib/host_keyblock.c \
	host/lib/host_misc.c \
	host/lib/host_signature.c \
	host/lib/signature_digest.c

HOSTLIB_OBJS = ${HOSTLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${HOSTLIB_OBJS}


# Link with hostlib by default
LIBS = $(HOSTLIB)

# Might need this too.
CRYPTO_LIBS := $(shell ${PKG_CONFIG} --libs libcrypto)


# ----------------------------------------------------------------------------
# Now for the userspace binaries

CGPT = ${BUILD}/cgpt/cgpt

CGPT_SRCS = \
	cgpt/cgpt.c \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_common.c \
	cgpt/cgpt_create.c \
	cgpt/cgpt_find.c \
	cgpt/cgpt_legacy.c \
	cgpt/cgpt_prioritize.c \
	cgpt/cgpt_repair.c \
	cgpt/cgpt_show.c \
	cgpt/cmd_add.c \
	cgpt/cmd_boot.c \
	cgpt/cmd_create.c \
	cgpt/cmd_find.c \
	cgpt/cmd_legacy.c \
	cgpt/cmd_prioritize.c \
	cgpt/cmd_repair.c \
	cgpt/cmd_show.c

CGPT_OBJS = ${CGPT_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${CGPT_OBJS}

C_DESTDIR = ${DESTDIR}


# Scripts to install directly (not compiled)
UTIL_SCRIPTS = \
	utility/dev_debug_vboot \
	utility/dev_make_keypair \
	utility/enable_dev_usb_boot \
	utility/vbutil_what_keys

# These utilities should be linked statically.
UTIL_NAMES_STATIC = \
	crossystem \
	dump_fmap \
	gbb_utility

UTIL_NAMES = ${UTIL_NAMES_STATIC} \
	dev_sign_file \
	dump_kernel_config \
	dumpRSAPublicKey \
	load_kernel_test \
	pad_digest_utility \
	signature_digest_utility \
	tpm_init_temp_fix \
	tpmc \
	vbutil_firmware \
	vbutil_kernel \
	vbutil_key \
	vbutil_keyblock \
	verify_data

ifneq (${IN_CHROOT},)
UTIL_NAMES += mount-encrypted
endif

ifeq (${MINIMAL},)
UTIL_NAMES += \
	bmpblk_font \
	bmpblk_utility \
	eficompress \
	efidecompress
endif

UTIL_BINS_STATIC := $(addprefix ${BUILD}/utility/,${UTIL_NAMES_STATIC})
UTIL_BINS = $(addprefix ${BUILD}/utility/,${UTIL_NAMES})
ALL_DEPS += $(addsuffix .d,${UTIL_BINS})

U_DESTDIR = ${DESTDIR}


# The unified firmware utility will eventually replace all the others
FUTIL_BIN = ${BUILD}/futility/futility

FUTIL_SRCS = \
	futility/IGNOREME.c

FUTIL_LDS = futility/futility.lds

FUTIL_OBJS = ${FUTIL_SRCS:%.c=${BUILD}/%.o}

ALL_DEPS += $(addsuffix .d,${FUTIL_BIN})
ALL_OBJS += ${FUTIL_OBJS}

F_DESTDIR = ${DESTDIR}


# Library of handy test functions.
TESTLIB = ${BUILD}/tests/test.a

TESTLIB_SRCS = \
	tests/test_common.c \
	tests/timer_utils.c \
	tests/crc32_test.c

TESTLIB_OBJS = ${TESTLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${TESTLIB_OBJS}


# And some compiled tests.
TEST_NAMES = \
	cgptlib_test \
	rollback_index2_tests \
	rollback_index3_tests \
	rsa_padding_test \
	rsa_utility_tests \
	rsa_verify_benchmark \
	sha_benchmark \
	sha_tests \
	stateful_util_tests \
	tlcl_tests \
	tpm_bootmode_tests \
	utility_string_tests \
	utility_tests \
	vboot_api_init_tests \
	vboot_api_devmode_tests \
	vboot_api_firmware_tests \
	vboot_api_kernel_tests \
	vboot_audio_tests \
	vboot_common_tests \
	vboot_common2_tests \
	vboot_common3_tests \
	vboot_display_tests \
	vboot_firmware_tests \
	vboot_nvstorage_test

# Grrr
ifneq (${IN_CHROOT},)
TEST_NAMES += CgptManagerTests
endif

# TODO: port these tests to new API, if not already eqivalent
# functionality in other tests.  These don't even compile at present.
#
#		big_firmware_tests
#		big_kernel_tests
#		firmware_image_tests
#		firmware_rollback_tests
#		firmware_splicing_tests
#		firmware_verify_benchmark
#		kernel_image_tests
#		kernel_rollback_tests
#		kernel_splicing_tests
#		kernel_verify_benchmark
#		rollback_index_test
#		verify_firmware_fuzz_driver
#		verify_kernel_fuzz_driver
#               utility/load_firmware_test

# And a few more...
TLCL_TESTS = \
	tpmtest_earlyextend \
	tpmtest_earlynvram \
        tpmtest_earlynvram2 \
	tpmtest_enable \
	tpmtest_fastenable \
	tpmtest_globallock \
        tpmtest_redefine_unowned \
        tpmtest_spaceperm \
	tpmtest_testsetup \
	tpmtest_timing \
        tpmtest_writelimit
TLCL_TEST_NAMES = $(addprefix tpm_lite/,${TLCL_TESTS})
TLCL_TEST_BINS = $(addprefix ${BUILD}/tests/,${TLCL_TEST_NAMES})

TEST_NAMES += ${TLCL_TEST_NAMES}

TEST_BINS = $(addprefix ${BUILD}/tests/,${TEST_NAMES})
ALL_DEPS += $(addsuffix .d,${TEST_BINS})

# Directory containing test keys
TEST_KEYS = ${SRC_RUN}/tests/testkeys

# ----------------------------------------------------------------------------
# TODO: why not make this include *all* the cgpt files, and simply have
# cgpt link against it?
# TODO: CgptManager.cc should move to the installer project.  Shouldn't be
# in libcgpt-cc.a.
AU_CGPTLIB_SRCS = \
	cgpt/CgptManager.cc \
	cgpt/cgpt_create.c \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_show.c \
	cgpt/cgpt_repair.c \
	cgpt/cgpt_prioritize.c \
	cgpt/cgpt_common.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/stub/utility_stub.c

AU_CGPTLIB_OBJS = $(filter %.o, \
	${AU_CGPTLIB_SRCS:%.c=${BUILD}/%.o} \
	${AU_CGPTLIB_SRCS:%.cc=${BUILD}/%.o})
ALL_OBJS += ${AU_CGPTLIB_OBJS}


##############################################################################
# Finally, some targets. High-level ones first.

# Create output directories if necessary.  Do this via explicit shell commands
# so it happens before trying to generate/include dependencies.
SUBDIRS := firmware host cgpt utility futility tests tests/tpm_lite
_dir_create := $(foreach d, \
	$(shell find ${SUBDIRS} -name '*.c' -exec  dirname {} \; | sort -u), \
	$(shell [ -d ${BUILD}/${d} ] || mkdir -p ${BUILD}/${d}))


# Default target.
.PHONY: all
all: fwlib $(if ${FIRMWARE_ARCH},,host_stuff) $(if ${COV},coverage)

# Host targets
.PHONY: host_stuff
host_stuff: hostlib cgpt utils futil tests

# AU targets
.PHONY: au_stuff
au_stuff: libcgpt_cc libdump_kernel_config cgptmanager_tests

.PHONY: clean
clean:
	${Q}/bin/rm -rf ${BUILD}

.PHONY: install
install: cgpt_install utils_install futil_install

# Don't delete intermediate object files
.SECONDARY:

# TODO: I suspect this is missing some object files.  Make a temp
# target which cleans all known obj/exe's and see what's left; those
# are the files which need deps.
ALL_DEPS += ${ALL_OBJS:%.o=%.o.d}
-include ${ALL_DEPS}

# ----------------------------------------------------------------------------
# Firmware library

# TPM-specific flags.  These depend on the particular TPM we're targeting for.
# They are needed here only for compiling parts of the firmware code into
# user-level tests.

# TPM_BLOCKING_CONTINUESELFTEST is defined if TPM_ContinueSelfTest blocks until
# the self test has completed.

${FWLIB_OBJS}: CFLAGS += -DTPM_BLOCKING_CONTINUESELFTEST

# TPM_MANUAL_SELFTEST is defined if the self test must be started manually
# (with a call to TPM_ContinueSelfTest) instead of starting automatically at
# power on.
#
# We sincerely hope that TPM_BLOCKING_CONTINUESELFTEST and TPM_MANUAL_SELFTEST
# are not both defined at the same time.  (See comment in code.)

# CFLAGS += -DTPM_MANUAL_SELFTEST

ifeq (${FIRMWARE_ARCH},i386)
# Unrolling loops in cryptolib makes it faster
${FWLIB_OBJS}: CFLAGS += -DUNROLL_LOOPS

# Workaround for coreboot on x86, which will power off asynchronously
# without giving us a chance to react. This is not an example of the Right
# Way to do things. See chrome-os-partner:7689, and the commit message
# that made this change.
${FWLIB_OBJS}: CFLAGS += -DSAVE_LOCALE_IMMEDIATELY

# On x86 we don't actually read the GBB data into RAM until it is needed.
# Therefore it makes sense to cache it rather than reading it each time.
# Enable this feature.
${FWLIB_OBJS}: CFLAGS += -DCOPY_BMP_DATA
endif

ifeq (${FIRMWARE_ARCH},)
# Disable rollback TPM when compiling locally, since otherwise
# load_kernel_test attempts to talk to the TPM.
${FWLIB_OBJS}: CFLAGS += -DDISABLE_ROLLBACK_TPM
endif

.PHONY: fwlib
fwlib: ${FWLIB} $(if ${FIRMWARE_ARCH},,${BUILD}/firmware/linktest/main)

${FWLIB}: ${FWLIB_OBJS}
	@printf "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@printf "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

# ----------------------------------------------------------------------------
# Host library

.PHONY: hostlib
hostlib: ${HOSTLIB} ${BUILD}/host/linktest/main

${BUILD}/host/% ${HOSTLIB}: INCLUDES += \
	-Ihost/include\
	-Ihost/arch/${ARCH}/include

# TODO: better way to make .a than duplicating this recipe each time?
${HOSTLIB}: ${HOSTLIB_OBJS} ${FWLIB_OBJS}
	@printf "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@printf "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

# ----------------------------------------------------------------------------
# CGPT library and utility

.PHONY: cgpt
cgpt: ${CGPT}

${CGPT}: LDFLAGS += -static
${CGPT}: LDLIBS += -luuid

${CGPT}: ${CGPT_OBJS} ${LIBS}
	@printf "    LDcgpt        $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o ${CGPT} ${CFLAGS} $^ ${LDFLAGS} ${LDLIBS}

.PHONY: cgpt_install
cgpt_install: ${CGPT}
	@printf "    INSTALL       CGPT\n"
	${Q}mkdir -p ${C_DESTDIR}
	${Q}${INSTALL} -t ${C_DESTDIR} $^

# ----------------------------------------------------------------------------
# Utilities

# These have their own headers too.
${BUILD}/utility/%: INCLUDES += -Ihost/include -Iutility/include

# Utilities for auto-update toolkits must be statically linked.
${UTIL_BINS_STATIC}: LDFLAGS += -static

.PHONY: utils
utils: ${UTIL_BINS} ${UTIL_SCRIPTS}
# TODO: change ebuild to pull scripts directly out of utility dir
	${Q}cp -f ${UTIL_SCRIPTS} ${BUILD}/utility
	${Q}chmod a+rx $(patsubst %,${BUILD}/%,${UTIL_SCRIPTS})

.PHONY: utils_install
utils_install: ${UTIL_BINS} ${UTIL_SCRIPTS}
	@printf "    INSTALL       UTILS\n"
	${Q}mkdir -p ${U_DESTDIR}
	${Q}${INSTALL} -t ${U_DESTDIR} $^

# ----------------------------------------------------------------------------
# new Firmware Utility

.PHONY: futil
futil: ${FUTIL_BIN}

${FUTIL_BIN}: ${FUTIL_LDS} ${FUTIL_OBJS}
	@printf "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o $@ ${CFLAGS} $^ ${LDFLAGS} ${LDLIBS}

.PHONY: futil_install
futil_install: ${FUTIL_BIN}
	@printf "    INSTALL       futility\n"
	${Q}mkdir -p ${F_DESTDIR}
	${Q}${INSTALL} -t ${F_DESTDIR} $^


# ----------------------------------------------------------------------------
# Mount-encrypted utility for cryptohome

# TODO: mount-encrypted should move to cryptohome and just link against
# vboot-host.a for tlcl and crossystem.

# The embedded libcrypto conflicts with the shipped openssl,
# so mount-* builds without the common CFLAGS (and those includes).

${BUILD}/utility/mount-helpers.o: \
		utility/mount-helpers.c \
		utility/mount-helpers.h \
		utility/mount-encrypted.h
	@printf "    CCm-e         $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} -Wall -Werror -O2 -D_FORTIFY_SOURCE=2 -fstack-protector \
		${COV_FLAGS} \
		$(shell ${PKG_CONFIG} --cflags glib-2.0 openssl) \
		-c $< -o $@

${BUILD}/utility/mount-encrypted: \
		utility/mount-encrypted.c \
		utility/mount-encrypted.h \
		${BUILD}/utility/mount-helpers.o ${LIBS}
	@printf "    CCm-exe       $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} -Wall -Werror -O2 -D_FORTIFY_SOURCE=2 -fstack-protector \
		$(shell ${PKG_CONFIG} --cflags glib-2.0 openssl) \
		-Ifirmware/include \
		-Ihost/include \
		${COV_FLAGS} \
		${LDFLAGS} \
		$< -o $@ \
		${BUILD}/utility/mount-helpers.o ${LIBS} \
		$(shell ${PKG_CONFIG} --libs glib-2.0 openssl) \
		-lm
ifneq (${COV},)
	${Q}mv -f mount-encrypted.gcno ${BUILD}/utility
endif

# ----------------------------------------------------------------------------
# Utility to generate TLCL structure definition header file.

${BUILD}/utility/tlcl_generator: CFLAGS += -fpack-struct

STRUCTURES_TMP=${BUILD}/tlcl_structures.tmp
STRUCTURES_SRC=firmware/lib/tpm_lite/include/tlcl_structures.h

.PHONY: update_tlcl_structures
update_tlcl_structures: ${BUILD}/utility/tlcl_generator
	@printf "    Rebuilding TLCL structures\n"
	${Q}${BUILD}/utility/tlcl_generator > ${STRUCTURES_TMP}
	${Q}cmp -s ${STRUCTURES_TMP} ${STRUCTURES_SRC} || \
		( echo "%% Updating structures.h %%" && \
		  cp ${STRUCTURES_TMP} ${STRUCTURES_SRC} )

# ----------------------------------------------------------------------------
# Library to dump kernel config
# Used by platform/installer, as well as standalone utility.

.PHONY: libdump_kernel_config
libdump_kernel_config: ${DUMPKERNELCONFIGLIB}

${DUMPKERNELCONFIGLIB}: ${BUILD}/utility/dump_kernel_config_lib.o
	@printf "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@printf "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

# ----------------------------------------------------------------------------
# And this thing.

.PHONY: libcgpt_cc
libcgpt_cc: ${AU_CGPTLIB}

${AU_CGPTLIB}: INCLUDES += -Ifirmware/lib/cgptlib/include
${AU_CGPTLIB}: ${AU_CGPTLIB_OBJS}
	@printf "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@printf "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

# ----------------------------------------------------------------------------
# Tests

.PHONY: tests
tests: ${TEST_BINS}

${TEST_BINS}: ${TESTLIB}

${TESTLIB}: ${TESTLIB_OBJS}
	@printf "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@printf "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^


# ----------------------------------------------------------------------------
# Generic build rules. LIBS and OBJS can be overridden to tweak the generic
# rules for specific targets.

${BUILD}/%: ${BUILD}/%.o ${OBJS} ${LIBS}
	@printf "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o $@ ${CFLAGS} $< ${OBJS} ${LIBS} ${LDFLAGS} ${LDLIBS}

${BUILD}/%.o: %.c
	@printf "    CC            $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# Rules to recompile a single source file for library and test
# TODO: is there a tidier way to do this?
${BUILD}/%_for_lib.o: CFLAGS += -DFOR_LIBRARY
${BUILD}/%_for_lib.o: %.c
	@printf "    CC-for-lib    $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

${BUILD}/%_for_test.o: CFLAGS += -DFOR_TEST
${BUILD}/%_for_test.o: %.c
	@printf "    CC-for-test   $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# TODO: C++ files don't belong in vboot reference at all.  Convert to C.
${BUILD}/%.o: %.cc
	@printf "    CXX           $(subst ${BUILD}/,,$@)\n"
	${Q}${CXX} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# ----------------------------------------------------------------------------
# Here are the special tweaks to the generic rules.

# Linktest ensures firmware lib doesn't rely on outside libraries
${BUILD}/firmware/linktest/main: LIBS = ${FWLIB}

# Specific dependency here.
${BUILD}/utility/dump_kernel_config: LIBS += ${DUMPKERNELCONFIGLIB}
${BUILD}/utility/dump_kernel_config: ${DUMPKERNELCONFIGLIB}

# GBB utility needs C++ linker. TODO: It shouldn't.
${BUILD}/utility/gbb_utility: LD = ${CXX}

# Some utilities need external crypto functions
${BUILD}/utility/dumpRSAPublicKey: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/pad_digest_utility: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/signature_digest_utility: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/dev_sign_file: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/vbutil_firmware: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/vbutil_kernel: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/vbutil_key: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/vbutil_keyblock: LDLIBS += ${CRYPTO_LIBS}

${BUILD}/host/linktest/main: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/vboot_common2_tests: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/vboot_common3_tests: LDLIBS += ${CRYPTO_LIBS}

${BUILD}/utility/bmpblk_utility: LD = ${CXX}
${BUILD}/utility/bmpblk_utility: LDLIBS = -llzma -lyaml

BMPBLK_UTILITY_DEPS = \
	${BUILD}/utility/bmpblk_util.o \
	${BUILD}/utility/image_types.o \
	${BUILD}/utility/eficompress_for_lib.o \
	${BUILD}/utility/efidecompress_for_lib.o
${BUILD}/utility/bmpblk_utility: OBJS = ${BMPBLK_UTILITY_DEPS}
${BUILD}/utility/bmpblk_utility: ${BMPBLK_UTILITY_DEPS}

${BUILD}/utility/bmpblk_font: OBJS += ${BUILD}/utility/image_types.o
${BUILD}/utility/bmpblk_font: ${BUILD}/utility/image_types.o

# Allow multiple definitions, so tests can mock functions from other libraries
${BUILD}/tests/%: CFLAGS += -Xlinker --allow-multiple-definition
${BUILD}/tests/%: INCLUDES += -Ihost/include
${BUILD}/tests/%: LDLIBS += -lrt -luuid
${BUILD}/tests/%: LIBS += ${TESTLIB}

${BUILD}/tests/rollback_index2_tests: OBJS += \
	${BUILD}/firmware/lib/rollback_index_for_test.o
${BUILD}/tests/rollback_index2_tests: \
	${BUILD}/firmware/lib/rollback_index_for_test.o

${BUILD}/tests/tlcl_tests: OBJS += \
	${BUILD}/firmware/lib/tpm_lite/tlcl_for_test.o
${BUILD}/tests/tlcl_tests: \
	${BUILD}/firmware/lib/tpm_lite/tlcl_for_test.o

${BUILD}/tests/vboot_audio_tests: OBJS += \
	${BUILD}/firmware/lib/vboot_audio_for_test.o
${BUILD}/tests/vboot_audio_tests: \
	${BUILD}/firmware/lib/vboot_audio_for_test.o

.PHONY: cgptmanager_tests
cgptmanager_tests: ${BUILD}/tests/CgptManagerTests

${BUILD}/tests/CgptManagerTests: CFLAGS += ${PC_CFLAGS}
${BUILD}/tests/CgptManagerTests: LD = ${CXX}
${BUILD}/tests/CgptManagerTests: LDLIBS += -lgtest -lgflags ${PC_LDLIBS}
${BUILD}/tests/CgptManagerTests: LIBS = ${AU_CGPTLIB}
${BUILD}/tests/CgptManagerTests: ${AU_CGPTLIB}

${BUILD}/tests/rollback_index_test: INCLUDES += -I/usr/include
${BUILD}/tests/rollback_index_test: LIBS += -ltlcl

${TLCL_TEST_BINS}: OBJS += ${BUILD}/tests/tpm_lite/tlcl_tests.o
${TLCL_TEST_BINS}: ${BUILD}/tests/tpm_lite/tlcl_tests.o

##############################################################################
# Targets that exist just to run tests

# Frequently-run tests
.PHONY: test_targets
test_targets:: runcgpttests runmisctests

ifeq (${MINIMAL},)
# Bitmap utility isn't compiled for minimal variant
test_targets:: runbmptests
# Scripts don't work under qemu testing
# TODO: convert scripts to makefile so they can be called directly
test_targets:: runtestscripts
endif

.PHONY: test_setup
test_setup:: cgpt utils futil tests

# Qemu setup for cross-compiled tests.  Need to copy qemu binary into the
# sysroot.
ifneq (${QEMU_ARCH},)
test_setup:: qemu_install

.PHONY: qemu_install
qemu_install:
ifeq (${SYSROOT},)
	$(error SYSROOT must be set to the top of the target-specific root \
when cross-compiling for qemu-based tests to run properly.)
endif
	@printf "    Copying qemu binary.\n"
	${Q}cp -fu /usr/bin/${QEMU_BIN} ${BUILD}/${QEMU_BIN}
	${Q}chmod a+rx ${BUILD}/${QEMU_BIN}
endif

.PHONY: runtests
runtests: test_targets

# Generate test keys
.PHONY: genkeys
genkeys: utils
	tests/gen_test_keys.sh

# Generate test cases for fuzzing
.PHONY: genfuzztestcases
genfuzztestcases: utils
	tests/gen_fuzz_test_cases.sh

.PHONY: runbmptests
runbmptests: test_setup
	cd tests/bitmaps && BMPBLK=${BUILD_RUN}/utility/bmpblk_utility \
		./TestBmpBlock.py -v

.PHONY: runcgpttests
runcgpttests: test_setup
	${RUNTEST} ${BUILD_RUN}/tests/cgptlib_test
# HEY - elsewhere
ifneq (${IN_CHROOT},)
	${RUNTEST} ${BUILD_RUN}/tests/CgptManagerTests --v=1
endif

.PHONY: runtestscripts
runtestscripts: test_setup genfuzztestcases
	tests/run_cgpt_tests.sh ${BUILD_RUN}/cgpt/cgpt
	tests/run_preamble_tests.sh
	tests/run_rsa_tests.sh
	tests/run_vbutil_kernel_arg_tests.sh
	tests/run_vbutil_tests.sh

.PHONY: runmisctests
runmisctests: test_setup
	${RUNTEST} ${BUILD_RUN}/tests/rollback_index2_tests
	${RUNTEST} ${BUILD_RUN}/tests/rollback_index3_tests
	${RUNTEST} ${BUILD_RUN}/tests/rsa_utility_tests
	${RUNTEST} ${BUILD_RUN}/tests/sha_tests
	${RUNTEST} ${BUILD_RUN}/tests/stateful_util_tests
	${RUNTEST} ${BUILD_RUN}/tests/tlcl_tests
	${RUNTEST} ${BUILD_RUN}/tests/tpm_bootmode_tests
	${RUNTEST} ${BUILD_RUN}/tests/utility_string_tests
	${RUNTEST} ${BUILD_RUN}/tests/utility_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_devmode_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_init_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_firmware_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_audio_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common2_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common3_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vboot_display_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_firmware_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_nvstorage_test

.PHONY: runfutiltests
runfutiltests: DESTDIR := ${TEST_INSTALL_DIR}
runfutiltests: test_setup install
	@echo "$@ passed"

# Run long tests, including all permutations of encryption keys (instead of
# just the ones we use) and tests of currently-unused code.
# Not run by automated build.
.PHONY: runlongtests
runlongtests: test_setup genkeys genfuzztestcases
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common2_tests ${TEST_KEYS} --all
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common3_tests ${TEST_KEYS} --all
	tests/run_preamble_tests.sh --all
	tests/run_vbutil_tests.sh --all

# TODO: tests to run when ported to new API
#	./run_image_verification_tests.sh
#	# Splicing tests
#	${BUILD}/tests/firmware_splicing_tests
#	${BUILD}/tests/kernel_splicing_tests
#	# Rollback Tests
#	${BUILD}/tests/firmware_rollback_tests
#	${BUILD}/tests/kernel_rollback_tests

# Code coverage
.PHONY: coverage_init
coverage_init: test_setup
	rm -f ${COV_INFO}*
	lcov -c -i -d . -b . -o ${COV_INFO}.initial

.PHONY: coverage_html
coverage_html:
	lcov -c -d . -b . -o ${COV_INFO}.tests
	lcov -a ${COV_INFO}.initial -a ${COV_INFO}.tests -o ${COV_INFO}.total
	lcov -r ${COV_INFO}.total '/usr/*' '*/linktest/*' -o ${COV_INFO}.local
	genhtml ${COV_INFO}.local -o ${BUILD}/coverage

# Generate addtional coverage stats just for firmware subdir, because the
# per-directory stats for the whole project don't include their own subdirs.
	lcov -e ${COV_INFO}.local '${SRCDIR}/firmware/*' \
		-o ${COV_INFO}.firmware

.PHONY: coverage
ifeq (${COV},)
coverage:
	$(error Build coverage like this: make clean && COV=1 make)
else
coverage: coverage_init runtests coverage_html
endif

