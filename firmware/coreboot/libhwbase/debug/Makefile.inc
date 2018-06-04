hw-y += hw-debug.ads
hw-y += hw-debug.adb
hw-$(CONFIG_HWBASE_DEBUG_NULL) += null/hw-debug_sink.ads
hw-$(CONFIG_HWBASE_DEBUG_TEXT_IO) += text_io/hw-debug_sink.ads
