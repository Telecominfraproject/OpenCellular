#!/bin/bash

#Get the tip-sdk path where the changes are there.

function check_output {
    if [ $? == 0 ]; then
       printf "$1: \e[1;32mPASS\e[0m\n"
    else
       printf "$2 \e[1;31mFAIL\e[0m\n"
       exit
    fi
}

NEW_LINUX_DIR="/home/oc/release_creation"
DRIVER_PATH="linux/kernel/linux/drivers"
UBOOT_PATH="bootloader/u-boot"

cd $NEW_LINUX_DIR
rm -rf test1 test2 tip-sdk
check_output "rm -rf test1 test2 tip-sdk"
mkdir test1
check_output "mkdir test1"
mkdir test2
check_output "mkdir test2"

DIR_1="/home/oc/release_creation/test1"
DIR_2="/home/oc/release_creation/test2"

NEW_LINUX_DIR_1="/home/oc/release_creation/test1/tip-sdk"
NEW_LINUX_DIR_2="/home/oc/release_creation/test2/tip-sdk"
OLD_LINUX_DIR=$1
TIPSDK_FILE=tip-sdk


cd $NEW_LINUX_DIR
check_output "cd $NEW_LINUX_DIR"
tar -zxf tip-sdk.tar.gz
check_output "tar -zxf tip-sdk.tar.gz"

#copy the new tip-sdk folder from the predefined path. 
#to the /tmp directory and untar the same.
cp -rf  $NEW_LINUX_DIR/$TIPSDK_FILE $DIR_1
check_output "cp $NEW_LINUX_DIR/$TIPSDK_FILE $DIR_1"
cp -rf  $NEW_LINUX_DIR/$TIPSDK_FILE $DIR_2
check_output "cp $NEW_LINUX_DIR/$TIPSDK_FILE $DIR_2"


#copy the files from the old folder to the new one 
#cp $OLD_LINUX_DIR/linux/kernel/linux/arch/mips/cavium-octeon/octeon-irq.c  $NEW_LINUX_DIR_1/linux/kernel/linux/arch/mips/cavium-octeon/octeon-irq.c
#check_output "cp $OLD_LINUX_DIR/linux/kernel/linux/arch/mips/cavium-octeon/octeon-irq.c  $NEW_LINUX_DIR_1/tip-sdk/linux/kernel/linux/arch/mips/cavium-octeon/octeon-irq.c"

#cp $OLD_LINUX_DIR/$DRIVER_PATH/misc/dvt.c          $NEW_LINUX_DIR_1/$DRIVER_PATH/misc/dvt.c
#check_output "cp $OLD_LINUX_DIR/$DRIVER_PATH/misc/dvt.c          $NEW_LINUX_DIR_1/$DRIVER_PATH/misc/dvt.c"

#cp $OLD_LINUX_DIR/$DRIVER_PATH/misc/Kconfig        $NEW_LINUX_DIR_1/$DRIVER_PATH/misc/Kconfig
#check_output "cp $OLD_LINUX_DIR/$DRIVER_PATH/misc/Kconfig        $NEW_LINUX_DIR_1/$DRIVER_PATH/misc/Kconfig"

#cp $OLD_LINUX_DIR/$DRIVER_PATH/misc/Makefile       $NEW_LINUX_DIR_1/$DRIVER_PATH/misc/Makefile
#check_output "cp $OLD_LINUX_DIR/$DRIVER_PATH/misc/Makefile       $NEW_LINUX_DIR_1/$DRIVER_PATH/misc/Makefile"

cp $OLD_LINUX_DIR/$DRIVER_PATH/hwmon/ina2xx.c      $NEW_LINUX_DIR_1/$DRIVER_PATH/hwmon/ina2xx.c
check_output "cp $OLD_LINUX_DIR/$DRIVER_PATH/hwmon/ina2xx.c      $NEW_LINUX_DIR_1/$DRIVER_PATH/hwmon/ina2xx.c"

cp $OLD_LINUX_DIR/$UBOOT_PATH/board/octeon/tip/tip.dts     $NEW_LINUX_DIR_1/$UBOOT_PATH/board/octeon/tip/tip.dts
check_output "cp $OLD_LINUX_DIR/$UBOOT_PATH/board/octeon/tip/tip.dts     $NEW_LINUX_DIR_1/$UBOOT_PATH/board/octeon/tip/tip.dts"

#cp $OLD_LINUX_DIR/$UBOOT_PATH/include/configs/octeon_tip.h $NEW_LINUX_DIR_1/$UBOOT_PATH/include/configs/octeon_tip.h
#check_output "cp $OLD_LINUX_DIR/$UBOOT_PATH/include/configs/octeon_tip.h $NEW_LINUX_DIR_1/$UBOOT_PATH/include/configs/octeon_tip.h"

cd /home/oc/release_creation

#creat the linux patch file 
diff -Naur test2/tip-sdk/linux/kernel/linux test1/tip-sdk/linux/kernel/linux > linux.patch
#check_output "diff -Naur test2/tip-sdk/linux/kernel/linux test1/tip-sdk/linux/kernel/linux > linux.patch"

#Creat the uboot patch file
diff -Naur test2/tip-sdk/bootloader/u-boot test1/tip-sdk/bootloader/u-boot > uboot.patch
#check_output "diff -Naur test2/tip-sdk/bootloader/u-boot test1/tip-sdk/bootloader/u-boot > uboot.patch"

#cleaning process 
rm -rf /home/oc/release_creation/tip-sdk
check_output "/home/oc/release_creation/tip-sdk"
rm -rf /home/oc/release_creation/test1
check_output "rm -rf /home/oc/release_creation/test1"
rm -rf /home/oc/release_creation/test2
check_output "rm -rf /home/oc/release_creation/test2"
