/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <dm.h>
#include <error.h>

#ifndef DRIVERS_REGULATOR_H
#define DRIVERS_REGULATOR_H

struct regulator_driver {
	const struct driver drv;
};

#endif /* DRIVERS_REGULATOR_H */
