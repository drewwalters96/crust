/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <clock.h>
#include <delay.h>
#include <dm.h>
#include <error.h>
#include <i2c.h>
#include <mmio.h>
#include <pio.h>
#include <stddef.h>
#include <util.h>
#include <wallclock.h>

#define I2C_CCR_REG   0x14
#define I2C_CTRL_REG  0x0c
#define I2C_DATA_REG  0x08
#define I2C_EFR_REG   0x1c
#define I2C_LCR_REG   0x20
#define I2C_SRST_REG  0x18
#define I2C_STAT_REG  0x10
#define I2C_XADDR_REG 0x04

#define I2C_TIMEOUT   1000 /*< Timeout in μs. */

enum {
	START_COND_TX        = 0x08,
	START_COND_TX_REPEAT = 0x10,
	ADDR_WRITE_RX_ACK    = 0x18,
	ADDR_WRITE_RX_NACK   = 0x20,
	DATA_TX_ACK          = 0x28,
	DATA_TX_NACK         = 0x30,
	ADDR_READ_TX_ACK     = 0x40,
	ADDR_READ_RX_ACK     = 0x48,
	DATA_RX_NACK         = 0x58,
	IDLE                 = 0xf8,
};

static bool sun6i_a31_i2c_verify_status(struct device *dev, uint8_t status);
static bool sun6i_a31_i2c_wait_idle(struct device *dev);
static int sun6i_a31_i2c_write(struct device *dev, uint8_t data);

static int
sun6i_a31_i2c_read(struct device *dev, uint8_t *data)
{
	/* Wait for data. */
	if (!sun6i_a31_i2c_verify_status(dev, DATA_RX_NACK))
		return ENODEV;

	/* Read data. */
	*data = mmio_read32(dev->regs + I2C_DATA_REG);

	return SUCCESS;
}

static int
sun6i_a31_i2c_start(struct device *dev, uint8_t addr, uint8_t flag)
{
	uint8_t  status;
	uint32_t init_status = mmio_read32(dev->regs + I2C_STAT_REG);

	/* Send start. */
	mmio_setbits32(dev->regs + I2C_CTRL_REG, BIT(5) | BIT(3));

	/**
	 *  Check for start condition if initial status is IDLE, otherwise
	 *  check for repeated start.
	 */
	status = init_status == IDLE ? START_COND_TX : START_COND_TX_REPEAT;
	if (!sun6i_a31_i2c_verify_status(dev, status))
		return EIO;

	/* Write device address and flag. */
	mmio_write32(dev->regs + I2C_DATA_REG, addr << 1 | 1);
	mmio_setbits32(dev->regs + I2C_CTRL_REG, BIT(3));

	/* Check for address acknowledgement. */
	if (!sun6i_a31_i2c_verify_status(dev, ADDR_WRITE_RX_ACK))
		return ENODEV;

	return SUCCESS;
}

static int
sun6i_a31_i2c_stop(struct device *dev)
{
	/* Send stop. */
	mmio_clearbits32(dev->regs + I2C_CTRL_REG, BIT(4) | BIT(3));

	if (!sun6i_a31_i2c_verify_status(dev, IDLE))
		return EIO;

	return SUCCESS;
}

// static bool
// sun6i_a31_i2c_verify_status(struct device *dev, uint8_t status)
// {
// 	uint64_t timeout = wallclock_read() + I2C_TIMEOUT;
// 	uint8_t current_stat;

// 	while (!(mmio_read32(I2C_CTRL_REG) & BIT(3))) {
// 		/* Check for timeot. */
// 		if (wallclock_read() >= timeout) {
// 			if (status == IDLE)
// 				break;
// 			error("%s: Timeout waiting for status 0x%02x", dev->name, status);
// 			return false;
// 		}
// 	}

// 	/* Verify current status matches expected status. */
// 	if ((current_stat = mmio_read32(dev->regs + I2C_STAT_REG)) != status) {
// 		error("%s: Status 0x%02x does not match expected status 0x%02x", dev->name, current_stat, status);
// 		return false;
// 	}

// 	return true;
// }

static bool sun6i_a31_i2c_verify_status(struct device *dev, uint8_t status)
{
	if (status == IDLE)
		return sun6i_a31_i2c_wait_idle(dev);

	/* Timeout for I2C state change in 100kHz bus cycles */
	int timeout = 100;
	uint32_t reg;

	while (!(reg = (mmio_read32(dev->regs + I2C_CTRL_REG) & BIT(3)))) {
		/* 10 usecs = 1 100kHz bus cycle */
		udelay(10);
		if (!(timeout -= 1)) {
 			error("%s: Timeout waiting for status 0x%02x", dev->name, status);
			 return false;
		}
	}
	if ((reg = mmio_read32(dev->regs + I2C_STAT_REG)) != status) {
		error("%s: Status 0x%02x does not match expected status 0x%02x", dev->name, reg, status);
		return false;
	}

	return true;
}

static bool sun6i_a31_i2c_wait_idle(struct device *dev)
{
	/* Timeout for I2C state change in 100kHz bus cycles */
	int timeout = 10;
	uint32_t reg;

	while ((reg = mmio_read32(dev->regs + I2C_STAT_REG)) != IDLE) {
		/* 10 usecs = 1 100kHz bus cycle */
		udelay(10);
		if (!(timeout -= 1)) {
			error("%s: Timeout waiting for status 0x%02x", dev->name, IDLE);
			return false;
		}
	}

	return true;
}

static int
sun6i_a31_i2c_write(struct device *dev, uint8_t data)
{
	warn("Trying to write data %u", data);
	/* Write data. */
	mmio_setbits32(dev->regs + I2C_DATA_REG, data);

	/* Move to next state. */
	mmio_setbits32(dev->regs + I2C_CTRL_REG, BIT(3));

	if (!sun6i_a31_i2c_verify_status(dev, DATA_TX_ACK))
		return ENODEV;

	return SUCCESS;
}

static const struct i2c_driver_ops sun6i_a31_i2c_driver_ops = {
	.read  = sun6i_a31_i2c_read,
	.start = sun6i_a31_i2c_start,
	.stop  = sun6i_a31_i2c_stop,
	.write = sun6i_a31_i2c_write,
};

static int
sun6i_a31_i2c_probe(struct device *dev)
{
	int err;

	if ((err = clock_enable(dev)))
		return err;

	/* Set port L pins 0-1 to I2C. */
	pio_set_mode(dev->bus, 0, 2);
	pio_set_mode(dev->bus, 1, 2);

	/**
	 * Set I2C bus clock divider for 100 KHz operation.
	 * TODO(@drewwalters96): Set using clock_get_rate() after #103 is
	 * fixed.
	 */
	mmio_setbits32(dev->regs + I2C_CCR_REG, 0x00000011);

	/* Clear address. */
	mmio_write32(dev->regs, 0);
	mmio_write32(dev->regs + I2C_XADDR_REG, 0);

	/* Enable I2C bus, disable interrupts, send NACK after receive */
	mmio_write32(dev->regs + I2C_CTRL_REG, BIT(6));

	/* Disable extended features. */
	mmio_write32(dev->regs + I2C_EFR_REG, 0);

	/* Disable manual control of line levels. */
	mmio_clearbits32(dev->regs + I2C_LCR_REG, BIT(2) | BIT(0));

	/* Soft reset the controller. */
	mmio_setbits32(dev->regs + I2C_SRST_REG, BIT(0));

	if (!sun6i_a31_i2c_verify_status(dev, IDLE)) {
		error("%s: Failed to initialize", dev->name);
		debug("Status is %u", mmio_read32(dev->regs + I2C_STAT_REG));
		return EIO;
	}

	info("%s: Successfully initialized", dev->name);
	return SUCCESS;
}

const struct driver sun6i_a31_i2c_driver = {
	.name  = "sun6i-a31-i2c",
	.class = DM_CLASS_I2C,
	.probe = sun6i_a31_i2c_probe,
	.ops   = &sun6i_a31_i2c_driver_ops,
};
