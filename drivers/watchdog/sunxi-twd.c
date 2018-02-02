/*
 * Copyright © 2017 Samuel Holland <samuel@sholland.org>
 * Copyright © 2018 Drew Walters <drewwalters96@gmail.com>
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <error.h>
#include <mmio.h>
#include <drivers/clock.h>
#include <drivers/watchdog.h>
#include <drivers/watchdog/sunxi-twd.h>

#define TWD_CTRL_REG        0x10
#define TWD_RESTART_REG     0x14
#define TWD_INTV_VAL_REG    0x30

#define TWD_RESTART_KEY_VAL (0xD14 << 16)

static int
sunxi_twd_restart(struct device *dev)
{
	uint32_t reg;

	/* Enable and perform restart. */
	reg  = BIT(0);
	reg |= TWD_RESTART_KEY_VAL;

	mmio_write32(dev->regs + TWD_RESTART_REG, reg);

	return SUCCESS;
}

static int
sunxi_twd_disable(struct device *dev)
{
	uint32_t reg;

	/* Stop trusted watchdog. */
	reg  = mmio_read32(dev->regs + TWD_CTRL_REG);
	reg |= BIT(1);

	mmio_write32(dev->regs + TWD_CTRL_REG, reg);

	return SUCCESS;
}

static int
sunxi_twd_enable(struct device *dev, uint32_t timeout)
{
	uint32_t reg;
	uint32_t timeout_intv;

	/* Convert timeout value from microseconds to timeout interval.
	 *
	 * TODO(@smaeul): Use clock_get_freq(dev) once clock driver is
	 *                written.
	 */
	timeout_intv = timeout * 24;
	mmio_write32(dev->regs + TWD_INTV_VAL_REG, timeout_intv);

	/* Reset enable. */
	reg  = mmio_read32(dev->regs + TWD_CTRL_REG);
	reg |= BIT(9);

	mmio_write32(dev->regs + TWD_CTRL_REG, reg);

	sunxi_twd_restart(dev);

	return SUCCESS;
}

static int
sunxi_twd_probe(struct device *dev)
{
	int err;

	/* Enable clock and disable watchdog to achieve known state. */
	if ((err = clock_enable(dev)))
		return err;

	return sunxi_twd_disable(dev);
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
