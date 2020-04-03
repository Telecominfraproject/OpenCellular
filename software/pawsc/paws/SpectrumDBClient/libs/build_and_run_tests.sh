#!/bin/bash

#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#

# Helpful function to print a command and then execute it.
exe() { echo  "\$ $@" ; "$@" ; }

if [ "$1" == "" ]; then
    libs_to_test=" https-client gps json-parser state-machine string-buffer timers"
else
    libs_to_test=$1
fi

cflags="$cflags -D_GNU_SOURCE -g -I. -msoft-float -std=c99 -Wall"

make

for i in $libs_to_test; do
    echo
    echo "*************************************"
    echo $i
    echo "*************************************"
    echo

    rm -f $i/test_app*
    exe gcc $cflags $i/test.c -o $i/test_app -L. -l4gclient -lm -lssl -lcrypto
    exe $i/test_app

    echo
done
