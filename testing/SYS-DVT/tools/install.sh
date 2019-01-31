#!/bin/bash

LSM_RD_LINUX_DIR=$1
LSM_FILE=lsm_rd.gz
WORKING_DIR=/tmp/_dvt_rootfs
DVTHOME=`pwd`

function check_output {
    if [ $? == 0 ]; then
       printf "$1: \e[1;32mPASS\e[0m\n"
    else
       printf "$2 \e[1;31mFAIL\e[0m\n"
       exit
    fi
}

# sanity check

if ! [ $(id -u) = 0 ]; then
   printf "Please use sudo\n"
   exit 1
fi

if ! [ -d $LSM_RD_LINUX_DIR ]; then
    printf "$LSM_RD_LINUX_DIR\n" failed
    exit
fi

# ok. do the job
printf "\nStaring DVT package installation ...\n\n"

rm -rf $WORKING_DIR
check_output "rm -rf $WORKING_DIR"

mkdir -p $WORKING_DIR
check_output "mkdir -p $WORKING_DIR"

cp $LSM_RD_LINUX_DIR/$LSM_FILE $WORKING_DIR 
check_output "cp $LSM_RD_LINUX_DIR/$LSM_FILE $WORKING_DIR"

cd $WORKING_DIR
check_output "cd $WORKING_DIR"

gunzip $LSM_FILE
check_output "gunzip $LSM_FILE"

#Extract the rootfs
cpio -id < lsm_rd
check_output "cpio -idv < lsm_rd"

#install the dvt packages
cp $DVTHOME/pkgs/python2.7_mips.tar.gz $WORKING_DIR/usr/bin
check_output "$DVTHOME/pkgs/python2.7_mips.tar.gz $WORKING_DIR/usr/bin"

cd $WORKING_DIR/usr/bin
check_output "cd $WORKING_DIR/usr/bin"

printf "Installing python ...\n"
tar -xzf python2.7_mips.tar.gz 
check_output "tar -xzvf python2.7_mips.tar.gz"

ln -s python2.7 python
check_output "ln -s python2.7 python"

cp $DVTHOME/pkgs/libz.so.1.2.11 $WORKING_DIR/usr/lib64
check_output "cp $DVTHOME/pkgs/libz.so.1.2.11 $WORKING_DIR/usr/lib64"

cd $WORKING_DIR/usr/lib64 
check_output "cd $WORKING_DIR/usr/lib64"

rm libz.so;rm libz.so.1;rm libz.so.1.2.3
check_output "rm libz.so;rm libz.so.1;rm libz.so.1.2.3"

ln -s libz.so.1.2.11 libz.so
check_output "ln -s libz.so.1.2.11 libz.so"

ln -s libz.so.1.2.11 libz.so.1
check_output "ln -s libz.so.1.2.11 libz.so.1"

mknod $WORKING_DIR/dev/dvt c 112 1

#cleanup
rm $WORKING_DIR/usr/bin/python2.7_mips.tar.gz
check_output "$WORKING_DIR/usr/bin/python2.7_mips.tar.gz"

rm $WORKING_DIR/lsm_rd
check_output "$WORKING_DIR/lsm_rd"

# Rootfs has all the dvt relevant files; now archive it.
cd $WORKING_DIR
check_output "cd $WORKING_DIR"

find . | cpio --format=newc -oF /tmp/rootfs.cpio
check_output "find . | cpio --format=newc -oF /tmp/rootfs.cpio"

chmod a+r /tmp/rootfs.cpio
check_output "chmod a+r /tmp/rootfs.cpio"

printf "Archiving rootfs ... pl wait ...\n"
gzip -f -9 /tmp/rootfs.cpio > /tmp/rootfs.cpio.gz
check_output "gzip -9 /tmp/rootfs.cpio > /tmp/rootfs.cpio.gz"

mv -f /tmp/rootfs.cpio.gz $LSM_RD_LINUX_DIR/lsm_rd_dvt.gz
check_output "mv -f /tmp/rootfs.cpio.gz $LSM_RD_LINUX_DIR/lsm_rd_dvt.gz"

printf "lsm_rd.gz is dvt ready and available $LSM_RD_LINUX_DIR/lsm_rd_dvt.gz\n"
