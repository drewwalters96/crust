/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <clock.h>
#include <dm.h>
#include <error.h>
#include <i2c.h>
#include <mmio.h>
#include <pio.h>
#include <stddef.h>
#include <util.h>
#include <wallclock.h>

#define I2C_CCR_REG   0x14
#define I2C_CTRL_REG  0x0C
#define I2C_DATA_REG  0x08
#define I2C_EFR_REG   0x1C
#define I2C_LCR_REG   0x20
#define I2C_SRST_REG  0x18
#define I2C_STAT_REG  0x10
#define I2C_XADDR_REG 0x04

#define I2C_TIMEOUT   10 /*< Timeout in μs. */

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
static int sun6i_a31_i2c_write(struct device *dev, uint8_t data);

static int
sun6i_a31_i2c_read(struct device *dev, uint8_t *data)
{
	/* Wait for data. */
	if (sun6i_a31_i2c_verify_status(dev, DATA_RX_NACK))
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
	if (sun6i_a31_i2c_verify_status(dev, status))
		return EIO;

	/* Write device address and flag. */
	if (sun6i_a31_i2c_write(dev, addr << 1 | flag)) {
		if (!sun6i_a31_i2c_verify_status(dev, ADDR_WRITE_RX_ACK))
			return ENODEV;
	} else {
		return EIO;
	}

	/* Move to next state. */
	mmio_setbits32(dev->regs + I2C_CTRL_REG, BIT(3));

	return SUCCESS;
}

static int
sun6i_a31_i2c_stop(struct device *dev)
{
	/* Send stop. */
	mmio_clearbits32(dev->regs + I2C_CTRL_REG, BIT(4) | BIT(3));

	if (sun6i_a31_i2c_verify_status(dev, IDLE))
		return EIO;

	return SUCCESS;
}

static bool
sun6i_a31_i2c_verify_status(struct device *dev, uint8_t status)
{
	uint64_t timeout = wallclock_read() + I2C_TIMEOUT;

	while ((mmio_read32(dev->regs + I2C_STAT_REG)) != status) {
		/* Check for timeout. */
		if (wallclock_read() >= timeout) {
			error("%s: Timeout waiting for status %u",
			      dev->name, status);
			return false;
		}
	}

	return true;
}

static int
sun6i_a31_i2c_write(struct device *dev, uint8_t data)
{
	/* Write data. */
	mmio_setbits32(dev->regs + I2C_DATA_REG, data);

	/* Move to next state. */
	mmio_setbits32(dev->regs + I2C_CTRL_REG, BIT(3));

	if (sun6i_a31_i2c_verify_status(dev, DATA_TX_ACK))
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
	pio_set_mode(dev, 0, 010);
	pio_set_mode(dev, 1, 010);

	/**
	 * Set I2C bus clock divider for 24MHz operation.
	 * TODO(@drewwalters96): Set using clock_get_rate() after #103 is
	 * fixed.
	 */
	mmio_setbits32(dev->regs + I2C_CCR_REG, BIT(0));

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

	if (sun6i_a31_i2c_verify_status(dev, IDLE)) {
		error("%s: Failed to initialize", dev->name);
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
