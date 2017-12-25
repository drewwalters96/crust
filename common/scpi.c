/*
 * Copyright © 2017 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 */

#include <compiler.h>
#include <dm.h>
#include <mmio.h>
#include <scpi.h>
#include <stdint.h>
#include <util.h>
#include <drivers/msgbox.h>
#include <platform/memory.h>

#define SCPI_MEM_AREAS      4
#define SCPI_MEM_AREA_SIZE  (SCPI_MEM_SIZE / SCPI_MEM_AREAS)
#define SCPI_PAYLOAD_SIZE   (SCPI_MEM_AREA_SIZE - 2 * sizeof(uint32_t))

#define SCPI_MEM_AREA(n)    (&((struct scpi_mem *)SCPI_MEM_BASE)[n])
#define SCPI_RX_MEM_AREA(n) SCPI_MEM_AREA(2 * (n) + 1)
#define SCPI_TX_MEM_AREA(n) SCPI_MEM_AREA(2 * (n))

#define SCPI_MSGBOX_CHAN    0
#define SCPI_VIRTUAL_CHAN   BIT(0)

struct scpi_mem {
	uint32_t header;
	uint32_t status;
	uint32_t payload[SCPI_PAYLOAD_SIZE / sizeof(uint32_t)];
} __packed;

static void scpi_msg_handler(struct device *dev, uint8_t chan, uint32_t msg);

void
scpi_init(void)
{
	msgbox_register_handler(dm_get_by_class(DM_CLASS_MSGBOX),
	                        SCPI_MSGBOX_CHAN,
	                        scpi_msg_handler);
}

static void
scpi_msg_handler(struct device *dev, uint8_t chan, uint32_t msg)
{
	struct scpi_mem *rx_mem = SCPI_RX_MEM_AREA(chan);
	struct scpi_mem *tx_mem = SCPI_TX_MEM_AREA(chan);
	uint32_t payload_size, status;

	if (msg != SCPI_VIRTUAL_CHAN)
		return;

	switch (SCPI_HEADER_GET_CMD(rx_mem->header)) {
	case SCPI_CMD_GET_SCP_CAP:
		payload_size       = 28;
		status             = SCPI_OK;
		tx_mem->payload[0] = SCPI_CAP_VERSION(1, 2);
		tx_mem->payload[1] = SCPI_CAP_LIMITS(SCPI_PAYLOAD_SIZE,
		                                     SCPI_PAYLOAD_SIZE);
		tx_mem->payload[2] = SCPI_CAP_FWVERSION(0, 1, 0);
		tx_mem->payload[3] = BITMAP_BIT(SCPI_CMD_SCP_READY) |
		                     BITMAP_BIT(SCPI_CMD_GET_SCP_CAP);
		tx_mem->payload[4] = 0;
		tx_mem->payload[5] = 0;
		tx_mem->payload[6] = 0;
		break;
	default:
		payload_size = 0;
		status       = SCPI_E_SUPPORT;
	}

	tx_mem->header = SCPI_HEADER_SET_SIZE(rx_mem->header, payload_size);
	tx_mem->status = status;
	msgbox_send_msg(dev, chan, SCPI_VIRTUAL_CHAN);
}

void
scpi_ready(void)
{
	struct scpi_mem *tx_mem = SCPI_TX_MEM_AREA(0);

	tx_mem->header = SCPI_HEADER(SCPI_CMD_SCP_READY, 0, 0);
	tx_mem->status = SCPI_OK;
	msgbox_send_msg(dm_get_by_class(DM_CLASS_MSGBOX),
	                SCPI_MSGBOX_CHAN,
	                SCPI_VIRTUAL_CHAN);
}
