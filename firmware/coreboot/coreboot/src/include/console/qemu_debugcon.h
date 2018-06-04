#ifndef _QEMU_DEBUGCON_H_
#define _QEMU_DEBUGCON_H_

#include <rules.h>
#include <stdint.h>

void qemu_debugcon_init(void);
void qemu_debugcon_tx_byte(unsigned char data);

#if IS_ENABLED(CONFIG_CONSOLE_QEMU_DEBUGCON) && (ENV_ROMSTAGE || ENV_RAMSTAGE)
static inline void __qemu_debugcon_init(void)	{ qemu_debugcon_init(); }
static inline void __qemu_debugcon_tx_byte(u8 data)
{
	qemu_debugcon_tx_byte(data);
}
#else
static inline void __qemu_debugcon_init(void)	{}
static inline void __qemu_debugcon_tx_byte(u8 data)	{}
#endif

#endif
