#!/bin/sh -e
#
# Copyright (C) 2008-2011 Intel, 2014 sysmocom
# install.sh [device_name] [rootfs_name] [video_mode] [vga_mode]
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin

# We want a 1 Gig partition on the cf
partition_size=1000

# Parse original arguments coming from the kernel cmdline.
# Then shift the first two arguments which contains
# partition information.
dev_name=$1
shift
image_name=$1
shift

kernel_cmdline_console=""
kernel_cmdline_video_mode=""
kernel_cmdline_vga_mode=""

while [ "$1" != "" ]; do

    echo "$1" | grep -q "console="
    success_console=$?
    if [ $success_console -eq 0 ]; then
        kernel_cmdline_console=$1
    fi
    shift

done

# Get a list of hard drives
hdnamelist=""
live_dev_name=${dev_name%%/*}

echo "Searching for hard drives ..."

for device in `ls /sys/block/`; do
    case $device in
	loop*)
            # skip loop device
	    ;;
	ram*)
            # skip ram device
	    ;;
	*)
	    # skip the device LiveOS is on
	    # Add valid hard drive name to the list
	    if [ $device != $live_dev_name -a -e /dev/$device ]; then
		hdnamelist="$hdnamelist $device"
	    fi
	    ;;
    esac
done

TARGET_DEVICE_NAME=""
for hdname in $hdnamelist; do
    # Display found hard drives and their basic info
    echo "-------------------------------"
    echo /dev/$hdname
    if [ -r /sys/block/$hdname/device/vendor ]; then
	echo -n "VENDOR="
	cat /sys/block/$hdname/device/vendor
    fi
    echo -n "MODEL="
    cat /sys/block/$hdname/device/model
    cat /sys/block/$hdname/device/uevent
    echo
    # Get user choice
    while true; do
	echo -n "Do you want to install this image there? [y/n] "
	read answer
	if [ "$answer" = "y" -o "$answer" = "n" ]; then
	    break
	fi
	echo "Please answer y or n"
    done
    if [ "$answer" = "y" ]; then
	TARGET_DEVICE_NAME=$hdname
	break
    fi
done

if [ -n "$TARGET_DEVICE_NAME" ]; then
    echo "Installing image on /dev/$TARGET_DEVICE_NAME ..."
else
    echo "No hard drive selected. Installation aborted."
    exit 1
fi

device=$TARGET_DEVICE_NAME

#
# The udev automounter can cause pain here, kill it
#
rm -f /etc/udev/rules.d/automount.rules
rm -f /etc/udev/scripts/mount*

#
# Unmount anything the automounter had mounted
#
umount /dev/${device}* 2> /dev/null || /bin/true

if [ ! -b /dev/loop0 ] ; then
    mknod /dev/loop0 b 7 0
fi

mkdir -p /tmp
cat /proc/mounts > /etc/mtab

# MMC devices are special in a couple of ways
# 1) they use a partition prefix character 'p'
# 2) they are detected asynchronously (need rootwait)
rootwait=""
part_prefix=""
if [ ! "${device#mmcblk}" = "${device}" ]; then
	part_prefix="p"
	rootwait="rootwait"
fi
rootfs=/dev/${device}${part_prefix}1

echo "*****************"
echo "Rootfs partition size: $rootfs_size MB ($rootfs)"
echo "*****************"
echo "Deleting partition table on /dev/${device} ..."
dd if=/dev/zero of=/dev/${device} bs=512 count=2

echo "Creating new partition table on /dev/${device} ..."
parted /dev/${device} mklabel msdos

echo "Creating rootfs partition on $rootfs"
parted /dev/${device} mkpart primary 0% $partition_size

parted /dev/${device} print

echo "Formatting $rootfs to ext4..."
mkfs.ext4 $rootfs

mkdir /tgt_root
mkdir -p /boot

echo "Copying rootfs files..."
dd if=/run/media/$dev_name/$image_name of=$rootfs bs=1024

# Handling of the target root partition
mount $rootfs /tgt_root

if [ -d /tgt_root/etc/ ] ; then
    # We dont want udev to mount our root device while we're booting...
    if [ -d /tgt_root/etc/udev/ ] ; then
	echo "/dev/${device}" >> /tgt_root/etc/udev/mount.blacklist
    fi
fi

# Handling of the target boot partition
mount --bind /tgt_root/boot /boot
echo "Preparing boot partition..."

GRUBCFG="/boot/grub/grub.cfg"
mkdir -p $(dirname $GRUBCFG)

baudrate=`echo $kernel_cmdline_console | cut -f 2 -d ',' | cut -f 1 -d 'n'`

cat > $GRUBCFG << EOF
default=0
timeout=1

serial --unit=0 --speed=$baudrate
terminal --timeout=2 serial

terminal_input --append  serial
terminal_output --append serial
EOF

BOOTPASSWD=
BOOTUSER=
if [ -n "${BOOTPASSWD}" ] && [ -n "${BOOTUSER}" ]; then
cat >> $GRUBCFG << EOF
set superusers="$BOOTUSER"
password $BOOTUSER $BOOTPASSWD
EOF
fi

if [ -f /etc/grub.d/00_header ] ; then
    echo "Preparing custom grub2 menu..."

    cat >>$GRUBCFG << EOF
menuentry "Linux" {
    set root=(hd0,1)
    linux /vmlinuz root=$rootfs $rootwait ro $kernel_cmdline_console $kernel_cmdline_video_mode $kernel_cmdline_vga_mode net.ifnames=0 panic=60 quiet
}
EOF
    if [ -n "${BOOTPASSWD}" ] && [ -n "${BOOTUSER}" ]; then
        sed -i "s/\(menuentry\)\(.*\)\({\)/\1\2--unrestricted \3/g" $GRUBCFG
    fi
    chmod 0444 $GRUBCFG
fi

ln -fs /boot/bzImage /tgt_root/vmlinuz
grub-install /dev/${device}
echo "(hd0) /dev/${device}" > /boot/grub/device.map

umount /boot
umount /tgt_root

echo "Resizing partition"
e2fsck -f $rootfs || true
resize2fs $rootfs
sync

echo "Re-mounting the root and trying to apply backup."
mount $rootfs /tgt_root

echo "Finding backupfiles"
backupfile=`find /run/media/$dev_name/ -name "sysmocom-backup*.tar" | head -n1`
if [ -n "$backupfile" ]; then
    if [ ! -r "$backupfile" ]; then
        echo "configurations-file $backupfile it is not readable"
    else
        echo "restoring $backupfile"
        backupfile_basename=`basename $backupfile`
        cp $backupfile /tgt_root
        chroot /tgt_root /usr/sbin/sysmocom-restore $backupfile_basename
        rm /tgt_root/$backupfile_basename
    fi
else
    echo "No configurations-file found"
fi

umount /tgt_root
sync

echo "Remove your installation media, and press ENTER"

read enter

echo "Rebooting..."
reboot -f
