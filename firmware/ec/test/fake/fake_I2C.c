/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "fake_I2C.h"
#include <ti/drivers/I2C.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "helpers/attribute.h"
#include "helpers/math.h"

#define NUM_I2C_BUS 8

/* I2C driver takes 7-bit address, so we only need 128 entries */
#define MAX_I2C_DEV 128

static struct I2C_Config dummy_bus[NUM_I2C_BUS];

typedef struct Fake_I2C_Dev {
    void *reg_table;
    size_t tbl_size;
    size_t reg_size;
    size_t addr_size;
    Fake_I2C_Endianness endianness;
} Fake_I2C_Dev;

typedef struct Fake_I2C_Bus {
    bool open;
    Fake_I2C_Dev devs[MAX_I2C_DEV]; /* Waste of memory, but I'm not worried */
} Fake_I2C_Bus;

/* TODO: if we can target XDC for Linux, we can simply provide this fake i2c
 * as an alternative i2c implementation
 */
void fake_I2C_init(void)
{
    memset(dummy_bus, 0x00, sizeof(dummy_bus));
}

void fake_I2C_deinit(void)
{
    for (int i = 0; i < NUM_I2C_BUS; ++i) {
        if (dummy_bus[i].object) {
            free(dummy_bus[i].object);
            dummy_bus[i].object = NULL;
        }
    }
}

void fake_I2C_registerDevSimple(unsigned int bus, uint8_t addr, void *reg_table,
                                size_t tbl_size, size_t reg_size,
                                size_t addr_size,
                                Fake_I2C_Endianness endianness)
{
    if (bus >= NUM_I2C_BUS) {
        return;
    }

    if (addr >= MAX_I2C_DEV) {
        return;
    }

    if (!dummy_bus[bus].object) {
        dummy_bus[bus].object = calloc(1, sizeof(Fake_I2C_Bus));
    }
    Fake_I2C_Bus *fake_bus = dummy_bus[bus].object;

    Fake_I2C_Dev *dev_tbl = fake_bus->devs;
    dev_tbl[addr] = (Fake_I2C_Dev){
        .reg_table = reg_table,
        .tbl_size = tbl_size,
        .reg_size = reg_size,
        .addr_size = addr_size,
        .endianness = endianness,
    };
}

void fake_I2C_unregisterDev(unsigned int bus, uint8_t addr)
{
    Fake_I2C_Bus *fake_bus = dummy_bus[bus].object;
    Fake_I2C_Dev *dev_tbl = fake_bus->devs;
    if (dev_tbl) {
        dev_tbl[addr] = (Fake_I2C_Dev){};
    }
}

/* ========================== Faked Functions =============================== */

void I2C_close(I2C_Handle handle)
{
    Fake_I2C_Bus *fake_bus = handle->object;
    fake_bus->open = false;
}

I2C_Handle I2C_open(unsigned int index, I2C_Params *params)
{
    UNUSED(params);

    if (index >= NUM_I2C_BUS) {
        return NULL;
    }

    Fake_I2C_Bus *fake_bus = dummy_bus[index].object;
    if (!fake_bus || fake_bus->open) {
        return NULL;
    }
    fake_bus->open = true;
    return &dummy_bus[index];
}

void I2C_Params_init(I2C_Params *params)
{
    UNUSED(params);
}

/* Inverts arbitrarily large chunk of memory */
static void reverse_bytes(const void *restrict data_in, void *restrict data_out,
                          size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        ((uint8_t *)data_out)[i] = ((uint8_t *)data_in)[size - i - 1];
    }
}

/*
 * Converts to/from different endiannesses
 * It's a pain, but I can't think of a better way since we need to account for
 * variable length variables at runtime
 */
static void endian_conversion(const void *restrict data_in, size_t in_size,
                              Fake_I2C_Endianness in_endianness,
                              void *restrict data_out, size_t out_size,
                              Fake_I2C_Endianness out_endianness)
{
    assert(in_size <= out_size);

    memset(data_out, 0x00, out_size);
    if (out_endianness != in_endianness) {
        reverse_bytes(data_in, data_out, in_size);
    } else {
        memcpy(data_out, data_in, in_size);
    }

    /* Account for out_size > in_size in big endian systems */
    if ((out_size > in_size) && (out_endianness == __ORDER_BIG_ENDIAN__)) {
        memmove((uint8_t *)data_out + (out_size - in_size), data_out, in_size);
        memset(data_out, 0x00, (out_size - in_size));
    }
}

bool I2C_transfer(I2C_Handle handle, I2C_Transaction *transaction)
{
    if (!handle) {
        return false; /* This is actually a crash in the proper driver */
    }

    Fake_I2C_Bus *fake_bus = handle->object;
    if (!fake_bus->open) {
        return false;
    }

    Fake_I2C_Dev *dev_tbl = fake_bus->devs;
    if (!dev_tbl) {
        return false;
    }

    /* Check if device is registered and 'present' on bus */
    if (transaction->slaveAddress >= MAX_I2C_DEV) {
        return false;
    }
    if (!dev_tbl[transaction->slaveAddress].reg_table) {
        return false;
    }
    const Fake_I2C_Dev *dev = &dev_tbl[transaction->slaveAddress];

    /* The write buffer must have at least the address in it */
    if (transaction->writeCount < dev->addr_size) {
        return false;
    }
    size_t write_count = transaction->writeCount - dev->addr_size;
    uint8_t *write_buf = transaction->writeBuf;
    uint8_t *read_buf = transaction->readBuf;

    /* Get the address we're reading/writing to */
    uint32_t reg_addr;
    endian_conversion(write_buf, dev->addr_size, dev->endianness, &reg_addr,
                      sizeof(reg_addr), __BYTE_ORDER__);
    write_buf += dev->addr_size;
    reg_addr *= dev->reg_size; /* Convert address to memory address in table */

    /* Bounds check */
    if (reg_addr >= dev->tbl_size) {
        return false;
    }

    /* True pointer to memory in table */
    uint8_t *mem_addr = (uint8_t *)dev->reg_table + reg_addr;

    /* If we have data to write, save it to the table - we have to write it
     * in chunks in the event that we have 16-bit (or larger) registers with
     * a different endianness than the host */
    if (write_count > 0) {
        size_t write_size = MIN(write_count, dev->tbl_size - reg_addr);
        for (size_t i = 0; i < write_size / dev->reg_size; i += dev->reg_size) {
            endian_conversion(write_buf + i, dev->reg_size, dev->endianness,
                              mem_addr + i, dev->reg_size, __BYTE_ORDER__);
        }
    }

    /* Read requested data into read buffer */
    /* TODO: what address do we read from if we also wrote data? */
    if (transaction->readCount > 0) {
        size_t read_size =
                MIN(transaction->readCount, dev->tbl_size - reg_addr);
        for (size_t i = 0; i < read_size / dev->reg_size; i += 2) {
            endian_conversion(mem_addr + i, dev->reg_size, __BYTE_ORDER__,
                              read_buf + i, dev->reg_size, dev->endianness);
        }
    }

    return true;
}
