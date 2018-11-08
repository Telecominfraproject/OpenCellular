#!/bin/sh -e
BUILDHOST=$(uname -a)
GCCVER=$(gcc --version)
GITHASH=$(git rev-parse HEAD)
if [ $# = 2 ]; then
  BUILDTYPE="\tEC Build"
  BUILDFNAME="build_ec_cfg.txt"
  if [ -e "$1" ]; then
    ARMVER=$($1 --version)
  else
    ARMVER="arm-none-eabi-gcc: Not found"
  fi
  if [ -d "$2" ]; then
    TIRTOSVER="TI RTOS: $2"
  else
    TIRTOSVER="TI RTOS: Not found"
  fi
else
  BUILDTYPE="\tHost Build"
  BUILDFNAME="build_host_cfg.txt"
  ARMVER=""
  TIRTOSVER=""
fi
{
echo "$BUILDTYPE"
echo
echo "Git Hash: $GITHASH"
echo
echo "Build Host: $BUILDHOST"
echo
echo "GCC: $GCCVER"
echo
echo "$ARMVER"
echo
echo "$TIRTOSVER"
} > $BUILDFNAME
