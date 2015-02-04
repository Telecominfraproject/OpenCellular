# Copyright 2013 The Chromium OS Authors. All rights reserved.
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
export SRCDIR
BUILD = ${SRCDIR}/build
export BUILD

# Stuff for 'make install'
INSTALL = install
DESTDIR = /usr/local
LIBDIR ?= lib

# Default values
DEV_DEBUG_FORCE=

# Where exactly do the pieces go?
#  UB_DIR = utility binary directory
#  ULP_DIR = pkgconfig directory, usually /usr/lib/pkgconfig
#  DF_DIR = utility defaults directory
#  VB_DIR = vboot binary directory for dev-mode-only scripts
ifeq (${MINIMAL},)
# Host install just puts everything where it's told
UB_DIR=${DESTDIR}/bin
ULP_DIR=${DESTDIR}/${LIBDIR}/pkgconfig
DF_DIR=${DESTDIR}/default
VB_DIR=${DESTDIR}/bin
else
# Target install puts things into different places
UB_DIR=${DESTDIR}/usr/bin
ULP_DIR=${DESTDIR}/usr/${LIBDIR}/pkgconfig
DF_DIR=${DESTDIR}/etc/default
VB_DIR=${DESTDIR}/usr/share/vboot/bin
endif

# Where to install the (exportable) executables for testing?
TEST_INSTALL_DIR = ${BUILD}/install_for_test

# Verbose? Use V=1
ifeq (${V},)
Q := @
endif

# Quiet? Use QUIET=1
ifeq (${QUIET},)
PRINTF := printf
else
PRINTF := :
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
else ifeq (${FIRMWARE_ARCH},armv7)
  override FIRMWARE_ARCH := arm
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
	-mpreferred-stack-boundary=2 \
	${COMMON_FLAGS}
else ifeq (${FIRMWARE_ARCH}, x86_64)
CFLAGS ?= ${COMMON_FLAGS} \
	-fvisibility=hidden -fno-strict-aliasing -fomit-frame-pointer
else
# FIRMWARE_ARCH not defined; assuming local compile.
CC ?= gcc
CFLAGS += -DCHROMEOS_ENVIRONMENT -Wall -Werror ${DEBUG_FLAGS}
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

ifneq (${PD_SYNC},)
CFLAGS += -DPD_SYNC
endif

ifneq (${USE_MTD},)
CFLAGS += -DUSE_MTD
LDLIBS += -lmtdutils
endif

# NOTE: We don't use these files but they are useful for other packages to
# query about required compiling/linking flags.
PC_IN_FILES = vboot_host.pc.in

# Create / use dependency files
CFLAGS += -MMD -MF $@.d

ifeq (${FIRMWARE_ARCH},)
# Creates position independent code for non firmware target.
CFLAGS += -fPIE
endif

# These are required to access large disks and files on 32-bit systems.
CFLAGS += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

# Code coverage
ifneq (${COV},)
  COV_FLAGS = -O0 --coverage -DCOVERAGE
  CFLAGS += ${COV_FLAGS}
  LDFLAGS += ${COV_FLAGS}
  COV_INFO = ${BUILD}/coverage.info
endif

ifdef HAVE_MACOS
  CFLAGS += -DHAVE_MACOS -Wno-deprecated-declarations
endif

# And a few more default utilities
LD = ${CC}
CXX ?= g++
PKG_CONFIG ?= pkg-config

# Static?
ifneq (${STATIC},)
LDFLAGS += -static
PKG_CONFIG += --static
endif

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
	-Ifirmware/lib/tpm_lite/include \
	-Ifirmware/2lib/include

# If we're not building for a specific target, just stub out things like the
# TPM commands and various external functions that are provided by the BIOS.
ifeq (${FIRMWARE_ARCH},)
INCLUDES += -Ihost/include -Ihost/lib/include
endif

# Firmware library, used by the other firmware components (depthcharge,
# coreboot, etc.). It doesn't need exporting to some other place; they'll build
# this source tree locally and link to it directly.
FWLIB = ${BUILD}/vboot_fw.a

# Smaller firmware library common to all vboot 2.x, used only for
# 1) compile-time tests of the public API or
# 2) linking with an actual 2.0 or 2.1 implementation
FWLIB2X = ${BUILD}/vboot_fw2x.a

# Vboot 2.0 (deprecated - see firmware/README)
FWLIB20 = ${BUILD}/vboot_fw20.a
# Vboot 2.1 (not yet ready - see firmware/README)
FWLIB21 = ${BUILD}/vboot_fw21.a

# Firmware library sources needed by VbInit() call
VBINIT_SRCS = \
	firmware/lib/crc8.c \
	firmware/lib/utility.c \
	firmware/lib/vboot_api_init.c \
	firmware/lib/vboot_common_init.c \
	firmware/lib/vboot_nvstorage.c \
	firmware/lib/vboot_nvstorage_rollback.c \
	firmware/lib/region-init.c \

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
	firmware/lib/vboot_firmware.c \
	firmware/lib/region-fw.c \

# Additional firmware library sources needed by VbSelectAndLoadKernel() call
VBSLK_SRCS = \
	firmware/lib/cgptlib/cgptlib.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/gpt_misc.c \
	firmware/lib/utility_string.c \
	firmware/lib/vboot_api_kernel.c \
	firmware/lib/vboot_audio.c \
	firmware/lib/vboot_display.c \
	firmware/lib/vboot_kernel.c \
	firmware/lib/region-kernel.c \

# Code common to both vboot 2.0 (old structs) and 2.1 (new structs)
FWLIB2X_SRCS = \
	firmware/2lib/2api.c \
	firmware/2lib/2common.c \
	firmware/2lib/2crc8.c \
	firmware/2lib/2misc.c \
	firmware/2lib/2nvstorage.c \
	firmware/2lib/2rsa.c \
	firmware/2lib/2secdata.c \
	firmware/2lib/2sha1.c \
	firmware/2lib/2sha256.c \
	firmware/2lib/2sha512.c \
	firmware/2lib/2sha_utility.c \
	firmware/2lib/2tpm_bootmode.c

FWLIB20_SRCS = \
	firmware/lib20/api.c \
	firmware/lib20/common.c \
	firmware/lib20/misc.c \
	firmware/lib20/packed_key.c

FWLIB21_SRCS = \
	firmware/lib21/api.c \
	firmware/lib21/common.c \
	firmware/lib21/misc.c \
	firmware/lib21/packed_key.c

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
	firmware/stub/vboot_api_stub_init.c \
	firmware/stub/vboot_api_stub_region.c

VBSF_SRCS += \
	firmware/stub/vboot_api_stub_sf.c

VBSLK_SRCS += \
	firmware/stub/vboot_api_stub.c \
	firmware/stub/vboot_api_stub_disk.c \
	firmware/stub/vboot_api_stub_stream.c

FWLIB2X_SRCS += \
	firmware/2lib/2stub.c

endif

VBSF_SRCS += ${VBINIT_SRCS}
FWLIB_SRCS += ${VBSF_SRCS} ${VBSLK_SRCS}

VBINIT_OBJS = ${VBINIT_SRCS:%.c=${BUILD}/%.o}
VBSF_OBJS = ${VBSF_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS +=  ${VBINIT_OBJS} ${VBSF_OBJS}

FWLIB_OBJS = ${FWLIB_SRCS:%.c=${BUILD}/%.o}
FWLIB2X_OBJS = ${FWLIB2X_SRCS:%.c=${BUILD}/%.o}
FWLIB20_OBJS = ${FWLIB20_SRCS:%.c=${BUILD}/%.o}
FWLIB21_OBJS = ${FWLIB21_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${FWLIB_OBJS} ${FWLIB2X_OBJS} ${FWLIB20_OBJS} ${FWLIB21_OBJS}

# Intermediate library for the vboot_reference utilities to link against.
UTILLIB = ${BUILD}/libvboot_util.a
UTILLIB21 = ${BUILD}/libvboot_util21.a

UTILLIB_SRCS = \
	cgpt/cgpt_create.c \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_show.c \
	cgpt/cgpt_repair.c \
	cgpt/cgpt_prioritize.c \
	cgpt/cgpt_common.c \
	futility/dump_kernel_config_lib.c \
	host/arch/${ARCH}/lib/crossystem_arch.c \
	host/lib/crossystem.c \
	host/lib/file_keys.c \
	host/lib/fmap.c \
	host/lib/host_common.c \
	host/lib/host_key.c \
	host/lib/host_keyblock.c \
	host/lib/host_misc.c \
	host/lib/util_misc.c \
	host/lib/host_signature.c \
	host/lib/signature_digest.c

UTILLIB_OBJS = ${UTILLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${UTILLIB_OBJS}

UTILLIB21_SRCS += \
	host/lib21/host_fw_preamble.c \
	host/lib21/host_key.c \
	host/lib21/host_keyblock.c \
	host/lib21/host_misc.c \
	host/lib21/host_signature.c

UTILLIB21_OBJS = ${UTILLIB21_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${UTILLIB21_OBJS}

# Externally exported library for some target userspace apps to link with
# (cryptohome, updater, etc.)
HOSTLIB = ${BUILD}/libvboot_host.a

HOSTLIB_SRCS = \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_common.c \
	cgpt/cgpt_create.c \
	cgpt/cgpt_prioritize.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/crc8.c \
	firmware/lib/gpt_misc.c \
	firmware/lib/tpm_lite/tlcl.c \
	firmware/lib/utility_string.c \
	firmware/lib/vboot_nvstorage.c \
	firmware/stub/tpm_lite_stub.c \
	firmware/stub/utility_stub.c \
	firmware/stub/vboot_api_stub_disk.c \
	firmware/stub/vboot_api_stub_init.c \
	firmware/stub/vboot_api_stub_sf.c \
	futility/dump_kernel_config_lib.c \
	host/arch/${ARCH}/lib/crossystem_arch.c \
	host/lib/crossystem.c \
	host/lib/extract_vmlinuz.c \
	host/lib/fmap.c \
	host/lib/host_misc.c

HOSTLIB_OBJS = ${HOSTLIB_SRCS:%.c=${BUILD}/%.o}
ALL_OBJS += ${HOSTLIB_OBJS}

# Sigh. For historical reasons, the autoupdate installer must sometimes be a
# 32-bit executable, even when everything else is 64-bit. But it only needs a
# few functions, so let's just build those.
TINYHOSTLIB = ${BUILD}/libtinyvboot_host.a

TINYHOSTLIB_SRCS = \
	cgpt/cgpt_add.c \
	cgpt/cgpt_boot.c \
	cgpt/cgpt_common.c \
	cgpt/cgpt_create.c \
	cgpt/cgpt_prioritize.c \
	firmware/lib/cgptlib/cgptlib_internal.c \
	firmware/lib/cgptlib/crc32.c \
	firmware/lib/gpt_misc.c \
	firmware/lib/utility_string.c \
	firmware/stub/vboot_api_stub_disk.c \
	firmware/stub/vboot_api_stub_sf.c \
	firmware/stub/utility_stub.c \
	futility/dump_kernel_config_lib.c \
	host/lib/extract_vmlinuz.c

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
	cgpt/cgpt_nor.c \
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

CGPT_WRAPPER = ${BUILD}/cgpt/cgpt_wrapper

CGPT_WRAPPER_SRCS = \
	cgpt/cgpt_nor.c \
	cgpt/cgpt_wrapper.c

CGPT_WRAPPER_OBJS = ${CGPT_WRAPPER_SRCS:%.c=${BUILD}/%.o}

ALL_OBJS += ${CGPT_WRAPPER_OBJS}

# Utility defaults
UTIL_DEFAULTS = ${BUILD}/default/vboot_reference

# Scripts to install directly (not compiled)
UTIL_SCRIPTS = \
	utility/dev_debug_vboot \
	utility/enable_dev_usb_boot

ifeq (${MINIMAL},)
UTIL_SCRIPTS += \
	utility/dev_make_keypair \
	utility/vbutil_what_keys
endif

# These utilities should be linked statically.
UTIL_NAMES_STATIC = \
	utility/crossystem

UTIL_NAMES = ${UTIL_NAMES_STATIC} \
	utility/tpm_init_temp_fix \
	utility/dumpRSAPublicKey \
	utility/tpmc

# TODO: Do we still need eficompress and efidecompress for anything?
ifeq (${MINIMAL},)
UTIL_NAMES += \
	utility/bmpblk_font \
	utility/bmpblk_utility \
	utility/eficompress \
	utility/efidecompress \
	utility/load_kernel_test \
	utility/pad_digest_utility \
	utility/signature_digest_utility \
	utility/verify_data
endif

UTIL_BINS_STATIC := $(addprefix ${BUILD}/,${UTIL_NAMES_STATIC})
UTIL_BINS = $(addprefix ${BUILD}/,${UTIL_NAMES})
ALL_OBJS += $(addsuffix .o,${UTIL_BINS} ${UTIL_BINS_STATIC})


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
# But we still need both static (tiny) and dynamic (with openssl) versions.
FUTIL_STATIC_BIN = ${FUTIL_BIN}_s

# These are the executables that are now built in to futility. We'll create
# symlinks for these so the old names will still work.
FUTIL_SYMLINKS = \
	dump_fmap \
	dump_kernel_config \
	gbb_utility \
	vbutil_firmware \
	vbutil_kernel \
	vbutil_key \
	vbutil_keyblock

FUTIL_STATIC_SRCS = \
	futility/futility.c \
	futility/cmd_dump_fmap.c \
	futility/cmd_gbb_utility.c \
	futility/misc.c

FUTIL_SRCS = \
	${FUTIL_STATIC_SRCS} \
	futility/cmd_create.c \
	futility/cmd_dump_kernel_config.c \
	futility/cmd_load_fmap.c \
	futility/cmd_pcr.c \
	futility/cmd_show.c \
	futility/cmd_sign.c \
	futility/cmd_vbutil_firmware.c \
	futility/cmd_vbutil_kernel.c \
	futility/cmd_vbutil_key.c \
	futility/cmd_vbutil_keyblock.c \
	futility/file_type.c \
	futility/traversal.c \
	futility/vb1_helper.c

# List of commands built in futility and futility_s.
FUTIL_STATIC_CMD_LIST = ${BUILD}/gen/futility_static_cmds.c
FUTIL_CMD_LIST = ${BUILD}/gen/futility_cmds.c

# Workaround for TODO(crbug.com/437107).
FUTIL_STATIC_WORKAROUND_SRCS = firmware/stub/vboot_api_stub_static_sf.c

FUTIL_STATIC_OBJS = ${FUTIL_STATIC_SRCS:%.c=${BUILD}/%.o} \
	${FUTIL_STATIC_WORKAROUND_SRCS:%.c=${BUILD}/%.o} \
	${FUTIL_STATIC_CMD_LIST:%.c=%.o}
FUTIL_OBJS = ${FUTIL_SRCS:%.c=${BUILD}/%.o} ${FUTIL_CMD_LIST:%.c=%.o}

${FUTIL_OBJS}: INCLUDES += -Ihost/lib21/include -Ifirmware/lib21/include
${FUTIL_BIN}: ${UTILLIB21}
${FUTIL_BIN}: LIBS += ${UTILLIB21}

ALL_OBJS += ${FUTIL_OBJS}


# Library of handy test functions.
TESTLIB = ${BUILD}/tests/test.a

TESTLIB_SRCS = \
	tests/test_common.c \
	tests/timer_utils.c \
	tests/crc32_test.c

TESTLIB_OBJS = ${TESTLIB_SRCS:%.c=${BUILD}/%.o}
TEST_OBJS += ${TESTLIB_OBJS}


# And some compiled tests.
TEST_NAMES = \
	tests/cgptlib_test \
	tests/rollback_index2_tests \
	tests/rollback_index3_tests \
	tests/rsa_padding_test \
	tests/rsa_utility_tests \
	tests/rsa_verify_benchmark \
	tests/sha_benchmark \
	tests/sha_tests \
	tests/stateful_util_tests \
	tests/tlcl_tests \
	tests/tpm_bootmode_tests \
	tests/utility_string_tests \
	tests/utility_tests \
	tests/vboot_api_init_tests \
	tests/vboot_api_devmode_tests \
	tests/vboot_api_firmware_tests \
	tests/vboot_api_kernel_tests \
	tests/vboot_api_kernel2_tests \
	tests/vboot_api_kernel3_tests \
	tests/vboot_api_kernel4_tests \
	tests/vboot_audio_tests \
	tests/vboot_common_tests \
	tests/vboot_common2_tests \
	tests/vboot_common3_tests \
	tests/vboot_display_tests \
	tests/vboot_firmware_tests \
	tests/vboot_kernel_tests \
	tests/vboot_nvstorage_test \
	tests/verify_kernel \
	tests/futility/binary_editor \
	tests/futility/test_not_really

ifdef REGION_READ
TEST_NAMES += tests/vboot_region_tests
endif

TEST2X_NAMES = \
	tests/vb2_api_tests \
	tests/vb2_common_tests \
	tests/vb2_misc_tests \
	tests/vb2_nvstorage_tests \
	tests/vb2_rsa_utility_tests \
	tests/vb2_secdata_tests \
	tests/vb2_sha_tests

TEST20_NAMES = \
	tests/vb20_api_tests \
	tests/vb20_common_tests \
	tests/vb20_common2_tests \
	tests/vb20_verify_fw.c \
	tests/vb20_common3_tests \
	tests/vb20_misc_tests \
	tests/vb20_rsa_padding_tests \
	tests/vb20_verify_fw

TEST21_NAMES = \
	tests/vb21_api_tests \
	tests/vb21_common_tests \
	tests/vb21_common2_tests \
	tests/vb21_misc_tests \
	tests/vb21_host_fw_preamble_tests \
	tests/vb21_host_key_tests \
	tests/vb21_host_keyblock_tests \
	tests/vb21_host_misc_tests \
	tests/vb21_host_sig_tests

TEST_NAMES += ${TEST2X_NAMES} ${TEST20_NAMES} ${TEST21_NAMES}

# And a few more...
TLCL_TEST_NAMES = \
	tests/tpm_lite/tpmtest_earlyextend \
	tests/tpm_lite/tpmtest_earlynvram \
        tests/tpm_lite/tpmtest_earlynvram2 \
	tests/tpm_lite/tpmtest_enable \
	tests/tpm_lite/tpmtest_fastenable \
	tests/tpm_lite/tpmtest_globallock \
        tests/tpm_lite/tpmtest_redefine_unowned \
        tests/tpm_lite/tpmtest_spaceperm \
	tests/tpm_lite/tpmtest_testsetup \
	tests/tpm_lite/tpmtest_timing \
        tests/tpm_lite/tpmtest_writelimit

TEST_NAMES += ${TLCL_TEST_NAMES}

# Finally
TEST_BINS = $(addprefix ${BUILD}/,${TEST_NAMES})
TEST_OBJS += $(addsuffix .o,${TEST_BINS})

TEST2X_BINS = $(addprefix ${BUILD}/,${TEST2X_NAMES})
TEST20_BINS = $(addprefix ${BUILD}/,${TEST20_NAMES})
TEST21_BINS = $(addprefix ${BUILD}/,${TEST21_NAMES})

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
all: fwlib fwlib2x fwlib20 fwlib21 \
	$(if ${FIRMWARE_ARCH},,host_stuff) \
	$(if ${COV},coverage)

# Host targets
.PHONY: host_stuff
host_stuff: utillib hostlib cgpt utils futil tests utillib21

.PHONY: clean
clean:
	${Q}/bin/rm -rf ${BUILD}

.PHONY: install
install: cgpt_install utils_install signing_install futil_install \
	pc_files_install

.PHONY: install_mtd
install_mtd: install cgpt_wrapper_install

.PHONY: install_for_test
install_for_test: override DESTDIR = ${TEST_INSTALL_DIR}
install_for_test: install

# Don't delete intermediate object files
.SECONDARY:

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
${FWLIB2X_OBJS}: CFLAGS += -DUNROLL_LOOPS
${FWLIB20_OBJS}: CFLAGS += -DUNROLL_LOOPS
${FWLIB21_OBJS}: CFLAGS += -DUNROLL_LOOPS

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

ifdef REGION_READ
${FWLIB_OBJS}: CFLAGS += -DREGION_READ
endif

ifeq (${FIRMWARE_ARCH},)
# Disable rollback TPM when compiling locally, since otherwise
# load_kernel_test attempts to talk to the TPM.
${FWLIB_OBJS}: CFLAGS += -DDISABLE_ROLLBACK_TPM
endif

${FWLIB20_OBJS}: INCLUDES += -Ifirmware/lib20/include
${FWLIB21_OBJS}: INCLUDES += -Ifirmware/lib21/include

# Linktest ensures firmware lib doesn't rely on outside libraries
${BUILD}/firmware/linktest/main_vbinit: ${VBINIT_OBJS}
${BUILD}/firmware/linktest/main_vbinit: OBJS = ${VBINIT_OBJS}
TEST_OBJS += ${BUILD}/firmware/linktest/main_vbinit.o
${BUILD}/firmware/linktest/main_vbsf: ${VBSF_OBJS}
${BUILD}/firmware/linktest/main_vbsf: OBJS = ${VBSF_OBJS}
TEST_OBJS += ${BUILD}/firmware/linktest/main_vbsf.o
${BUILD}/firmware/linktest/main: ${FWLIB}
${BUILD}/firmware/linktest/main: LIBS = ${FWLIB}
TEST_OBJS += ${BUILD}/firmware/linktest/main.o

.PHONY: fwlinktest
fwlinktest: \
	${BUILD}/firmware/linktest/main_vbinit \
	${BUILD}/firmware/linktest/main_vbsf \
	${BUILD}/firmware/linktest/main

.PHONY: fwlib
fwlib: $(if ${FIRMWARE_ARCH},${FWLIB},fwlinktest)

${FWLIB}: ${FWLIB_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

.PHONY: fwlib2x
fwlib2x: ${FWLIB2X}

${FWLIB2X}: ${FWLIB2X_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

.PHONY: fwlib20
fwlib20: ${FWLIB20}

${FWLIB20}: ${FWLIB2X_OBJS} ${FWLIB20_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

.PHONY: fwlib21
fwlib21: ${FWLIB21}

${FWLIB21}: ${FWLIB2X_OBJS} ${FWLIB21_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

# ----------------------------------------------------------------------------
# Host library(s)

# Link tests for local utilities
${BUILD}/host/linktest/main: ${UTILLIB}
${BUILD}/host/linktest/main: LIBS = ${UTILLIB}
TEST_OBJS += ${BUILD}/host/linktest/main.o

.PHONY: utillib
utillib: ${UTILLIB} \
	${BUILD}/host/linktest/main

# TODO: better way to make .a than duplicating this recipe each time?
${UTILLIB}: ${UTILLIB_OBJS} ${FWLIB_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

.PHONY: utillib21
utillib21: ${UTILLIB21}

${UTILLIB21}: INCLUDES += -Ihost/lib21/include -Ifirmware/lib21/include
${UTILLIB21}: ${UTILLIB21_OBJS} ${FWLIB2X_OBJS} ${FWLIB21_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^


# Link tests for external repos
${BUILD}/host/linktest/extern: ${HOSTLIB}
${BUILD}/host/linktest/extern: LIBS = ${HOSTLIB}
${BUILD}/host/linktest/extern: LDLIBS += -static
TEST_OBJS += ${BUILD}/host/linktest/extern.o

.PHONY: hostlib
hostlib: ${HOSTLIB} \
	${BUILD}/host/linktest/extern

# TODO: better way to make .a than duplicating this recipe each time?
${HOSTLIB}: ${HOSTLIB_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^


# Ugh. This is a very cut-down version of HOSTLIB just for the installer.
.PHONY: tinyhostlib
tinyhostlib: ${TINYHOSTLIB}
	${Q}cp -f ${TINYHOSTLIB} ${HOSTLIB}

${TINYHOSTLIB}: ${TINYHOSTLIB_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^

# ----------------------------------------------------------------------------
# CGPT library and utility

.PHONY: cgpt_wrapper
cgpt_wrapper: ${CGPT_WRAPPER}

${CGPT_WRAPPER}: ${CGPT_WRAPPER_OBJS} ${UTILLIB}
	@$(PRINTF) "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o ${CGPT_WRAPPER} ${CFLAGS} $^

.PHONY: cgpt
cgpt: ${CGPT} ${CGPT_WRAPPER}

${CGPT}: LDLIBS += -luuid

${CGPT}: ${CGPT_OBJS} ${UTILLIB}
	@${PRINTF} "    LDcgpt        $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o ${CGPT} ${CFLAGS} ${LDFLAGS} $^ ${LDLIBS}

.PHONY: cgpt_install
cgpt_install: ${CGPT}
	@${PRINTF} "    INSTALL       CGPT\n"
	${Q}mkdir -p ${UB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} $^

.PHONY: cgpt_wrapper_install
cgpt_wrapper_install: cgpt_install ${CGPT_WRAPPER}
	@$(PRINTF) "    INSTALL       cgpt_wrapper\n"
	${Q}${INSTALL} -t ${UB_DIR} ${CGPT_WRAPPER}
	${Q}mv ${UB_DIR}/$(notdir ${CGPT}) \
		${UB_DIR}/$(notdir ${CGPT}).bin
	${Q}mv ${UB_DIR}/$(notdir ${CGPT_WRAPPER}) \
		${UB_DIR}/$(notdir ${CGPT})

# ----------------------------------------------------------------------------
# Utilities

# These have their own headers too.
${BUILD}/utility/%: INCLUDES += -Iutility/include

${UTIL_BINS} ${UTIL_BINS_STATIC}: ${UTILLIB}
${UTIL_BINS} ${UTIL_BINS_STATIC}: LIBS = ${UTILLIB}

# Utilities for auto-update toolkits must be statically linked.
${UTIL_BINS_STATIC}: LDFLAGS += -static


.PHONY: utils
utils: ${UTIL_BINS} ${UTIL_SCRIPTS}
	${Q}cp -f ${UTIL_SCRIPTS} ${BUILD}/utility
	${Q}chmod a+rx $(patsubst %,${BUILD}/%,${UTIL_SCRIPTS})

.PHONY: utils_install
utils_install: ${UTIL_BINS} ${UTIL_SCRIPTS} ${UTIL_DEFAULTS}
	@${PRINTF} "    INSTALL       UTILS\n"
	${Q}mkdir -p ${UB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} ${UTIL_BINS} ${UTIL_SCRIPTS}
	${Q}mkdir -p ${DF_DIR}
	${Q}${INSTALL} -t ${DF_DIR} -m 'u=rw,go=r,a-s' ${UTIL_DEFAULTS}

# And some signing stuff for the target
.PHONY: signing_install
signing_install: ${SIGNING_SCRIPTS} ${SIGNING_SCRIPTS_DEV} ${SIGNING_COMMON}
	@${PRINTF} "    INSTALL       SIGNING\n"
	${Q}mkdir -p ${UB_DIR} ${VB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} ${SIGNING_SCRIPTS}
	${Q}${INSTALL} -t ${VB_DIR} ${SIGNING_SCRIPTS_DEV}
	${Q}${INSTALL} -t ${VB_DIR} -m 'u=rw,go=r,a-s' ${SIGNING_COMMON}

# ----------------------------------------------------------------------------
# new Firmware Utility

.PHONY: futil
futil: ${FUTIL_STATIC_BIN} ${FUTIL_BIN}

${FUTIL_STATIC_BIN}: ${FUTIL_STATIC_OBJS} ${UTILLIB}
	@${PRINTF} "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o $@ ${CFLAGS} ${LDFLAGS} -static $^ ${LDLIBS}

${FUTIL_BIN}: LDLIBS += ${CRYPTO_LIBS}
${FUTIL_BIN}: ${FUTIL_OBJS} ${UTILLIB}
	@${PRINTF} "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o $@ ${CFLAGS} ${LDFLAGS} $^ ${LDLIBS}

.PHONY: futil_install
futil_install: ${FUTIL_BIN} ${FUTIL_STATIC_BIN}
	@${PRINTF} "    INSTALL       futility\n"
	${Q}mkdir -p ${UB_DIR}
	${Q}${INSTALL} -t ${UB_DIR} ${FUTIL_BIN} ${FUTIL_STATIC_BIN}
	${Q}for prog in ${FUTIL_SYMLINKS}; do \
		ln -sf futility "${UB_DIR}/$$prog"; done

# ----------------------------------------------------------------------------
# Utility to generate TLCL structure definition header file.

${BUILD}/utility/tlcl_generator: CFLAGS += -fpack-struct

STRUCTURES_TMP=${BUILD}/tlcl_structures.tmp
STRUCTURES_SRC=firmware/lib/tpm_lite/include/tlcl_structures.h

.PHONY: update_tlcl_structures
update_tlcl_structures: ${BUILD}/utility/tlcl_generator
	@${PRINTF} "    Rebuilding TLCL structures\n"
	${Q}${BUILD}/utility/tlcl_generator > ${STRUCTURES_TMP}
	${Q}cmp -s ${STRUCTURES_TMP} ${STRUCTURES_SRC} || \
		( echo "%% Updating structures.h %%" && \
		  cp ${STRUCTURES_TMP} ${STRUCTURES_SRC} )

# ----------------------------------------------------------------------------
# Tests

.PHONY: tests
tests: ${TEST_BINS}

${TEST_BINS}: ${UTILLIB} ${TESTLIB}
${TEST_BINS}: INCLUDES += -Itests
${TEST_BINS}: LIBS = ${TESTLIB} ${UTILLIB}

${TEST2X_BINS}: ${FWLIB2X}
${TEST2X_BINS}: LIBS += ${FWLIB2X}

${TEST20_BINS}: ${FWLIB20}
${TEST20_BINS}: INCLUDES += -Ifirmware/lib20/include
${TEST20_BINS}: LIBS += ${FWLIB20}

${TEST21_BINS}: ${UTILLIB21}
${TEST21_BINS}: INCLUDES += -Ihost/lib21/include -Ifirmware/lib21/include
${TEST21_BINS}: LIBS += ${UTILLIB21}

${TESTLIB}: ${TESTLIB_OBJS}
	@${PRINTF} "    RM            $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	@${PRINTF} "    AR            $(subst ${BUILD}/,,$@)\n"
	${Q}ar qc $@ $^


# ----------------------------------------------------------------------------
# Generic build rules. LIBS and OBJS can be overridden to tweak the generic
# rules for specific targets.

${BUILD}/%: ${BUILD}/%.o ${OBJS} ${LIBS}
	@${PRINTF} "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o $@ ${CFLAGS} ${LDFLAGS} $< ${OBJS} ${LIBS} ${LDLIBS}

${BUILD}/%.o: %.c
	@${PRINTF} "    CC            $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

${BUILD}/%.o: ${BUILD}/%.c
	@${PRINTF} "    CC            $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# Rules to recompile a single source file for library and test
# TODO: is there a tidier way to do this?
${BUILD}/%_for_lib.o: CFLAGS += -DFOR_LIBRARY
${BUILD}/%_for_lib.o: %.c
	@${PRINTF} "    CC-for-lib    $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

${BUILD}/%_for_test.o: CFLAGS += -DFOR_TEST
${BUILD}/%_for_test.o: %.c
	@${PRINTF} "    CC-for-test   $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# TODO: C++ files don't belong in vboot reference at all.  Convert to C.
${BUILD}/%.o: %.cc
	@${PRINTF} "    CXX           $(subst ${BUILD}/,,$@)\n"
	${Q}${CXX} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# ----------------------------------------------------------------------------
# Here are the special tweaks to the generic rules.

# Always create the defaults file, since it depends on input variables
.PHONY: ${UTIL_DEFAULTS}
${UTIL_DEFAULTS}:
	@${PRINTF} "    CREATE        $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@
	${Q}mkdir -p $(dir $@)
	${Q}echo '# Generated file. Do not edit.' > $@.tmp
	${Q}echo "DEV_DEBUG_FORCE=${DEV_DEBUG_FORCE}" >> $@.tmp
	${Q}mv -f $@.tmp $@

# Some utilities need external crypto functions
CRYPTO_LIBS := $(shell ${PKG_CONFIG} --libs libcrypto)

${BUILD}/utility/dumpRSAPublicKey: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/pad_digest_utility: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/utility/signature_digest_utility: LDLIBS += ${CRYPTO_LIBS}

${BUILD}/host/linktest/main: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/vboot_common2_tests: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/vboot_common3_tests: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/vb20_common2_tests: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/vb20_common3_tests: LDLIBS += ${CRYPTO_LIBS}
${BUILD}/tests/verify_kernel: LDLIBS += ${CRYPTO_LIBS}

${TEST21_BINS}: LDLIBS += ${CRYPTO_LIBS}

LZMA_LIBS := $(shell ${PKG_CONFIG} --libs liblzma)
YAML_LIBS := $(shell ${PKG_CONFIG} --libs yaml-0.1)

${BUILD}/utility/bmpblk_utility: LD = ${CXX}
${BUILD}/utility/bmpblk_utility: LDLIBS = ${LZMA_LIBS} ${YAML_LIBS}

BMPBLK_UTILITY_DEPS = \
	${BUILD}/utility/bmpblk_util.o \
	${BUILD}/utility/image_types.o \
	${BUILD}/utility/eficompress_for_lib.o \
	${BUILD}/utility/efidecompress_for_lib.o

${BUILD}/utility/bmpblk_utility: OBJS = ${BMPBLK_UTILITY_DEPS}
${BUILD}/utility/bmpblk_utility: ${BMPBLK_UTILITY_DEPS}
ALL_OBJS += ${BMPBLK_UTILITY_DEPS}

${BUILD}/utility/bmpblk_font: OBJS += ${BUILD}/utility/image_types.o
${BUILD}/utility/bmpblk_font: ${BUILD}/utility/image_types.o
ALL_OBJS += ${BUILD}/utility/image_types.o

# Allow multiple definitions, so tests can mock functions from other libraries
${BUILD}/tests/%: CFLAGS += -Xlinker --allow-multiple-definition
${BUILD}/tests/%: LDLIBS += -lrt -luuid
${BUILD}/tests/%: LIBS += ${TESTLIB}

${BUILD}/tests/rollback_index2_tests: OBJS += \
	${BUILD}/firmware/lib/rollback_index_for_test.o
${BUILD}/tests/rollback_index2_tests: \
	${BUILD}/firmware/lib/rollback_index_for_test.o
TEST_OBJS += ${BUILD}/firmware/lib/rollback_index_for_test.o

${BUILD}/tests/tlcl_tests: OBJS += \
	${BUILD}/firmware/lib/tpm_lite/tlcl_for_test.o
${BUILD}/tests/tlcl_tests: \
	${BUILD}/firmware/lib/tpm_lite/tlcl_for_test.o
TEST_OBJS += ${BUILD}/firmware/lib/tpm_lite/tlcl_for_test.o

${BUILD}/tests/vboot_audio_tests: OBJS += \
	${BUILD}/firmware/lib/vboot_audio_for_test.o
${BUILD}/tests/vboot_audio_tests: \
	${BUILD}/firmware/lib/vboot_audio_for_test.o
TEST_OBJS += ${BUILD}/firmware/lib/vboot_audio_for_test.o

TLCL_TEST_BINS = $(addprefix ${BUILD}/,${TLCL_TEST_NAMES})
${TLCL_TEST_BINS}: OBJS += ${BUILD}/tests/tpm_lite/tlcl_tests.o
${TLCL_TEST_BINS}: ${BUILD}/tests/tpm_lite/tlcl_tests.o
TEST_OBJS += ${BUILD}/tests/tpm_lite/tlcl_tests.o

# ----------------------------------------------------------------------------
# Here are the special rules that don't fit in the generic rules.

# Generates the list of commands defined in futility by running grep in the
# source files looking for the DECLARE_FUTIL_COMMAND() macro usage.
${FUTIL_STATIC_CMD_LIST}: ${FUTIL_STATIC_SRCS}
${FUTIL_CMD_LIST}: ${FUTIL_SRCS}
${FUTIL_CMD_LIST} ${FUTIL_STATIC_CMD_LIST}:
	@${PRINTF} "    GEN           $(subst ${BUILD}/,,$@)\n"
	${Q}rm -f $@ $@_t $@_commands
	${Q}mkdir -p ${BUILD}/gen
	${Q}grep -hoRE '^DECLARE_FUTIL_COMMAND\([^,]+' $^ \
		| sed 's/DECLARE_FUTIL_COMMAND(\(.*\)/_CMD(\1)/' \
		| sort >>$@_commands
	${Q}./scripts/getversion.sh >> $@_t
	${Q}echo '#define _CMD(NAME) extern const struct' \
		'futil_cmd_t __cmd_##NAME;' >> $@_t
	${Q}cat $@_commands >> $@_t
	${Q}echo '#undef _CMD' >> $@_t
	${Q}echo '#define _CMD(NAME) &__cmd_##NAME,' >> $@_t
	${Q}echo 'const struct futil_cmd_t *const futil_cmds[] = {' >> $@_t
	${Q}cat $@_commands >> $@_t
	${Q}echo '0};  /* null-terminated */' >> $@_t
	${Q}echo '#undef _CMD' >> $@_t
	${Q}mv $@_t $@
	${Q}rm -f $@_commands

##############################################################################
# Targets that exist just to run tests

# Frequently-run tests
.PHONY: test_targets
test_targets:: runcgpttests runmisctests run2tests

ifeq (${MINIMAL},)
# Bitmap utility isn't compiled for minimal variant
test_targets:: runbmptests runfutiltests
# Scripts don't work under qemu testing
# TODO: convert scripts to makefile so they can be called directly
test_targets:: runtestscripts
endif

.PHONY: test_setup
test_setup:: cgpt utils futil tests install_for_test

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
	@${PRINTF} "    Copying qemu binary.\n"
	${Q}cp -fu /usr/bin/${QEMU_BIN} ${BUILD}/${QEMU_BIN}
	${Q}chmod a+rx ${BUILD}/${QEMU_BIN}
endif

.PHONY: runtests
runtests: test_setup test_targets

# Generate test keys
.PHONY: genkeys
genkeys: utils test_setup
	tests/gen_test_keys.sh

# Generate test cases for fuzzing
.PHONY: genfuzztestcases
genfuzztestcases: utils test_setup
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
	tests/load_kernel_tests.sh
	tests/run_cgpt_tests.sh ${BUILD_RUN}/cgpt/cgpt
	tests/run_cgpt_tests.sh ${BUILD_RUN}/cgpt/cgpt -D 358400
	tests/run_preamble_tests.sh
	tests/run_rsa_tests.sh
	tests/run_vbutil_kernel_arg_tests.sh
	tests/run_vbutil_tests.sh
	tests/vb2_rsa_tests.sh
	tests/vb2_firmware_tests.sh

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

.PHONY: run2tests
run2tests: test_setup
	${RUNTEST} ${BUILD_RUN}/tests/vb2_api_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb2_common_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb2_misc_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb2_nvstorage_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb2_rsa_utility_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb2_secdata_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb2_sha_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb20_api_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb20_common_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb20_common2_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vb20_common3_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vb20_misc_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb21_api_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb21_common_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb21_common2_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vb21_misc_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb21_host_fw_preamble_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vb21_host_key_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vb21_host_keyblock_tests ${TEST_KEYS}
	${RUNTEST} ${BUILD_RUN}/tests/vb21_host_misc_tests
	${RUNTEST} ${BUILD_RUN}/tests/vb21_host_sig_tests ${TEST_KEYS}

.PHONY: runfutiltests
runfutiltests: test_setup
	tests/futility/run_test_scripts.sh ${TEST_INSTALL_DIR}/bin
	${RUNTEST} ${BUILD_RUN}/tests/futility/test_not_really

# Run long tests, including all permutations of encryption keys (instead of
# just the ones we use) and tests of currently-unused code.
# Not run by automated build.
.PHONY: runlongtests
runlongtests: test_setup genkeys genfuzztestcases
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common2_tests ${TEST_KEYS} --all
	${RUNTEST} ${BUILD_RUN}/tests/vboot_common3_tests ${TEST_KEYS} --all
	${RUNTEST} ${BUILD_RUN}/tests/vb20_common2_tests ${TEST_KEYS} --all
	${RUNTEST} ${BUILD_RUN}/tests/vb20_common3_tests ${TEST_KEYS} --all
	${RUNTEST} ${BUILD_RUN}/tests/vb21_common2_tests ${TEST_KEYS} --all
	tests/run_preamble_tests.sh --all
	tests/run_vbutil_tests.sh --all

# TODO: There were a number of ancient tests that hadn't been run in years.
# They were removed with https://chromium-review.googlesource.com/#/c/214610/
# Some day it might be nice to see what they were supposed to do.

.PHONY: runalltests
runalltests: runtests runfutiltests runlongtests

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
# Generate addtional coverage stats just for firmware subdir, because the stats
# for the whole project don't include subdirectory summaries. This will print
# the summary for just the firmware sources.
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

# Include generated dependencies
ALL_DEPS += ${ALL_OBJS:%.o=%.o.d}
TEST_DEPS += ${TEST_OBJS:%.o=%.o.d}
-include ${ALL_DEPS}
-include ${TEST_DEPS}

# We want to use only relative paths in cscope.files, especially since the
# paths inside and outside the chroot are different.
SRCDIRPAT=$(subst /,\/,${SRCDIR}/)

# Note: vboot 2.0 is deprecated, so don't index those files
${BUILD}/cscope.files: test_setup
	${Q}rm -f $@
	${Q}cat ${ALL_DEPS} | tr -d ':\\' | tr ' ' '\012' | \
		grep -v /lib20/ | \
		sed -e "s/${SRCDIRPAT}//" | \
		egrep '\.[chS]$$' | sort | uniq > $@

cmd_etags = etags -o ${BUILD}/TAGS $(shell cat ${BUILD}/cscope.files)
cmd_ctags = ctags -o ${BUILD}/tags $(shell cat ${BUILD}/cscope.files)
run_if_prog = $(if $(shell which $(1) 2>/dev/null),$(2),)

.PHONY: tags TAGS xrefs
tags TAGS xrefs: ${BUILD}/cscope.files
	${Q}\rm -f ${BUILD}/tags ${BUILD}/TAGS
	${Q}$(call run_if_prog,etags,${cmd_etags})
	${Q}$(call run_if_prog,ctags,${cmd_ctags})

PC_FILES = ${PC_IN_FILES:%.pc.in=${BUILD}/%.pc}
${PC_FILES}: ${PC_IN_FILES}
	${Q}sed \
		-e 's:@LDLIBS@:${LDLIBS}:' \
		-e 's:@LIBDIR@:${LIBDIR}:' \
		$< > $@

.PHONY: pc_files_install
pc_files_install: ${PC_FILES}
	${Q}mkdir -p ${ULP_DIR}
	${Q}${INSTALL} -D -m 0644 $< ${ULP_DIR}/$(notdir $<)
