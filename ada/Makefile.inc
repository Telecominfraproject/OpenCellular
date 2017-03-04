hw-$(CONFIG_HWBASE_STATIC_MMIO) += static_mmio/hw-mmio_range.adb
hw-$(CONFIG_HWBASE_DYNAMIC_MMIO) += dynamic_mmio/hw-mmio_range.adb
hw-$(CONFIG_HWBASE_TIMER_CLOCK_GETTIME) += clock_gettime/hw-time-timer.adb

hw-$(CONFIG_HWBASE_TIMER_MUTIME) += $(muen-common-path)/musinfo/musinfo.ads
hw-$(CONFIG_HWBASE_TIMER_MUTIME) += $(muen-common-path)/muschedinfo/muschedinfo.ads
hw-$(CONFIG_HWBASE_TIMER_MUTIME) += mutime/hw-time-timer.adb

hw-$(CONFIG_HWBASE_POSIX_FILE) += posix/hw-file.adb
