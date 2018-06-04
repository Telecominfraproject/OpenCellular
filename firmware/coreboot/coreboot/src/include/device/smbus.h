#ifndef DEVICE_SMBUS_H
#define DEVICE_SMBUS_H

#include <stdint.h>
#include <device/device.h>
#include <device/i2c_bus.h>
#include <device/smbus_def.h>

/* Common SMBus bus operations */
struct smbus_bus_operations {
	int    (*recv_byte)(device_t dev);
	int    (*send_byte)(device_t dev, u8 value);
	int    (*read_byte)(device_t dev, u8 addr);
	int   (*write_byte)(device_t dev, u8 addr, u8 value);
	int   (*block_read)(device_t dev, u8 cmd, u8 bytes, u8 *buffer);
	int  (*block_write)(device_t dev, u8 cmd, u8 bytes, const u8 *buffer);
};

static inline const struct smbus_bus_operations *ops_smbus_bus(struct bus *bus)
{
	const struct smbus_bus_operations *bops;

	bops = 0;
	if (bus && bus->dev && bus->dev->ops)
		bops = bus->dev->ops->ops_smbus_bus;

	return bops;
}

struct bus *get_pbus_smbus(device_t dev);
int smbus_set_link(device_t dev);

static inline int smbus_recv_byte(struct device *const dev)
{
	return i2c_dev_readb(dev);
}

static inline int smbus_send_byte(struct device *const dev, u8 byte)
{
	return i2c_dev_writeb(dev, byte);
}

static inline int smbus_read_byte(struct device *const dev, u8 addr)
{
	return i2c_dev_readb_at(dev, addr);
}

static inline int smbus_write_byte(struct device *const dev, u8 addr, u8 val)
{
	return i2c_dev_writeb_at(dev, addr, val);
}

int smbus_block_read(device_t dev, u8 cmd, u8 bytes, u8 *buffer);
int smbus_block_write(device_t dev, u8 cmd, u8 bytes, const u8 *buffer);

#if IS_ENABLED(CONFIG_SMBUS_HAS_AUX_CHANNELS)
void smbus_switch_to_channel(uint8_t channel_number);
uint8_t smbus_get_current_channel(void);
#endif

#endif /* DEVICE_SMBUS_H */
