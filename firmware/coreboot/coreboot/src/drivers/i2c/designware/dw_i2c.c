/*
 * This file is part of the coreboot project.
 *
 * Copyright 2009 Vipin Kumar, ST Microelectronics
 * Copyright 2017 Google Inc.
 * Copyright 2017 Intel Corporation.
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

#include <arch/acpigen.h>
#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/i2c_bus.h>
#include <device/i2c_simple.h>
#include <string.h>
#include <timer.h>
#include "dw_i2c.h"

/* Use a ~10ms timeout for various operations */
#define DW_I2C_TIMEOUT_US		10000

/* High and low times in different speed modes (in ns) */
enum {
	/* SDA Hold Time */
	DEFAULT_SDA_HOLD_TIME		= 300,
	/* Standard Speed */
	MIN_SS_SCL_HIGHTIME		= 4000,
	MIN_SS_SCL_LOWTIME		= 4700,
	/* Fast Speed */
	MIN_FS_SCL_HIGHTIME		= 600,
	MIN_FS_SCL_LOWTIME		= 1300,
	/* Fast Plus Speed */
	MIN_FP_SCL_HIGHTIME		= 260,
	MIN_FP_SCL_LOWTIME		= 500,
	/* High Speed */
	MIN_HS_SCL_HIGHTIME		= 60,
	MIN_HS_SCL_LOWTIME		= 160,
};

/* Frequency represented as ticks per ns. Can also be used to calculate
 * the number of ticks to meet a time target or the period. */
struct freq {
	uint32_t ticks;
	uint32_t ns;
};

/* Control register definitions */
enum {
	CONTROL_MASTER_MODE		= (1 << 0),
	CONTROL_SPEED_SS		= (1 << 1),
	CONTROL_SPEED_FS		= (1 << 2),
	CONTROL_SPEED_HS		= (3 << 1),
	CONTROL_SPEED_MASK		= (3 << 1),
	CONTROL_10BIT_SLAVE		= (1 << 3),
	CONTROL_10BIT_MASTER		= (1 << 4),
	CONTROL_RESTART_ENABLE		= (1 << 5),
	CONTROL_SLAVE_DISABLE		= (1 << 6),
};

/* Command/Data register definitions */
enum {
	CMD_DATA_CMD			= (1 << 8),
	CMD_DATA_STOP			= (1 << 9),
};

/* Status register definitions */
enum {
	STATUS_ACTIVITY			= (1 << 0),
	STATUS_TX_FIFO_NOT_FULL		= (1 << 1),
	STATUS_TX_FIFO_EMPTY		= (1 << 2),
	STATUS_RX_FIFO_NOT_EMPTY	= (1 << 3),
	STATUS_RX_FIFO_FULL		= (1 << 4),
	STATUS_MASTER_ACTIVITY		= (1 << 5),
	STATUS_SLAVE_ACTIVITY		= (1 << 6),
};

/* Enable register definitions */
enum {
	ENABLE_CONTROLLER		= (1 << 0),
};

/* Interrupt status register definitions */
enum {
	INTR_STAT_RX_UNDER		= (1 << 0),
	INTR_STAT_RX_OVER		= (1 << 1),
	INTR_STAT_RX_FULL		= (1 << 2),
	INTR_STAT_TX_OVER		= (1 << 3),
	INTR_STAT_TX_EMPTY		= (1 << 4),
	INTR_STAT_RD_REQ		= (1 << 5),
	INTR_STAT_TX_ABORT		= (1 << 6),
	INTR_STAT_RX_DONE		= (1 << 7),
	INTR_STAT_ACTIVITY		= (1 << 8),
	INTR_STAT_STOP_DET		= (1 << 9),
	INTR_STAT_START_DET		= (1 << 10),
	INTR_STAT_GEN_CALL		= (1 << 11),
};

/* I2C Controller MMIO register space */
struct dw_i2c_regs {
	uint32_t control;
	uint32_t target_addr;
	uint32_t slave_addr;
	uint32_t master_addr;
	uint32_t cmd_data;
	uint32_t ss_scl_hcnt;
	uint32_t ss_scl_lcnt;
	uint32_t fs_scl_hcnt;
	uint32_t fs_scl_lcnt;
	uint32_t hs_scl_hcnt;
	uint32_t hs_scl_lcnt;
	uint32_t intr_stat;
	uint32_t intr_mask;
	uint32_t raw_intr_stat;
	uint32_t rx_thresh;
	uint32_t tx_thresh;
	uint32_t clear_intr;
	uint32_t clear_rx_under_intr;
	uint32_t clear_rx_over_intr;
	uint32_t clear_tx_over_intr;
	uint32_t clear_rd_req_intr;
	uint32_t clear_tx_abrt_intr;
	uint32_t clear_rx_done_intr;
	uint32_t clear_activity_intr;
	uint32_t clear_stop_det_intr;
	uint32_t clear_start_det_intr;
	uint32_t clear_gen_call_intr;
	uint32_t enable;
	uint32_t status;
	uint32_t tx_level;
	uint32_t rx_level;
	uint32_t sda_hold;
	uint32_t tx_abort_source;
	uint32_t slv_data_nak_only;
	uint32_t dma_cr;
	uint32_t dma_tdlr;
	uint32_t dma_rdlr;
	uint32_t sda_setup;
	uint32_t ack_general_call;
	uint32_t enable_status;
	uint32_t fs_spklen;
	uint32_t hs_spklen;
	uint32_t clr_restart_det;
	uint32_t comp_param1;
	uint32_t comp_version;
	uint32_t comp_type;
} __packed;

static const struct i2c_descriptor {
	enum i2c_speed speed;
	struct freq freq;
	int min_thigh_ns;
	int min_tlow_ns;
} speed_descriptors[] = {
	{
		.speed = I2C_SPEED_STANDARD,
		.freq = {
			.ticks = 100,
			.ns = 1000*1000,
		},
		.min_thigh_ns = MIN_SS_SCL_HIGHTIME,
		.min_tlow_ns = MIN_SS_SCL_LOWTIME,
	},
	{
		.speed = I2C_SPEED_FAST,
		.freq = {
			.ticks = 400,
			.ns = 1000*1000,
		},
		.min_thigh_ns = MIN_FS_SCL_HIGHTIME,
		.min_tlow_ns = MIN_FS_SCL_LOWTIME,
	},
	{
		.speed = I2C_SPEED_FAST_PLUS,
		.freq = {
			.ticks = 1,
			.ns = 1000,
		},
		.min_thigh_ns = MIN_FP_SCL_HIGHTIME,
		.min_tlow_ns = MIN_FP_SCL_LOWTIME,
	},
	{
		/* 100pF max capacitance */
		.speed = I2C_SPEED_HIGH,
		.freq = {
			.ticks = 3400,
			.ns = 1000*1000,
		},
		.min_thigh_ns = MIN_HS_SCL_HIGHTIME,
		.min_tlow_ns = MIN_HS_SCL_LOWTIME,
	},
};

static const struct soc_clock {
	int clk_speed_mhz;
	struct freq freq;
} soc_clocks[] = {
	{
		.clk_speed_mhz = 120,
		.freq = {
			.ticks = 120,
			.ns = 1000,
		},
	},
	{
		.clk_speed_mhz = 133,
		.freq = {
			.ticks = 400,
			.ns = 3000,
		},
	},
};

static const struct i2c_descriptor *get_bus_descriptor(enum i2c_speed speed)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(speed_descriptors); i++)
		if (speed == speed_descriptors[i].speed)
			return &speed_descriptors[i];

	return NULL;
}

static const struct soc_clock *get_soc_descriptor(int ic_clk)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(soc_clocks); i++)
		if (ic_clk == soc_clocks[i].clk_speed_mhz)
			return &soc_clocks[i];

	return NULL;
}

static int counts_from_time(const struct freq *f, int ns)
{
	return DIV_ROUND_UP(f->ticks * ns, f->ns);
}

static int counts_from_freq(const struct freq *fast, const struct freq *slow)
{
	return DIV_ROUND_UP(fast->ticks * slow->ns, fast->ns * slow->ticks);
}

/* Enable this I2C controller */
static void dw_i2c_enable(struct dw_i2c_regs *regs)
{
	uint32_t enable = read32(&regs->enable);

	if (!(enable & ENABLE_CONTROLLER))
		write32(&regs->enable, enable | ENABLE_CONTROLLER);
}

/* Disable this I2C controller */
static int dw_i2c_disable(struct dw_i2c_regs *regs)
{
	uint32_t enable = read32(&regs->enable);

	if (enable & ENABLE_CONTROLLER) {
		struct stopwatch sw;

		write32(&regs->enable, enable & ~ENABLE_CONTROLLER);

		/* Wait for enable bit to clear */
		stopwatch_init_usecs_expire(&sw, DW_I2C_TIMEOUT_US);
		while (read32(&regs->enable_status) & ENABLE_CONTROLLER)
			if (stopwatch_expired(&sw))
				return -1;
	}

	return 0;
}

/* Wait for this I2C controller to go idle for transmit */
static int dw_i2c_wait_for_bus_idle(struct dw_i2c_regs *regs)
{
	struct stopwatch sw;

	/* Start timeout for up to 16 bytes in FIFO */
	stopwatch_init_usecs_expire(&sw, 16 * DW_I2C_TIMEOUT_US);

	while (!stopwatch_expired(&sw)) {
		uint32_t status = read32(&regs->status);

		/* Check for master activity and keep waiting */
		if (status & STATUS_MASTER_ACTIVITY)
			continue;

		/* Check for TX FIFO empty to indicate TX idle */
		if (status & STATUS_TX_FIFO_EMPTY)
			return 0;
	}

	/* Timed out while waiting for bus to go idle */
	return -1;
}

/* Transfer one byte of one segment, sending stop bit if requested */
static int dw_i2c_transfer_byte(struct dw_i2c_regs *regs,
				  const struct i2c_msg *segment,
				  size_t byte, int send_stop)
{
	struct stopwatch sw;
	uint32_t cmd = CMD_DATA_CMD; /* Read op */

	stopwatch_init_usecs_expire(&sw, DW_I2C_TIMEOUT_US);

	if (!(segment->flags & I2C_M_RD)) {
		/* Write op only: Wait for FIFO not full */
		while (!(read32(&regs->status) & STATUS_TX_FIFO_NOT_FULL)) {
			if (stopwatch_expired(&sw)) {
				printk(BIOS_ERR, "I2C transmit timeout\n");
				return -1;
			}
		}
		cmd = segment->buf[byte];
	}

	/* Send stop on last byte, if desired */
	if (send_stop && byte == segment->len - 1)
		cmd |= CMD_DATA_STOP;

	write32(&regs->cmd_data, cmd);

	if (segment->flags & I2C_M_RD) {
		/* Read op only: Wait for FIFO data and store it */
		while (!(read32(&regs->status) & STATUS_RX_FIFO_NOT_EMPTY)) {
			if (stopwatch_expired(&sw)) {
				printk(BIOS_ERR, "I2C receive timeout\n");
				return -1;
			}
		}
		segment->buf[byte] = read32(&regs->cmd_data);
	}

	return 0;
}

static int _dw_i2c_transfer(unsigned int bus, const struct i2c_msg *segments,
				size_t count)
{
	struct stopwatch sw;
	struct dw_i2c_regs *regs;
	size_t byte;
	int ret = -1;

	regs = (struct dw_i2c_regs *)dw_i2c_base_address(bus);
	if (!regs) {
		printk(BIOS_ERR, "I2C bus %u base address not found\n", bus);
		return -1;
	}

	/* The assumption is that the host controller is disabled -- either
	   after running this function or from performing the intialization
	   sequence in dw_i2c_init(). */

	/* Set target slave address */
	write32(&regs->target_addr, segments->slave);

	dw_i2c_enable(regs);

	/* Process each segment */
	while (count--) {
		if (IS_ENABLED(CONFIG_DRIVERS_I2C_DESIGNWARE_DEBUG)) {
			printk(BIOS_DEBUG, "i2c %u:%02x %s %d bytes : ",
			       bus, segments->slave,
			       (segments->flags & I2C_M_RD) ? "R" : "W",
			       segments->len);
		}

		/* Read or write each byte in segment */
		for (byte = 0; byte < segments->len; byte++) {
			/*
			 * Set stop condition on final segment only.
			 * Repeated start will be automatically generated
			 * by the controller on R->W or W->R switch.
			 */
			if (dw_i2c_transfer_byte(regs, segments, byte,
						   count == 0) < 0) {
				printk(BIOS_ERR, "I2C %s failed: bus %u "
				       "addr 0x%02x\n",
				       (segments->flags & I2C_M_RD) ?
				       "read" : "write", bus, segments->slave);
				goto out;
			}
		}

		if (IS_ENABLED(CONFIG_DRIVERS_I2C_DESIGNWARE_DEBUG)) {
			int j;
			for (j = 0; j < segments->len; j++)
				printk(BIOS_DEBUG, "%02x ", segments->buf[j]);
			printk(BIOS_DEBUG, "\n");
		}

		segments++;
	}

	/* Wait for interrupt status to indicate transfer is complete */
	stopwatch_init_usecs_expire(&sw, DW_I2C_TIMEOUT_US);
	while (!(read32(&regs->raw_intr_stat) & INTR_STAT_STOP_DET)) {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "I2C stop bit not received\n");
			goto out;
		}
	}

	/* Read to clear INTR_STAT_STOP_DET */
	read32(&regs->clear_stop_det_intr);

	/* Wait for the bus to go idle */
	if (dw_i2c_wait_for_bus_idle(regs)) {
		printk(BIOS_ERR, "I2C timeout waiting for bus %u idle\n", bus);
		goto out;
	}

	/* Flush the RX FIFO in case it is not empty */
	stopwatch_init_usecs_expire(&sw, 16 * DW_I2C_TIMEOUT_US);
	while (read32(&regs->status) & STATUS_RX_FIFO_NOT_EMPTY) {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "I2C timeout flushing RX FIFO\n");
			goto out;
		}
		read32(&regs->cmd_data);
	}

	ret = 0;

out:
	read32(&regs->clear_intr);
	dw_i2c_disable(regs);
	return ret;
}

int dw_i2c_transfer(unsigned int bus, const struct i2c_msg *msg, size_t count)
{
	const struct i2c_msg *orig_msg = msg;
	size_t i;
	size_t start;
	uint16_t addr;

	if (count == 0 || !msg)
		return -1;

	/* Break up the transfers at the differing slave address boundary. */
	addr = orig_msg->slave;

	for (i = 0, start = 0; i < count; i++, msg++) {
		if (addr != msg->slave) {
			if (_dw_i2c_transfer(bus, &orig_msg[start], i - start))
				return -1;
			start = i;
			addr = msg->slave;
		}
	}

	return _dw_i2c_transfer(bus, &orig_msg[start], count - start);
}

/* Global I2C bus handler, defined in include/device/i2c_simple.h */
int platform_i2c_transfer(unsigned int bus, struct i2c_msg *msg, int count)
{
	return dw_i2c_transfer(bus, msg, count < 0 ? 0 : count);
}

static int dw_i2c_set_speed_config(unsigned int bus,
				const struct dw_i2c_speed_config *config)
{
	struct dw_i2c_regs *regs;
	void *hcnt_reg, *lcnt_reg;

	regs = (struct dw_i2c_regs *)dw_i2c_base_address(bus);
	if (!regs || !config)
		return -1;

	/* Nothing to do if no values are set */
	if (!config->scl_lcnt && !config->scl_hcnt && !config->sda_hold)
		return 0;

	if (config->speed >= I2C_SPEED_HIGH) {
		/* High and Fast Ultra speed */
		hcnt_reg = &regs->hs_scl_hcnt;
		lcnt_reg = &regs->hs_scl_lcnt;
	} else if (config->speed >= I2C_SPEED_FAST) {
		/* Fast and Fast-Plus speed */
		hcnt_reg = &regs->fs_scl_hcnt;
		lcnt_reg = &regs->fs_scl_lcnt;
	} else {
		/* Standard speed */
		hcnt_reg = &regs->ss_scl_hcnt;
		lcnt_reg = &regs->ss_scl_lcnt;
	}

	/* SCL count must be set after the speed is selected */
	if (config->scl_hcnt)
		write32(hcnt_reg, config->scl_hcnt);
	if (config->scl_lcnt)
		write32(lcnt_reg, config->scl_lcnt);

	/* Set SDA Hold Time register */
	if (config->sda_hold)
		write32(&regs->sda_hold, config->sda_hold);

	return 0;
}

static int dw_i2c_gen_config_rise_fall_time(struct dw_i2c_regs *regs,
					enum i2c_speed speed,
					const struct dw_i2c_bus_config *bcfg,
					int ic_clk,
					struct dw_i2c_speed_config *config)
{
	const struct i2c_descriptor *bus;
	const struct soc_clock *soc;
	int fall_cnt, rise_cnt, min_tlow_cnt, min_thigh_cnt, spk_cnt;
	int hcnt, lcnt, period_cnt, diff, tot;
	int data_hold_time_ns;

	bus = get_bus_descriptor(speed);
	soc = get_soc_descriptor(ic_clk);

	if (bus == NULL) {
		printk(BIOS_ERR, "dw_i2c: invalid bus speed %d\n", speed);
		return -1;
	}

	if (soc == NULL) {
		printk(BIOS_ERR, "dw_i2c: invalid SoC clock speed %d MHz\n",
			ic_clk);
		return -1;
	}

	/* Get the proper spike suppression count based on target speed. */
	if (speed >= I2C_SPEED_HIGH)
		spk_cnt = read32(&regs->hs_spklen);
	else
		spk_cnt = read32(&regs->fs_spklen);

	/* Find the period, rise, fall, min tlow, and min thigh in terms of
	 * counts of SoC clock. */
	period_cnt = counts_from_freq(&soc->freq, &bus->freq);
	rise_cnt = counts_from_time(&soc->freq, bcfg->rise_time_ns);
	fall_cnt = counts_from_time(&soc->freq, bcfg->fall_time_ns);
	min_tlow_cnt = counts_from_time(&soc->freq, bus->min_tlow_ns);
	min_thigh_cnt = counts_from_time(&soc->freq, bus->min_thigh_ns);

	printk(DW_I2C_DEBUG, "dw_i2c: SoC %d/%d ns Bus: %d/%d ns\n",
		soc->freq.ticks, soc->freq.ns, bus->freq.ticks, bus->freq.ns);
	printk(DW_I2C_DEBUG,
"		dw_i2c: period %d rise %d fall %d tlow %d thigh %d spk %d\n",
		period_cnt, rise_cnt, fall_cnt, min_tlow_cnt, min_thigh_cnt,
		spk_cnt);

	/*
	 * Back solve for hcnt and lcnt according to the following equations.
	 * SCL_High_time = [(HCNT + IC_*_SPKLEN + 7) * ic_clk] + SCL_Fall_time
	 * SCL_Low_time = [(LCNT + 1) * ic_clk] - SCL_Fall_time + SCL_Rise_time
	 */
	hcnt = min_thigh_cnt - fall_cnt - 7 - spk_cnt;
	lcnt = min_tlow_cnt - rise_cnt + fall_cnt - 1;

	if (hcnt < 0 || lcnt < 0) {
		printk(BIOS_ERR, "dw_i2c: bad counts. hcnt = %d lcnt = %d\n",
			hcnt, lcnt);
		return -1;
	}

	/* Now add things back up to ensure the period is hit. If off,
	 * split the difference and bias to lcnt for remainder. */
	tot = hcnt + lcnt + 7 + spk_cnt + rise_cnt + 1;

	if (tot < period_cnt) {
		diff = (period_cnt - tot) / 2;
		hcnt += diff;
		lcnt += diff;
		tot = hcnt + lcnt + 7 + spk_cnt + rise_cnt + 1;
		lcnt += period_cnt - tot;
	}

	config->speed = speed;
	config->scl_lcnt = lcnt;
	config->scl_hcnt = hcnt;

	/* Use internal default unless other value is specified. */
	data_hold_time_ns = DEFAULT_SDA_HOLD_TIME;
	if (bcfg->data_hold_time_ns)
		data_hold_time_ns = bcfg->data_hold_time_ns;

	config->sda_hold = counts_from_time(&soc->freq, data_hold_time_ns);

	printk(DW_I2C_DEBUG, "dw_i2c: hcnt = %d lcnt = %d sda hold = %d\n",
		hcnt, lcnt, config->sda_hold);

	return 0;
}

int dw_i2c_gen_speed_config(uintptr_t dw_i2c_addr,
					enum i2c_speed speed,
					const struct dw_i2c_bus_config *bcfg,
					struct dw_i2c_speed_config *config)
{
	const int ic_clk = CONFIG_DRIVERS_I2C_DESIGNWARE_CLOCK_MHZ;
	struct dw_i2c_regs *regs;
	uint16_t hcnt_min, lcnt_min;
	int i;

	regs = (struct dw_i2c_regs *)dw_i2c_addr;

	_Static_assert(CONFIG_DRIVERS_I2C_DESIGNWARE_CLOCK_MHZ != 0,
		"DRIVERS_I2C_DESIGNWARE_CLOCK_MHZ can't be zero!");

	/* Apply board specific override for this speed if found */
	for (i = 0; i < DW_I2C_SPEED_CONFIG_COUNT; i++) {
		if (bcfg->speed_config[i].speed != speed)
			continue;
		memcpy(config, &bcfg->speed_config[i], sizeof(*config));
		return 0;
	}

	/* If rise time is set use the time calculation. */
	if (bcfg->rise_time_ns)
		return dw_i2c_gen_config_rise_fall_time(regs, speed, bcfg,
							ic_clk, config);

	if (speed >= I2C_SPEED_HIGH) {
		/* High speed */
		hcnt_min = MIN_HS_SCL_HIGHTIME;
		lcnt_min = MIN_HS_SCL_LOWTIME;
	} else if (speed >= I2C_SPEED_FAST_PLUS) {
		/* Fast-Plus speed */
		hcnt_min = MIN_FP_SCL_HIGHTIME;
		lcnt_min = MIN_FP_SCL_LOWTIME;
	} else if (speed >= I2C_SPEED_FAST) {
		/* Fast speed */
		hcnt_min = MIN_FS_SCL_HIGHTIME;
		lcnt_min = MIN_FS_SCL_LOWTIME;
	} else {
		/* Standard speed */
		hcnt_min = MIN_SS_SCL_HIGHTIME;
		lcnt_min = MIN_SS_SCL_LOWTIME;
	}

	config->speed = speed;
	config->scl_hcnt = ic_clk * hcnt_min / KHz;
	config->scl_lcnt = ic_clk * lcnt_min / KHz;
	config->sda_hold = ic_clk * DEFAULT_SDA_HOLD_TIME / KHz;

	return 0;
}

static int dw_i2c_set_speed(unsigned int bus, enum i2c_speed speed,
				const struct dw_i2c_bus_config *bcfg)
{
	struct dw_i2c_regs *regs;
	struct dw_i2c_speed_config config;
	uint32_t control;

	/* Clock must be provided by Kconfig */
	regs = (struct dw_i2c_regs *)dw_i2c_base_address(bus);
	if (!regs || !speed)
		return -1;

	control = read32(&regs->control);
	control &= ~CONTROL_SPEED_MASK;

	if (speed >= I2C_SPEED_HIGH) {
		/* High and Fast-Ultra speed share config registers */
		control |= CONTROL_SPEED_HS;
	} else if (speed >= I2C_SPEED_FAST) {
		/* Fast speed and Fast-Plus */
		control |= CONTROL_SPEED_FS;
	} else {
		/* Standard speed */
		control |= CONTROL_SPEED_SS;
	}

	/* Generate speed config based on clock */
	if (dw_i2c_gen_speed_config((uintptr_t)regs, speed, bcfg, &config) < 0)
		return -1;

	/* Select this speed in the control register */
	write32(&regs->control, control);

	/* Write the speed config that was generated earlier */
	dw_i2c_set_speed_config(bus, &config);

	return 0;
}


/*
 * Initialize this bus controller and set the speed.
 *
 * The bus speed can be passed in Hz or using values from device/i2c.h and
 * will default to I2C_SPEED_FAST if it is not provided.
 */
int dw_i2c_init(unsigned int bus, const struct dw_i2c_bus_config *bcfg)
{
	struct dw_i2c_regs *regs;
	enum i2c_speed speed;

	if (!bcfg)
		return -1;

	speed = bcfg->speed ? : I2C_SPEED_FAST;

	regs = (struct dw_i2c_regs *)dw_i2c_base_address(bus);
	if (!regs) {
		printk(BIOS_ERR, "I2C bus %u base address not found\n", bus);
		return -1;
	}

	if (dw_i2c_disable(regs) < 0) {
		printk(BIOS_ERR, "I2C timeout disabling bus %u\n", bus);
		return -1;
	}

	/* Put controller in master mode with restart enabled */
	write32(&regs->control, CONTROL_MASTER_MODE | CONTROL_SLAVE_DISABLE |
		CONTROL_RESTART_ENABLE);

	/* Set bus speed to FAST by default */
	if (dw_i2c_set_speed(bus, speed, bcfg) < 0) {
		printk(BIOS_ERR, "I2C failed to set speed for bus %u\n", bus);
		return -1;
	}

	/* Set RX/TX thresholds to smallest values */
	write32(&regs->rx_thresh, 0);
	write32(&regs->tx_thresh, 0);

	/* Enable stop detection interrupt */
	write32(&regs->intr_mask, INTR_STAT_STOP_DET);

	printk(BIOS_INFO, "DW I2C bus %u at 0x%p (%u KHz)\n",
	       bus, regs, speed / KHz);

	return 0;
}

/*
 * Write ACPI object to describe speed configuration.
 *
 * ACPI Object: Name ("xxxx", Package () { scl_lcnt, scl_hcnt, sda_hold }
 *
 * SSCN: I2C_SPEED_STANDARD
 * FMCN: I2C_SPEED_FAST
 * FPCN: I2C_SPEED_FAST_PLUS
 * HSCN: I2C_SPEED_HIGH
 */
static void dw_i2c_acpi_write_speed_config(
	const struct dw_i2c_speed_config *config)
{
	if (!config)
		return;
	if (!config->scl_lcnt && !config->scl_hcnt && !config->sda_hold)
		return;

	if (config->speed >= I2C_SPEED_HIGH)
		acpigen_write_name("HSCN");
	else if (config->speed >= I2C_SPEED_FAST_PLUS)
		acpigen_write_name("FPCN");
	else if (config->speed >= I2C_SPEED_FAST)
		acpigen_write_name("FMCN");
	else
		acpigen_write_name("SSCN");

	/* Package () { scl_lcnt, scl_hcnt, sda_hold } */
	acpigen_write_package(3);
	acpigen_write_word(config->scl_hcnt);
	acpigen_write_word(config->scl_lcnt);
	acpigen_write_dword(config->sda_hold);
	acpigen_pop_len();
}

/*
 * The device should already be enabled and out of reset,
 * either from early init in coreboot or SiliconInit in FSP.
 */
void dw_i2c_dev_init(struct device *dev)
{
	const struct dw_i2c_bus_config *config;
	int bus = dw_i2c_soc_dev_to_bus(dev);

	if (bus < 0)
		return;

	config = dw_i2c_get_soc_cfg(bus);

	if (!config)
		return;

	dw_i2c_init(bus, config);
}

/*
 * Generate I2C timing information into the SSDT for the OS driver to consume,
 * optionally applying override values provided by the caller.
 */
void dw_i2c_acpi_fill_ssdt(struct device *dev)
{
	const struct dw_i2c_bus_config *bcfg;
	uintptr_t dw_i2c_addr;
	struct dw_i2c_speed_config sgen;
	enum i2c_speed speeds[DW_I2C_SPEED_CONFIG_COUNT] = {
		I2C_SPEED_STANDARD,
		I2C_SPEED_FAST,
		I2C_SPEED_FAST_PLUS,
		I2C_SPEED_HIGH,
	};
	int i, bus;
	const char *path;

	if (!dev->enabled)
		return;

	bus = dw_i2c_soc_dev_to_bus(dev);

	if (bus < 0)
		return;

	bcfg = dw_i2c_get_soc_cfg(bus);

	if (!bcfg)
		return;

	dw_i2c_addr = dw_i2c_base_address(bus);
	if (!dw_i2c_addr)
		return;

	path = acpi_device_path(dev);
	if (!path)
		return;

	acpigen_write_scope(path);

	/* Report timing values for the OS driver */
	for (i = 0; i < DW_I2C_SPEED_CONFIG_COUNT; i++) {
		/* Generate speed config. */
		if (dw_i2c_gen_speed_config(dw_i2c_addr, speeds[i], bcfg,
						&sgen) < 0)
			continue;

		/* Generate ACPI based on selected speed config */
		dw_i2c_acpi_write_speed_config(&sgen);
	}

	acpigen_pop_len();
}

static int dw_i2c_dev_transfer(struct device *dev,
				const struct i2c_msg *msg, size_t count)
{
	return dw_i2c_transfer(dw_i2c_soc_dev_to_bus(dev), msg, count);
}

const struct i2c_bus_operations dw_i2c_bus_ops = {
	.transfer = dw_i2c_dev_transfer,
};
