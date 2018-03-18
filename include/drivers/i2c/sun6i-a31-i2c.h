/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#ifndef DRIVERS_I2C_SUN6I_A31_I2C_H
#define DRIVERS_I2C_SUN6I_A31_I2C_H

#include <i2c.h>

#define I2C_NUM_PINS 2

#define I2C_PINS     (struct gpio_pin *)(struct gpio_pin[I2C_NUM_PINS])

extern const struct i2c_driver sun6i_a31_i2c_driver;

#endif /* DRIVERS_I2C_SUN6I_A31_I2C_H */
