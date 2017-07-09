hw-y += hw.ads
hw-y += hw-mmio_range.ads
hw-y += hw-mmio_regs.adb
hw-y += hw-mmio_regs.ads
hw-y += hw-pci.ads
hw-y += hw-pci-mmconf.adb
hw-y += hw-pci-mmconf.ads
hw-y += hw-port_io.adb
hw-y += hw-port_io.ads
hw-y += hw-sub_regs.ads
hw-y += hw-time.ads
hw-y += hw-time.adb
hw-y += hw-time-timer.ads

hw-$(CONFIG_HWBASE_POSIX_FILE) += hw-file.ads

hw-config-ads := $(subst //,/,$(call src-to-obj,,$(dir)/hw-config).ads)
$(hw-config-ads): $(dir)/hw-config.ads.template $(cnf)
	printf "    GENERATE   $(patsubst /%,%,$(subst $(obj)/,,$@))\n"
	sed \
	    -e's/<<DEFAULT_MMCONF_BASE>>/$(CONFIG_HWBASE_DEFAULT_MMCONF)/' \
	    -e's/<<DYNAMIC_MMIO>>/$(if $(filter y,$(CONFIG_HWBASE_DYNAMIC_MMIO)),True,False)/' \
	    $< >$@
hw-gen-y += $(hw-config-ads)
