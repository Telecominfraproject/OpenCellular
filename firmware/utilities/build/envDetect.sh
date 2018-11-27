#!/bin/sh -e
BUILDHOST=$(uname -a)
GCCVER=$(gcc --version)
GITHASH=$(git rev-parse HEAD)
if [ $# = 2 ]; then
  BUILDTYPE="\tEC Build"
  BUILDFNAME="build_ec_cfg.txt"
  UNITYVER=""
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
elif [ $# = 1 ]; then
  BUILDTYPE="\tUnit Test Build"
  BUILDFNAME="build_ut_cfg.txt"
  ARMVER=""
  TIRTOSVER=""
  UNITYVER="Unity: $1"
else
  BUILDTYPE="\tHost Build"
  BUILDFNAME="build_host_cfg.txt"
  ARMVER=""
  TIRTOSVER=""
  UNITYVER=""
fi
{
echo "$BUILDTYPE"
echo
if [ ! "$UNITYVER" = "" ]; then
  echo "$UNITYVER"
  echo
fi
echo "Git Hash: $GITHASH"
echo
echo "Build Host: $BUILDHOST"
echo
if [ ! "$TIRTOSVER" = "" ]; then
  echo "$TIRTOSVER"
  echo
fi
echo "GCC: $GCCVER"
echo
if [ ! "$ARMVER" = "" ]; then
  echo "$ARMVER"
  echo
fi
} > $BUILDFNAME
