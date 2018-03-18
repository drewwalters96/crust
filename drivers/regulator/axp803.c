/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <debug.h>
#include <error.h>
#include <i2c.h>
#include <regulator.h>
#include <stddef.h>
#include <timer.h>
#include <regulator/axp803.h>

static int
axp803_probe(struct device *dev)
{
	int     err;
	uint8_t addr, reg;

	if ((err = i2c_probe(dev->bus, 0x34))) {
		error("%s: i2c probe 0x%x error %u",
		      dev->name, dev->addr, err);
		goto end;
	}

	addr = 0x00;
	if ((err = i2c_read_reg(dev->bus, dev->addr, addr, &reg)))
		error("%s: i2c read %u error %u", dev->name, addr, err);
	else
		info("%s: Power souce status = 0x%02x", dev->name, reg);

	addr = 0x03;
	if ((err = i2c_read_reg(dev->bus, dev->addr, 0x00, &reg)))
		error("%s: i2c read %u error %u", dev->name, addr, err);
	else
		info("%s: IC type = 0x%02x", dev->name, reg);

	addr = 0x21;
	if ((err = i2c_read_reg(dev->bus, dev->addr, 0x00, &reg))) {
		error("%s: i2c read %u error %u", dev->name, addr, err);
	} else {
		uint32_t voltage = reg;
		if (voltage <= 70)
			voltage = 900 + voltage * 10;
		else
			voltage = 1220 + (voltage - 70) * 20;
		info("%s: CPU voltage = %dmV", dev->name, voltage);
	}

end:
	return SUCCESS;
}

const struct regulator_driver axp803_driver = {
	.drv = {
		.name  = "axp803",
		.class = DM_CLASS_REGULATOR,
		.probe = axp803_probe,
	},
};
