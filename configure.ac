#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cbootimage], [1.5], [pchiu@nvidia.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/cbootimage.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

# Checks for libraries.
AC_CHECK_LIB(m, log2)

# Checks for header files.
AC_CHECK_HEADERS([stdio.h string.h ctype.h sys/stat.h getopt.h stdlib.h assert.h errno.h sys/types.h math.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
