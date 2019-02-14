#!/bin/sh

ARG1=$1
ARG2=$2
ARG3=$2
ARG4=$2

if [ "$ARG1" = "help" ]; then
    echo "./occmd.sh <module_name> <get/set> <param/all> <value>"
fi

if [ "$ARG1" = "bb" ]; then
    echo "In BB module"
    if [ "$ARG2" = "get" ]; then
        echo "In get module"
        exit
    else 
      if [ "$ARG2" = "set" ]; then
        echo "In set module"
        exit
      fi
    echo "./occmd.sh <module_name> <get/set> <param/all> <value>"
    fi
fi

echo "./occmd.sh <module_name> <get/set> <param/all> <value>"
