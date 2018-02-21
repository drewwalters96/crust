/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#ifndef DRIVERS_I2C_H
#define DRIVERS_I2C_H

#include <dm.h>
#include <error.h>
#include <stdbool.h>
#include <stdint.h>

#define I2C_OPS(dev) ((struct i2c_driver_ops *)((dev)->drv->ops))

struct i2c_driver_ops {
	int (*read)(struct device *dev, uint8_t *data);
	int (*start)(struct device *dev, int i2c_op);
	int (*stop)(struct device *dev);
	int (*write)(struct device *dev, uint8_t data);
};

enum I2COps {
	I2C_READ,
	I2C_WRITE,
};

/**
 * TODO(@drewwalters96): Add function documentation.
 */
static inline int
i2c_probe(struct device *dev)
{
	struct device *i2c_dev = dev->bus;

	return I2C_OPS(i2c_dev)->write(i2c_dev, (dev->addr << 1));
}

static inline int
i2c_read_reg(struct device *dev, uint8_t addr, uint8_t *data)
{
	struct device *i2c_dev = dev->bus;

	/* Start transaction. */
	if (!I2C_OPS(i2c_dev)->start(i2c_dev, I2C_WRITE))
		return EIO;

	/* Write device address and write flag. */
	if (!I2C_OPS(i2c_dev)->write(i2c_dev, dev->addr << 1))
		return ENODEV;

	/* Write register address. */
	if (!I2C_OPS(i2c_dev)->write(i2c_dev, addr))
		return EIO;

	/* Repeat start. */
	if (!I2C_OPS(i2c_dev)->start(i2c_dev, I2C_READ))

		/* Send register address and read flag. */
		if (!I2C_OPS(i2c_dev)->write(i2c_dev, addr << 1 | BIT(0)))
			return EIO;

	/* Read data */
	if (!I2C_OPS(i2c_dev)->read(i2c_dev, data))
		return EIO;

	/* Stop transaction. */
	return I2C_OPS(i2c_dev)->stop(i2c_dev);
}

static inline int
i2c_write_reg(struct device *dev, uint8_t addr, uint8_t data)
{
	struct device *i2c_dev = dev->bus;

	/* Start transaction. */
	if (!I2C_OPS(i2c_dev)->start(i2c_dev, I2C_WRITE))
		return EIO;

	/* Write device address. */
	if (!I2C_OPS(i2c_dev)->write(i2c_dev, dev->addr << 1))
		return EIO;

	/* Send register address. */
	if (!I2C_OPS(i2c_dev)->write(i2c_dev, addr << 1))
		return EIO;

	/* Write data */
	if (!I2C_OPS(i2c_dev)->write(i2c_dev, data))
		return EIO;

	/* Stop transaction. */
	return I2C_OPS(i2c_dev)->stop(i2c_dev);
}

bool i2c_status_verify(struct device *dev, uint8_t status);

#endif /* DRIVERS_I2C_H */
