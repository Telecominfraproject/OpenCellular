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

# We should only run pwd once, not every time we refer to ${BUILD}.
SRCDIR := $(shell pwd)
BUILD = $(SRCDIR)/build
export BUILD

# Stuff for 'make install'
INSTALL = install
DESTDIR = /usr/local/bin
OLDDIR = old_bins

# Where exactly do the pieces go?
#  FT_DIR = futility target directory - where it will be on the target
#  F_DIR  = futility install directory - where it gets put right now
#  UB_DIR = userspace binary directory for futility's exec() targets
#  VB_DIR = target vboot directory - for dev-mode-only helpers, keys, etc.
ifeq (${MINIMAL},)
# Host install just puts everything in one place
FT_DIR=${DESTDIR}
F_DIR=${DESTDIR}
UB_DIR=${DESTDIR}/${OLDDIR}
else
# Target install puts things into DESTDIR subdirectories
FT_DIR=/usr/bin
F_DIR=${DESTDIR}${FT_DIR}
UB_DIR=${F_DIR}/${OLDDIR}
VB_DIR=${DESTDIR}/usr/share/vboot/bin
endif

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

ifneq (${OLDDIR},)
CFLAGS += -DOLDDIR=${OLDDIR}
endif

ifneq (${DEBUG},)
CFLAGS += -DVBOOT_DEBUG
endif

ifeq (${DISABLE_NDEBUG},)
CFLAGS += -DNDEBUG
endif

ifneq (${FORCE_LOGGING_ON},)
CFLAGS += -DFORCE_LOGGING_ON=${FORCE_LOGGING_ON}
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

##############################################################################
# Now we need to describe everything we might want or need to build

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

# Firmware library sources needed by VbInit() call
VBINIT_SRCS = \
	firmware/lib/crc8.c \
	firmware/lib/utility.c \
	firmware/lib/vboot_api_init.c \
	firmware/lib/vboot_common_init.c \
	firmware/lib/vboot_nvstorage.c \

# Additional firmware library sources needed by VbSelectFirmware() call
VBSF_SRCS = \
	firmware/lib/cryptolib/padding.c \
	firmware/lib/cryptolib/rsa.c \
	firmware/lib/cryptolib/rsa_utility.c \
	firmware/lib/cryptolib/sha1.c \
	firmware/lib/cryptolib/sha256.c \
	firmware/lib/cryptolib/sha512.c \
	firmware/lib/cryptolib/sha_utility.c \
	firmware/lib/stateful_util.c \
	firmware/lib/vboot_api_firmware.c \
	firmware/lib/vboot_common.c \
	firmware/lib/vboot_firmware.c

# Additional firmware library sources needed by VbSelectAndLoadKernel() call
VBSLK_SRCS = \
	firmware/lib/cgptlib/cgptlib.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/utility_string.c \
	firmware/lib/vboot_api_kernel.c \
	firmware/lib/vboot_audio.c \
	firmware/lib/vboot_display.c \
	firmware/lib/vboot_kernel.c

# Support real TPM unless BIOS sets MOCK_TPM
ifeq (${MOCK_TPM},)
VBINIT_SRCS += \
	firmware/lib/rollback_index.c \
	firmware/lib/tpm_lite/tlcl.c

VBSF_SRCS += \
	firmware/lib/tpm_bootmode.c
else
VBINIT_SRCS += \
	firmware/lib/mocked_rollback_index.c \
	firmware/lib/tpm_lite/mocked_tlcl.c

VBSF_SRCS += \
	firmware/lib/mocked_tpm_bootmode.c
endif

ifeq (${FIRMWARE_ARCH},)
# Include BIOS stubs in the firmware library when compiling for host
# TODO: split out other stub funcs too
VBINIT_SRCS += \
	firmware/stub/tpm_lite_stub.c \
	firmware/stub/utility_stub.c \
	firmware/stub/vboot_api_stub_init.c

VBSF_SRCS += \
	firmware/stub/vboot_api_stub_sf.c

VBSLK_SRCS += \
	firmware/stub/vboot_api_stub.c \
	firmware/stub/vboot_api_stub_disk.c
endif

VBSF_SRCS += ${VBINIT_SRCS}
FWLIB_SRCS += ${VBSF_SRCS} ${VBSLK_SRCS}

VBINIT_OBJS = ${VBINIT_SRCS:%.c=${BUILD}/%.o}
VBSF_OBJS = ${VBSF_SRCS:%.c=${BUILD}/%.o}

FWLIB_OBJS = ${FWLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${FWLIB_OBJS}


# Library to build the utilities. "HOST" mostly means "userspace".
HOSTLIB = ${BUILD}/libvboot_host.a

HOSTLIB_SRCS = \
	cgpt/cgpt_create.c \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_show.c \
	cgpt/cgpt_repair.c \
	cgpt/cgpt_prioritize.c \
	cgpt/cgpt_common.c \
	host/arch/${ARCH}/lib/crossystem_arch.c \
	host/lib/crossystem.c \
	host/lib/file_keys.c \
	host/lib/fmap.c \
	host/lib/host_common.c \
	host/lib/host_key.c \
	host/lib/host_keyblock.c \
	host/lib/host_misc.c \
	host/lib/host_signature.c \
	host/lib/signature_digest.c \
	utility/dump_kernel_config_lib.c

HOSTLIB_OBJS = ${HOSTLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${HOSTLIB_OBJS}

# Link with hostlib by default
LIBS = $(HOSTLIB)

# Might need this too.
CRYPTO_LIBS := $(shell ${PKG_CONFIG} --libs libcrypto)

# Sigh. For historical reasons, the autoupdate installer must sometimes be a
# 32-bit executable, even when everything else is 64-bit. But it only needs a
# few functions, so let's just build those.
TINYHOSTLIB = ${BUILD}/libtinyvboot_host.a

TINYHOSTLIB_SRCS = \
	cgpt/cgpt_create.c \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_show.c \
	cgpt/cgpt_repair.c \
	cgpt/cgpt_prioritize.c \
	cgpt/cgpt_common.c \
	utility/dump_kernel_config_lib.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/lib/utility_string.c \
	firmware/stub/utility_stub.c

TINYHOSTLIB_OBJS = ${TINYHOSTLIB_SRCS:%.c=${BUILD}/%.o}

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


# Scripts to install directly (not compiled)
UTIL_SCRIPTS = \
	utility/dev_debug_vboot \
	utility/enable_dev_usb_boot \
	utility/vbutil_what_keys

ifeq (${MINIMAL},)
UTIL_SCRIPTS += \
	utility/dev_make_keypair
endif

# These utilities should be linked statically.
UTIL_NAMES_STATIC = \
	crossystem \
	dump_fmap \
	gbb_utility

UTIL_NAMES = ${UTIL_NAMES_STATIC} \
	dev_sign_file \
	dump_kernel_config \
	dumpRSAPublicKey \
	tpm_init_temp_fix \
	tpmc \
	vbutil_firmware \
	vbutil_kernel \
	vbutil_key \
	vbutil_keyblock \

ifeq (${MINIMAL},)
UTIL_NAMES += \
	bmpblk_font \
	bmpblk_utility \
	eficompress \
	efidecompress \
	load_kernel_test \
	pad_digest_utility \
	signature_digest_utility \
	verify_data
endif

UTIL_BINS_STATIC := $(addprefix ${BUILD}/utility/,${UTIL_NAMES_STATIC})
UTIL_BINS = $(addprefix ${BUILD}/utility/,${UTIL_NAMES})
ALL_DEPS += $(addsuffix .d,${UTIL_BINS})


# Scripts for signing stuff.
SIGNING_SCRIPTS = \
	utility/tpm-nvsize \
	utility/chromeos-tpm-recovery

# These go in a different place.
SIGNING_SCRIPTS_DEV = \
	scripts/image_signing/resign_firmwarefd.sh \
	scripts/image_signing/make_dev_firmware.sh \
	scripts/image_signing/make_dev_ssd.sh \
	scripts/image_signing/set_gbb_flags.sh

# Installed, but not made executable.
SIGNING_COMMON = scripts/image_signing/common_minimal.sh


# The unified firmware utility will eventually replace all the others
FUTIL_BIN = ${BUILD}/futility/futility

# These are the others it will replace.
FUTIL_OLD = $(notdir ${CGPT} ${UTIL_BINS} ${UTIL_SCRIPTS} \
		${SIGNING_SCRIPTS} ${SIGNING_SCRIPTS_DEV})

FUTIL_SRCS = \
	futility/futility.c \
	futility/cmd_foo.c

FUTIL_LDS = futility/futility.lds

FUTIL_OBJS = ${FUTIL_SRCS:%.c=${BUILD}/%.o}

ALL_DEPS += $(addsuffix .d,${FUTIL_BIN})
ALL_OBJS += ${FUTIL_OBJS}


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
	vboot_api_kernel2_tests \
	vboot_api_kernel3_tests \
	vboot_api_kernel4_tests \
	vboot_audio_tests \
	vboot_common_tests \
	vboot_common2_tests \
	vboot_common3_tests \
	vboot_display_tests \
	vboot_firmware_tests \
	vboot_kernel_tests \
	vboot_nvstorage_test

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

.PHONY: clean
clean:
	${Q}/bin/rm -rf ${BUILD}

.PHONY: install
install: cgpt_install utils_install signing_install futil_install

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

# Link tests
${BUILD}/firmware/linktest/main_vbinit: LIBS =
${BUILD}/firmware/linktest/main_vbinit: OBJS = ${VBINIT_OBJS}
${BUILD}/firmware/linktest/main_vbsf: LIBS =
${BUILD}/firmware/linktest/main_vbsf: OBJS = ${VBSF_OBJS}

.phony: fwlinktest
fwlinktest: ${FWLIB} \
	${BUILD}/firmware/linktest/main_vbinit \
	${BUILD}/firmware/linktest/main_vbsf \
	${BUILD}/firmware/linktest/main

.PHONY: fwlib
fwlib: ${FWLIB} $(if ${FIRMWARE_ARCH},,fwlinktest)

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


# Ugh. This is a very cut-down version of HOSTLIB just for the installer.
.PHONY: tinyhostlib
tinyhostlib: ${TINYHOSTLIB}
	${Q}cp -f ${TINYHOSTLIB} ${HOSTLIB}

${TINYHOSTLIB}: ${TINYHOSTLIB_OBJS}
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
	${Q}mkdir -p ${UB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} $^

# ----------------------------------------------------------------------------
# Utilities

# These have their own headers too.
${BUILD}/utility/%: INCLUDES += -Ihost/include -Iutility/include

# Utilities for auto-update toolkits must be statically linked.
${UTIL_BINS_STATIC}: LDFLAGS += -static

.PHONY: utils
utils: ${UTIL_BINS} ${UTIL_SCRIPTS}
	${Q}cp -f ${UTIL_SCRIPTS} ${BUILD}/utility
	${Q}chmod a+rx $(patsubst %,${BUILD}/%,${UTIL_SCRIPTS})

.PHONY: utils_install
utils_install: ${UTIL_BINS} ${UTIL_SCRIPTS}
	@printf "    INSTALL       UTILS\n"
	${Q}mkdir -p ${UB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} ${UTIL_BINS} ${UTIL_SCRIPTS}

# And some signing stuff for the target
.PHONY: signing_install
signing_install: ${SIGNING_SCRIPTS} ${SIGNING_SCRIPTS_DEV} ${SIGNING_COMMON}
	@printf "    INSTALL       SIGNING\n"
	${Q}mkdir -p ${UB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} ${SIGNING_SCRIPTS}
	${Q}${INSTALL} -t ${UB_DIR} ${SIGNING_SCRIPTS_DEV}
	${Q}${INSTALL} -t ${UB_DIR} -m 'u=rw,go=r,a-s' ${SIGNING_COMMON}
ifneq (${VB_DIR},)
	${Q}mkdir -p ${VB_DIR}
	${Q}for prog in $(notdir ${SIGNING_SCRIPTS_DEV}); do \
		ln -sf "${FT_DIR}/futility" "${VB_DIR}/$$prog"; done
endif

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
	${Q}mkdir -p ${F_DIR}
	${Q}${INSTALL} -t ${F_DIR} ${FUTIL_BIN}
	${Q}for prog in ${FUTIL_OLD}; do \
		ln -sf futility "${F_DIR}/$$prog"; done

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

# GBB utility needs C++ linker. TODO: It shouldn't.
${BUILD}/utility/gbb_utility: LD = ${CXX}

# Because we play some clever linker script games to add new commands without
# changing any header files, futility must be linked with ld.bfd, not gold.
${FUTIL_BIN}: LDFLAGS += -fuse-ld=bfd

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
test_targets:: runbmptests runfutiltests
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
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_firmware_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_init_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_kernel_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_kernel2_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_kernel3_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_api_kernel4_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_audio_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common2_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common3_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vboot_display_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_firmware_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_kernel_tests
	${RUNTEST} ${BUILD_RUN}/tests/vboot_nvstorage_test

.PHONY: runfutiltests
runfutiltests: override DESTDIR = ${TEST_INSTALL_DIR}
runfutiltests: test_setup install
	futility/tests/run_futility_tests.sh ${DESTDIR}

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
	lcov -r ${COV_INFO}.local '*/stub/*' -o ${COV_INFO}.nostub
	lcov -e ${COV_INFO}.nostub '${SRCDIR}/firmware/*' \
		-o ${COV_INFO}.firmware

.PHONY: coverage
ifeq (${COV},)
coverage:
	$(error Build coverage like this: make clean && COV=1 make)
else
coverage: coverage_init runtests coverage_html
endif
