/*
 * Copyright © 2017 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#ifndef SCPI_H
#define SCPI_H

#include <util.h>

#define SCPI_HEADER_CMD(n)         ((n) & 0xff)
#define SCPI_HEADER_GET_CMD(n)     ((n) & 0xff)
#define SCPI_HEADER_SENDER(n)      (((n) & 0xff) << 8)
#define SCPI_HEADER_GET_SENDER(n)  (((n) >> 8) & 0xff)
#define SCPI_HEADER_SIZE(n)        (((n) & 0xff) << 16)
#define SCPI_HEADER_GET_SIZE(n)    (((n) >> 16) & 0xff)
#define SCPI_HEADER_SET_SIZE(r, s) (((r) & U(0xff00ffff)) | \
	                            SCPI_HEADER_SIZE(s))

#define SCPI_HEADER(cmd, sender, size) \
	(SCPI_HEADER_CMD(cmd) | \
	 SCPI_HEADER_SENDER(sender) | \
	 SCPI_HEADER_SIZE(size))

/* Command 0x02: Get SCP capability */
#define SCPI_CAP_VERSION(x, y) ((((x) & 0xfff) << 16) | ((y) & 0xffff))
#define SCPI_CAP_LIMITS(x, y)  ((((x) & 0xf) << 16) | ((y) & 0xff))
#define SCPI_CAP_FWVERSION(x, y, z) \
	((((x) & 0xff) << 24) | (((y) & 0xff) << 16) | ((z) & 0xffff))

enum {
	SCPI_CMD_SCP_READY   = 0x01,
	SCPI_CMD_GET_SCP_CAP = 0x02,
	SCPI_CMD_SET_CSS_PWR = 0x03,
	SCPI_CMD_GET_CSS_PWR = 0x04,
	SCPI_CMD_SET_SYS_PWR = 0x05,
};

enum {
	SCPI_OK         = 0,
	SCPI_E_PARAM    = 1,
	SCPI_E_ALIGN    = 2,
	SCPI_E_SIZE     = 3,
	SCPI_E_HANDLER  = 4,
	SCPI_E_ACCESS   = 5,
	SCPI_E_RANGE    = 6,
	SCPI_E_TIMEOUT  = 7,
	SCPI_E_NOMEM    = 8,
	SCPI_E_PWRSTATE = 9,
	SCPI_E_SUPPORT  = 10,
	SCPI_E_DEVICE   = 11,
	SCPI_E_BUSY     = 12,
	SCPI_E_OS       = 13,
	SCPI_E_DATA     = 14,
	SCPI_E_STATE    = 15,
};

void scpi_init(void);
void scpi_ready(void);

#endif /* SCPI_H */
