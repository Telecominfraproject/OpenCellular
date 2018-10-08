/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/devices/mdio_bb.h"

#include <stdbool.h> /* Required for sysctl (poorly written header) */
#include <stdint.h>

#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>

#define MDIO_READ 2
#define MDIO_WRITE 1

#define MDIO_SETUP_TIME 10
#define MDIO_HOLD_TIME 10

/* Minimum MDC period is 400 ns, plus some margin for error.  MDIO_DELAY
 * is done twice per period.
 */
#define MDIO_DELAY 40

/* The PHY may take up to 300 ns to produce data, plus some margin
 * for error.
 */
#define MDIO_READ_DELAY 35

static void ndelay(uint32_t delay_count)
{
    SysCtlDelay(delay_count);
}

/* MDIO must already be configured as output. */
static void mdiobb_send_bit(uint16_t val)
{
    if (val)
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
    else
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
    ndelay(MDIO_DELAY);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    ndelay(MDIO_DELAY);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
}

/* MDIO must already be configured as input. */
static int mdiobb_get_bit()
{
    ndelay(MDIO_DELAY);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    ndelay(MDIO_READ_DELAY);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);

    return (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7) ? 1 : 0);
}

/* MDIO must already be configured as output. */
static void mdiobb_send_num(uint16_t val, int bits)
{
    int i;

    for (i = bits - 1; i >= 0; i--)
        mdiobb_send_bit((val >> i) & 1);
}

static void mdiobb_set_mdio_dir(int dir)
{
    if (dir)
        GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);
    else
        GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_7);
}

/* MDIO must already be configured as input. */
static uint16_t mdiobb_get_num(int bits)
{
    int i;
    uint16_t ret = 0;

    for (i = bits - 1; i >= 0; i--) {
        ret <<= 1;
        ret |= mdiobb_get_bit();
    }

    return ret;
}

/*
 * mdiobb_cmd - Send the preamble, address, and register (common to read and
 * write).
 */
static void mdiobb_cmd(int op, uint8_t phy, uint8_t reg)
{
    int i;

    mdiobb_set_mdio_dir(1);

    /*
     * Send a 32 bit preamble ('1's) with an extra '1' bit for good
     * measure.  The IEEE spec says this is a PHY optional
     * requirement.
     */

    for (i = 0; i < 32; i++)
        mdiobb_send_bit(1);

    /* send the start bit (01) and the read opcode (10) or write (10).
     Clause 45 operation uses 00 for the start and 11, 10 for
     read/write */
    mdiobb_send_bit(0);
    mdiobb_send_bit(1);
    mdiobb_send_bit((op >> 1) & 1);
    mdiobb_send_bit((op >> 0) & 1);

    mdiobb_send_num(phy, 5);
    mdiobb_send_num(reg, 5);
}

static void mdiobb_turnaround(void)
{
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    ndelay(MDIO_DELAY);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
    ndelay(MDIO_DELAY);
}

int mdiobb_read(int phy, unsigned int reg)
{
    int ret, i;

    mdiobb_cmd(MDIO_READ, phy, reg);

    /* send the turnaround (10) */
    mdiobb_turnaround();
    mdiobb_set_mdio_dir(0);

    /* check the turnaround bit: the PHY should be driving it to zero, if this
     * PHY is listed in phy_ignore_ta_mask as having broken TA, skip that
     */
    if (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7) != 0) {
        /* PHY didn't drive TA low -- flush any bits it
         * may be trying to send.
         */
        for (i = 0; i < 32; i++)
            mdiobb_get_bit();

        return 0xffff;
    }

    ret = mdiobb_get_num(16);
    mdiobb_get_bit();
    return ret;
}

int mdiobb_write(int phy, int reg, uint16_t val)
{
    mdiobb_cmd(MDIO_WRITE, phy, reg);

    /* send the turnaround (10) */
    mdiobb_send_bit(1);
    mdiobb_send_bit(0);

    mdiobb_send_num(val, 16);

    mdiobb_set_mdio_dir(0);
    return 0;
}

void mdiobb_write_by_paging(int smi_device, int reg_addr, int data)
{
    int read_val = 0;
    int write_reg = 0;
    /*
     * First make sure Switch is idle for the operation by reading the bit 15
     * of global 2 register #24
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    mdiobb_write(0x7, 0x19, data);

    write_reg = (0x1 << 15) | (0x1 << 12) | (0x1 << 10) | (smi_device << 5) |
                reg_addr;
    mdiobb_write(0x7, 0x18, write_reg);

    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);
}

int mdiobb_read_by_paging(int smi_device, int reg_addr)
{
    int read_val = 0xf00f;
    int write_reg = 0;
    /*
     * First make sure Switch is idle for the operation by reading the bit 15
     * of global 2 register #24
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    write_reg =
            (1 << 15) | (1 << 12) | (0x2 << 10) | (smi_device << 5) | reg_addr;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Wait till transactions complete.
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    read_val = mdiobb_read(0x7, 0x19);
    return read_val;
}

/* Try to write directly using 0x18 and 0x19 registers */
void mdiobb_write_by_paging_c45(int smi_device, unsigned int reg_addr,
                                unsigned int data)
{
    unsigned int read_val = 0xf00f;
    unsigned int write_reg = 0;

    /*
     * First make sure Switch is idle for the operation by reading the bit 15
     * of global 2 register #24
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register 0x19 */
    mdiobb_write(0x7, 0x19, 0x0003);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 13;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register # 14 */
    mdiobb_write(0x7, 0x19, reg_addr);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 14;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register 0x19 */
    mdiobb_write(0x7, 0x19, 0x4003);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 13;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register 0x19 */
    mdiobb_write(0x7, 0x19, data);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 14;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);
}

/* try to read the C45 registers using 13 and 14 registers */
unsigned int mdiobb_read_by_paging_c45(int smi_device, unsigned int reg_addr)
{
    unsigned int read_val = 0xf00f;
    unsigned int write_reg = 0;

    /*
     * First make sure Switch is idle for the operation by reading the bit 15
     * of global 2 register #24
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register 0x19 */
    mdiobb_write(0x7, 0x19, 0x0003);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 13;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register # 14 */
    mdiobb_write(0x7, 0x19, reg_addr);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 14;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Fill the address in register 0x19 */
    mdiobb_write(0x7, 0x19, 0x4003);

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x1 << 10) | (smi_device << 5) | 13;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    /* Read register# 14 direcy*/

    /* The data sent for register #13 */
    write_reg = (1 << 15) | (1 << 12) | (0x2 << 10) | (smi_device << 5) | 14;
    mdiobb_write(0x7, 0x18, write_reg);

    /*
     * Register# 13 written?
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    read_val = mdiobb_read(0x7, 0x19);
    return read_val;
}

void mdiobb_set_bits(int smi_device, int reg_addr, int datamask)
{
    int read_val = 0;
    int write_reg = 0;

    /*
     * First take the contains of the register
     */
    read_val = mdiobb_read_by_paging(smi_device, reg_addr);
    datamask |= read_val;

    /*
     * First make sure Switch is idle for the operation by reading the bit 15
     * of global 2 register #24
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    mdiobb_write(0x7, 0x19, datamask);

    write_reg = (0x1 << 15) | (0x1 << 12) | (0x1 << 10) | (smi_device << 5) |
                reg_addr;
    mdiobb_write(0x7, 0x18, write_reg);

    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);
}

void mdiobb_clear_bits(int smi_device, int reg_addr, int datamask)
{
    int read_val = 0;
    int write_reg = 0;

    /*
     * First take the contains of the register
     */
    read_val = mdiobb_read_by_paging(smi_device, reg_addr);
    datamask = (~datamask) & read_val;
    /*
     * First make sure Switch is idle for the operation by reading the bit 15
     * of global 2 register #24
     */
    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);

    mdiobb_write(0x7, 0x19, datamask);

    write_reg = (0x1 << 15) | (0x1 << 12) | (0x1 << 10) | (smi_device << 5) |
                reg_addr;
    mdiobb_write(0x7, 0x18, write_reg);

    do {
        read_val = mdiobb_read(0x7, 0x18);
        if (!(read_val & (1 << 15)))
            break;
    } while (1);
}

void mdiobb_set_bits_C45(int smi_device, unsigned int reg_addr,
                         unsigned int datamask)
{
    int read_val = 0;

    /*
     * First take the contains of the register and set the bits.
     */
    read_val = mdiobb_read_by_paging_c45(smi_device, reg_addr);
    datamask |= read_val;

    /* Write back into the register */
    mdiobb_write_by_paging_c45(smi_device, reg_addr, datamask);
}

void mdiobb_clear_bits_C45(int smi_device, unsigned int reg_addr,
                           unsigned int datamask)
{
    int read_val = 0;

    /*
     * First take the contains of the register clear the bits
     */
    read_val = mdiobb_read_by_paging_c45(smi_device, reg_addr);
    datamask = (~datamask) & read_val;

    /* Write back into the register */
    mdiobb_write_by_paging_c45(smi_device, reg_addr, datamask);
}

void prepare_mdio(void)
{
    int delay = 0;
    /*
     *  Enable the GPIO pins for the MDC/MDIO (PN6 & PN7).
     */
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);

    /*
     * Enable the Timer 0 & 1.
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Enable processor interrupts.
    IntMasterEnable();

    /*
     * Configure timer
     */
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);
    delay = 0x700000;
    while (delay--)
        ;
}
