#ifndef _SYSMOBTS_EEPROM_H
#define _SYSMOBTS_EEPROM_H

#include <stdint.h>

struct sysmobts_net_cfg {
	uint8_t  mode;		/* 0 */
	uint32_t ip;		/* 1 - 4 */
	uint32_t mask;		/* 5 - 8 */
	uint32_t gw;		/* 9 - 12 */
	uint32_t dns;		/* 13 - 16 */
} __attribute__((packed));

struct sysmobts_eeprom {		/* offset */
	uint8_t eth_mac[6];		/* 0-5 */
	uint8_t _pad0[10];		/* 6-15 */
	uint16_t unused1;		/* 16-17 */
	uint8_t temp1_max;		/* 18 */
	uint8_t temp2_max;		/* 19 */
	uint32_t serial_nr;		/* 20-23 */
	uint32_t operational_hours;	/* 24-27 */
	uint32_t boot_count;		/* 28-31 */
	uint16_t model_nr;		/* 32-33 */
	uint16_t model_flags;		/* 34-35 */
	uint8_t trx_nr;			/* 36 */
	uint8_t boot_state[48];		/* 37-84 */
	uint8_t _pad1[18];              /* 85-102 */
	struct sysmobts_net_cfg net_cfg;/* 103-119 */
	uint8_t crc;			/* 120 */
	uint8_t gpg_key[128];		/* 121-249 */
} __attribute__((packed));

enum sysmobts_model_number {
	MODEL_SYSMOBTS_1002	= 1002,
	MODEL_SYSMOBTS_1020	= 1020,
	MODEL_SYSMOBTS_2050	= 2050,
};

enum sysmobts_net_mode {
	NET_MODE_DHCP,
	NET_MODE_STATIC,
};

#endif
