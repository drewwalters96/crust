/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <debug.h>
#include <delay.h>
#include <dm.h>
#include <i2c.h>
#include <mmio.h>
#include <stddef.h>
#include <util.h>

int
i2c_probe(struct device *dev)
{
	struct device *i2c_dev = dev->bus;
	int start_result;

	assert(i2c_dev);

	/* Start and stop a transaction. */
	start_result = I2C_OPS(i2c_dev)->start(i2c_dev, dev->addr, I2C_WRITE);
	I2C_OPS(i2c_dev)->stop(i2c_dev);

	return start_result;
}

int
i2c_read_reg(struct device *dev, uint8_t addr, uint8_t *data)
{
	struct device *i2c_dev = dev->bus;
	int result;

	assert(i2c_dev);

	/* Start transaction. */
	if (!(result = I2C_OPS(i2c_dev)->start(i2c_dev, dev->addr, I2C_WRITE)))
		goto abort;

	/* Write register address. */
	if (!(result = I2C_OPS(i2c_dev)->write(i2c_dev, addr)))
		goto abort;

	/* Repeat start. */
	if (!(result = I2C_OPS(i2c_dev)->start(i2c_dev, dev->addr, I2C_READ)))
		goto abort;

	/* Read data */
	if (!(result = I2C_OPS(i2c_dev)->read(i2c_dev, data)))
		goto abort;

	/* Stop transaction. */
abort:
	I2C_OPS(i2c_dev)->stop(i2c_dev);

	return result;
}

int
i2c_write_reg(struct device *dev, uint8_t addr, uint8_t data)
{
	struct device *i2c_dev = dev->bus;
	int result;

	assert(i2c_dev);

	/* Start transaction. */
	if (!(result = I2C_OPS(i2c_dev)->start(i2c_dev, dev->addr, I2C_WRITE)))
		goto abort;

	/* Write register address. */
	if (!(result = I2C_OPS(i2c_dev)->write(i2c_dev, addr)))
		goto abort;

	/* Write data */
	if (!(result = I2C_OPS(i2c_dev)->write(i2c_dev, data)))
		goto abort;

	/* Stop transaction. */
abort:
	I2C_OPS(i2c_dev)->stop(i2c_dev);

	return result;
}
