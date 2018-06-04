#include <stdint.h>
#include <pc80/mc146818rtc.h>
#include <fallback.h>

#include "mc146818rtc_boot.c"

#if  CONFIG_MAX_REBOOT_CNT > 15
#error "CONFIG_MAX_REBOOT_CNT too high"
#endif

static inline __attribute__((unused)) int boot_count(uint8_t rtc_byte)
{
	return rtc_byte >> 4;
}

static inline __attribute__((unused)) uint8_t increment_boot_count(uint8_t rtc_byte)
{
	return rtc_byte + (1 << 4);
}

static inline __attribute__((unused)) uint8_t boot_set_fallback(uint8_t rtc_byte)
{
	return rtc_byte & ~RTC_BOOT_NORMAL;
}

static inline __attribute__((unused)) int boot_use_normal(uint8_t rtc_byte)
{
	return rtc_byte & RTC_BOOT_NORMAL;
}

static inline __attribute__((unused)) int do_normal_boot(void)
{
	unsigned char byte;

	if (cmos_error() || !cmos_chksum_valid()) {
		/* Invalid CMOS checksum detected!
		 * Force fallback boot...
		 */
		byte = cmos_read(RTC_BOOT_BYTE);
		byte &= boot_set_fallback(byte) & 0x0f;
		byte |= 0xf << 4;
		cmos_write(byte, RTC_BOOT_BYTE);
	}

	/* The RTC_BOOT_BYTE is now o.k. see where to go. */
	byte = cmos_read(RTC_BOOT_BYTE);

	/* Are we attempting to boot normally? */
	if (boot_use_normal(byte)) {
		/* Are we already at the max count? */
		if (boot_count(byte) < CONFIG_MAX_REBOOT_CNT)
			byte = increment_boot_count(byte);
		else
			byte = boot_set_fallback(byte);
	}

	/* Save the boot byte */
	cmos_write(byte, RTC_BOOT_BYTE);

	/* Return selected code path for this boot attempt */
	return boot_use_normal(byte);
}

unsigned read_option_lowlevel(unsigned start, unsigned size, unsigned def)
{
#if IS_ENABLED(CONFIG_USE_OPTION_TABLE)
	unsigned byte;

	byte = cmos_read(start/8);
	return (byte >> (start & 7U)) & ((1U << size) - 1U);
#else
	return def;
#endif
}
