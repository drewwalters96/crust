/*
 * Copyright © 2017 Samuel Holland <samuel@sholland.org>
 * Copyright © 2018 Drew Walters <drewwalters96@gmail.com>
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#ifndef DRIVERS_WATCHDOG_H
#define DRIVERS_WATCHDOG_H

#include <dm.h>
#include <stdint.h>

#define WATCHDOG_OPS(dev) ((struct watchdog_driver_ops *)((dev)->drv->ops))

struct watchdog_driver_ops {
	int (*restart)(struct device *dev);
	int (*disable)(struct device *dev);
	int (*enable)(struct device *dev, uint32_t timeout);
};

static inline int
watchdog_restart(struct device *dev)
{
	return WATCHDOG_OPS(dev)->restart(dev);
}

static inline int
watchdog_disable(struct device *dev)
{
	return WATCHDOG_OPS(dev)->disable(dev);
}

static inline int
watchdog_enable(struct device *dev, uint32_t timeout)
{
	return WATCHDOG_OPS(dev)->enable(dev, timeout);
}

#endif /* DRIVERS_WATCHDOG_H */
