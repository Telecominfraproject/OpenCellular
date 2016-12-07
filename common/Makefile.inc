gfxinit-y += hw-gfx-dp_aux_ch.adb
gfxinit-y += hw-gfx-dp_aux_ch.ads
gfxinit-y += hw-gfx-dp_defs.ads
gfxinit-y += hw-gfx-dp_info.adb
gfxinit-y += hw-gfx-dp_info.ads
gfxinit-y += hw-gfx-dp_training.adb
gfxinit-y += hw-gfx-dp_training.ads
gfxinit-y += hw-gfx-edid.adb
gfxinit-y += hw-gfx-edid.ads
gfxinit-y += hw-gfx-gma-config_helpers.adb
gfxinit-y += hw-gfx-gma-config_helpers.ads
gfxinit-y += hw-gfx-gma-connector_info.adb
gfxinit-y += hw-gfx-gma-connector_info.ads
gfxinit-y += hw-gfx-gma-connectors.ads
gfxinit-y += hw-gfx-gma-display_probing.adb
gfxinit-y += hw-gfx-gma-display_probing.ads
gfxinit-y += hw-gfx-gma-dp_aux_ch.ads
gfxinit-y += hw-gfx-gma-dp_aux_request.adb
gfxinit-y += hw-gfx-gma-dp_aux_request.ads
gfxinit-y += hw-gfx-gma-dp_info.ads
gfxinit-y += hw-gfx-gma-i2c.adb
gfxinit-y += hw-gfx-gma-i2c.ads
gfxinit-y += hw-gfx-gma-panel.adb
gfxinit-y += hw-gfx-gma-panel.ads
gfxinit-y += hw-gfx-gma-pch-fdi.adb
gfxinit-y += hw-gfx-gma-pch-fdi.ads
gfxinit-y += hw-gfx-gma-pch-sideband.adb
gfxinit-y += hw-gfx-gma-pch-sideband.ads
gfxinit-y += hw-gfx-gma-pch-transcoder.adb
gfxinit-y += hw-gfx-gma-pch-transcoder.ads
gfxinit-y += hw-gfx-gma-pch-vga.adb
gfxinit-y += hw-gfx-gma-pch-vga.ads
gfxinit-y += hw-gfx-gma-pch.ads
gfxinit-y += hw-gfx-gma-pipe_setup.adb
gfxinit-y += hw-gfx-gma-pipe_setup.ads
gfxinit-y += hw-gfx-gma-port_detect.ads
gfxinit-y += hw-gfx-gma-registers.adb
gfxinit-y += hw-gfx-gma-registers.ads
gfxinit-y += hw-gfx-gma-transcoder.adb
gfxinit-y += hw-gfx-gma-transcoder.ads
gfxinit-y += hw-gfx-gma.adb
gfxinit-y += hw-gfx-gma.ads
gfxinit-y += hw-gfx-i2c.ads
gfxinit-y += hw-gfx.ads
gfxinit-$(CONFIG_HWBASE_DYNAMIC_MMIO) += hw-gfx-framebuffer_filler.adb
gfxinit-$(CONFIG_HWBASE_DYNAMIC_MMIO) += hw-gfx-framebuffer_filler.ads

CONFIG_GFX_GMA_CPU		:= $(call strip_quotes,$(CONFIG_GFX_GMA_CPU))
CONFIG_GFX_GMA_CPU_VARIANT	:= $(call strip_quotes,$(CONFIG_GFX_GMA_CPU_VARIANT))
CONFIG_GFX_GMA_INTERNAL_PORT	:= $(call strip_quotes,$(CONFIG_GFX_GMA_INTERNAL_PORT))
CONFIG_GFX_GMA_ANALOG_I2C_PORT	:= $(call strip_quotes,$(CONFIG_GFX_GMA_ANALOG_I2C_PORT))

hw-gfx-gma-config-ads := $(subst //,/,$(call src-to-obj,,$(dir)/hw-gfx-gma-config).ads)

$(hw-gfx-gma-config-ads): $(dir)/hw-gfx-gma-config.ads.template $(cnf)
	printf "    GENERATE   $(patsubst /%,%,$(subst $(obj)/,,$@))\n"
	sed -e's/<<CPU>>/$(CONFIG_GFX_GMA_CPU)/' \
	    -e's/<<CPU_VARIANT>>/$(CONFIG_GFX_GMA_CPU_VARIANT)/' \
	    -e's/<<INTERNAL_PORT>>/$(CONFIG_GFX_GMA_INTERNAL_PORT)/' \
	    -e's/<<ANALOG_I2C_PORT>>/$(CONFIG_GFX_GMA_ANALOG_I2C_PORT)/' \
	    -e's/<<DEFAULT_MMIO_BASE>>/$(CONFIG_GFX_GMA_DEFAULT_MMIO)/' \
	    $< >$@
gfxinit-gen-y += $(hw-gfx-gma-config-ads)

ifneq ($(filter Ironlake Sandybridge Ivybridge,$(CONFIG_GFX_GMA_CPU)),)
subdirs-y += ironlake
else ifneq ($(filter Haswell Broadwell,$(CONFIG_GFX_GMA_CPU)),)
subdirs-y += haswell_shared haswell
else
subdirs-y += haswell_shared skylake
endif
