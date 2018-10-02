SOC_FAMILY = "ti33x"
require conf/machine/include/soc-family.inc

DEFAULTTUNE ?= "cortexa8hf-neon"
require conf/machine/include/tune-cortexa8.inc

PREFERRED_PROVIDER_virtual/xserver = "xserver-xorg"

# For built-in LCD, add xf86-input-tslib
XSERVER = "xserver-xorg \
           xf86-input-evdev \
           xf86-input-mouse \
           xf86-video-fbdev \
           xf86-input-keyboard"

# Default to external video, change to smallscreen for built-in LCD
GUI_MACHINE_CLASS = "bigscreen"

# Increase this everytime you change something in the kernel
MACHINE_KERNEL_PR = "r21"

KERNEL_IMAGETYPE = "zImage"

UBOOT_ARCH = "arm"
UBOOT_MACHINE = "am335x_evm_config"

UBOOT_ENTRYPOINT = "0x80008000"
UBOOT_LOADADDRESS = "0x80008000"

# Use the expected value of the ubifs filesystem's volume name in the kernel
# and u-boot.
UBI_VOLNAME = "rootfs"

# List common SoC features, may need to add touchscreen for specific machines
MACHINE_FEATURES = "kernel26 apm usbgadget usbhost vfat ext2 screen alsa ethernet"

