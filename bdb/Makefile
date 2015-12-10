# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This Makefile normally builds in a 'build' subdir, but use
#
#    make BUILD=<dir>
#
# to put the output somewhere else.

##############################################################################
# Configuration variables come first.

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

CC ?= gcc
LD = ${CC}
PKG_CONFIG ?= pkg-config

SRCDIR := $(shell pwd)
export SRCDIR
BUILD = ${SRCDIR}/build
export BUILD
KEYDIR = ${SRCDIR}/testkeys

CFLAGS = -Wall -Werror

# Create / use dependency files
CFLAGS += -MMD -MF $@.d

##############################################################################
# Create output directories if necessary.  Do this via explicit shell commands
# so it happens before trying to generate/include dependencies.
_dir_create := $(shell [ -d ${BUILD} ] || mkdir -p ${BUILD}))
_keydir_create := $(shell [ -d ${KEYDIR} ] || mkdir -p ${KEYDIR}))

INC_PATH := $(shell ${PKG_CONFIG} --cflags libcrypto)
CFLAGS += ${INC_PATH}

CRYPTO_LIBS := $(shell ${PKG_CONFIG} --libs libcrypto)
LDLIBS += ${CRYPTO_LIBS}

##############################################################################
# Sources

LIBSRC = bdb.c host.c sha.c rsa.c
LIBOBJ = ${LIBSRC:%.c=${BUILD}/%.o}

BDBTESTSRC = bdb_test.c
BDBTESTOBJ = ${BDBTESTSRC:%.c=${BUILD}/%.o}
BDBTEST = ${BUILD}/bdb_test

BDBCREATESRC = bdb_create.c
BDBCREATEOBJ = ${BDBCREATESRC:%.c=${BUILD}/%.o}
BDBCREATE = ${BUILD}/bdb_create

DUMPRSASRC = dump_rsa.c
DUMPRSAOBJ = ${DUMPRSASRC:%.c=${BUILD}/%.o}
DUMPRSA = ${BUILD}/dump_rsa

ALL_OBJS = ${LIBOBJ} ${BDBTESTOBJ} ${BDBCREATEOBJ}
ALL_EXES = ${BDBTEST} ${BDBCREATE} ${DUMPRSA}

##############################################################################
# Targets

.PHONY: all
all: ${ALL_EXES}

.PHONY: clean
clean:
	${Q}/bin/rm -rf ${BUILD}

.PHONY: bdb
bdb: ${BDBCREATE}
	${Q}${BDBCREATE}

.PHONY: runtests
runtests: ${BDBTEST}
	${Q}${BDBTEST}

.PHONY: testkeys
testkeys: ${DUMPRSA}
	${Q}openssl genrsa -F4 -out ${KEYDIR}/bdbkey.pem 4096
	${Q}openssl req -batch -new -x509 -key ${KEYDIR}/bdbkey.pem \
		-out ${KEYDIR}/bdbkey.crt
	${Q}${DUMPRSA} -cert ${KEYDIR}/bdbkey.crt > ${KEYDIR}/bdbkey.keyb

	${Q}openssl genrsa -3 -out ${KEYDIR}/subkey.pem 3072
	${Q}openssl req -batch -new -x509 -key ${KEYDIR}/subkey.pem \
		-out ${KEYDIR}/subkey.crt
	${Q}${DUMPRSA} -cert ${KEYDIR}/subkey.crt > ${KEYDIR}/subkey.keyb

${BDBTEST}: ${BDBTESTOBJ} ${LIBOBJ}
	@$(PRINTF) "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o ${BDBTEST} ${CFLAGS} $^ ${LIBS} ${LDLIBS}

${BDBCREATE}: ${BDBCREATEOBJ} ${LIBOBJ}
	@$(PRINTF) "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o ${BDBCREATE} ${CFLAGS} $^ ${LIBS} ${LDLIBS}

${DUMPRSA}: ${DUMPRSAOBJ} ${LIBOBJ}
	@$(PRINTF) "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o ${DUMPRSA} ${CFLAGS} $^ ${LIBS} ${LDLIBS}

##############################################################################
# Generic build rules. LIBS and OBJS can be overridden to tweak the generic
# rules for specific targets.

${BUILD}/%: ${BUILD}/%.o ${OBJS} ${LIBS}
	@${PRINTF} "    LD            $(subst ${BUILD}/,,$@)\n"
	${Q}${LD} -o $@ ${CFLAGS} ${LDFLAGS} $< ${OBJS} ${LIBS} ${LDLIBS}

${BUILD}/%.o: %.c
	@${PRINTF} "    CC            $(subst ${BUILD}/,,$@)\n"
	${Q}${CC} ${CFLAGS} ${INCLUDES} -c -o $@ $<

# Include generated dependencies
ALL_DEPS += ${ALL_OBJS:%.o=%.o.d}
-include ${ALL_DEPS}
