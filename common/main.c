/*
 * Copyright © 2017-2018 The Crust Firmware Authors.
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <console.h>
#include <debug.h>
#include <dm.h>
#include <stdbool.h>
#include <work.h>
#include <drivers/watchdog.h>
#include <platform/devices.h>

#define WDOG_TIMEOUT (5 * 1000 * 1000) /* 5 seconds */

void main(void);

void
main(void)
{
	struct device *twd_dev;

	console_init(DEV_UART0);
	dm_init();

	/* Enable trusted watchdog. */
	if ((twd_dev = dm_get_by_class(DM_CLASS_WATCHDOG))) {
		watchdog_enable(twd_dev, WDOG_TIMEOUT);
		info("Trusted watchdog enabled.");
	} else {
		warn("Trusted watchdog did not start.");
	}

	/* Process work queue. */
	while (true) {
		process_work();

		/* TODO: Enter sleep state */
	}
}
