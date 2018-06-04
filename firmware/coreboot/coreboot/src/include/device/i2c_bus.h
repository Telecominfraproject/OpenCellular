/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DEVICE_I2C_BUS_H_
#define _DEVICE_I2C_BUS_H_

#include <stdlib.h>
#include <stdint.h>
#include <device/i2c.h>
#include <device/device.h>

/* I2C bus operation for ramstage drivers */
struct i2c_bus_operations {
	int (*transfer)(struct device *, const struct i2c_msg *, size_t count);
};

/*
 * Returns the first upstream facing link whose bus implements either
 * `i2c_bus_operations` *or* `smbus_bus_operations`.
 *
 * If not NULL, guarantees that `->dev`, `->dev->ops` and either
 * `->dev->ops->ops_i2c_bus` or `->dev->ops->ops_smbus_bus` are
 * not NULL.
 */
struct bus *i2c_link(struct device *);

/*
 * Shorthand for `i2c_link(dev)->dev`.
 *
 * Returns NULL if i2c_link(dev) returns NULL.
 */
static inline DEVTREE_CONST struct device *i2c_busdev(struct device *dev)
{
	struct bus *const link = i2c_link(dev);
	return link ? link->dev : NULL;
}

/*
 * Slave driver interface functions. These will look for the next
 * `i2c_bus_operations` *or* `smbus_bus_operations` and perform the
 * respective transfers.
 *
 * The interface is limited to what current slave drivers demand.
 * Extend as required.
 *
 * All functions return a negative `enum cb_err` value on error.
 * Either CB_ERR, CB_ERR_ARG or any CB_I2C_* (cf. include/types.h).
 */

/*
 * Reads one byte.
 * Compatible to smbus_recv_byte().
 *
 * Returns the read byte on success, negative `enum cb_err` value on error.
 */
int i2c_dev_readb(struct device *);

/*
 * Writes the byte `val`.
 * Compatible to smbus_send_byte().
 *
 * Returns 0 on success, negative `enum cb_err` value on error.
 */
int i2c_dev_writeb(struct device *, uint8_t val);

/*
 * Sends the register offset `off` and reads one byte.
 * Compatible to smbus_read_byte().
 *
 * Returns the read byte on success, negative `enum cb_err` value on error.
 */
int i2c_dev_readb_at(struct device *, uint8_t off);

/*
 * Sends the register offset `off` followed by the byte `val`.
 * Compatible to smbus_write_byte().
 *
 * Returns 0 on success, negative `enum cb_err` value on error.
 */
int i2c_dev_writeb_at(struct device *, uint8_t off, uint8_t val);

#endif	/* _DEVICE_I2C_BUS_H_ */
