/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <error.h>
#include <mmio.h>
#include <drivers/clock.h>
#include <drivers/watchdog.h>
#include <drivers/watchdog/sunxi-twd.h>

#define TWD_STATUS_REG      0x00
#define TWD_CTRL_REG        0x10
#define TWD_RESTART_REG     0x14
#define TWD_INTV_VAL_REG    0x30

#define TWD_RESTART_KEY_VAL (0xD14 << 16)

static int
sunxi_twd_restart(struct device *dev)
{
	uint32_t reg;

	/* Enable and perform restart. */
	reg = TWD_RESTART_KEY_VAL | BIT(0);
	mmio_write32(dev->regs + TWD_RESTART_REG, reg);

	return SUCCESS;
}

static int
sunxi_twd_disable(struct device *dev)
{
	/* Disable system reset, stop watchdog counter. */
	mmio_clearsetbits32(dev->regs + TWD_CTRL_REG, BIT(9), BIT(1));

	/* Clear any pending interrupt. */
	mmio_write32(dev->regs + TWD_STATUS_REG, BIT(0));

	return SUCCESS;
}

static int
sunxi_twd_enable(struct device *dev, uint32_t timeout)
{
	/* Convert timeout value from microseconds to timeout interval.
	 *
	 * TODO(@smaeul): Use clock_get_freq(dev) once clock driver is
	 *                written.
	 */
	uint32_t interval = timeout * 24;

	/* Program interval until watchdog fires. */
	mmio_write32(dev->regs + TWD_INTV_VAL_REG, interval);

	/* Resume watchdog counter, enable system reset. */
	mmio_clearsetbits32(dev->regs + TWD_CTRL_REG, BIT(1), BIT(9));

	return sunxi_twd_restart(dev);
}

static int
sunxi_twd_probe(struct device *dev)
{
	int err;

	/* Verify watchdog is disabled. */
	if ((err = sunxi_twd_disable(dev)))
		return err;

	/* Set counter clock source to OSC24M. */
	mmio_setbits32(dev->regs + TWD_CTRL_REG, BIT(31));

	return SUCCESS;
}

static const struct watchdog_driver_ops sunxi_twd_driver_ops = {
	.restart = sunxi_twd_restart,
	.disable = sunxi_twd_disable,
	.enable  = sunxi_twd_enable,
};

const struct driver sunxi_twd_driver = {
	.name  = "sunxi-twd",
	.class = DM_CLASS_WATCHDOG,
	.probe = sunxi_twd_probe,
	.ops   = &sunxi_twd_driver_ops,
};
