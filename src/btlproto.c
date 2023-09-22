/*
 *
 */

#include <string.h>

#include "target.h"
#include "btlproto.h"
#include "flash.h"

static uint8_t crc8_cal_buf(const void *data, int len)
{
	int i;
	uint8_t crc = 0;
	const uint8_t *p = data;

	for (i = 0; i < len; i++)
		crc = crc8_calc(crc, *p++, BTL_CRC_POLY);

	return crc;
}

#if 0
static void btl_write(uint8_t status, const void *data, unsigned int len)
{
	uint8_t buf[BTL_MAX_PKT_SIZE];
	btl_packet_t *pkt = (btl_packet_t *)buf;

	pkt->prefix = BTL_PKT_PREXIX;
	pkt->size = len;
	pkt->cmd = BTL_CMD_INFO | BTL_PKT_REPLY;
	pkt->status = status;

	memcpy(pkt->data, data, len);

	btl_packet_crc(pkt) = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));

	btl_write_pkt(pkt, btl_size_pkt(pkt));
}
#endif

static void btl_make_header(btl_packet_t *pkt, uint8_t status, uint8_t size)
{
	pkt->prefix = BTL_PKT_PREXIX;
	pkt->size = size;
	pkt->cmd |= BTL_PKT_REPLY;
	pkt->status = status;
}

/*
 * Protect bootloader area
 */
static int btl_area(uint32_t addr, uint8_t size)
{
	if (((addr >= BTL_ADDR && (addr + size) < (BTL_ADDR + BTL_SIZE)) ||
	    ((addr + size) >= BTL_ADDR && (addr + size) < (BTL_ADDR + BTL_SIZE))))
		return 1;

	return 0;
}

/*
 * Command handlers
 */
static int btl_cmd_info(btl_packet_t *pkt)
{
	int size = sizeof(BTL_VERSION_STR);

	memcpy(pkt->data, BTL_VERSION_STR, size);
	return size;
}

static int btl_cmd_read(btl_packet_t *pkt)
{
	return flash_read(pkt->addr, pkt->data, BTL_MAX_DATA_SIZE);
}

static int btl_cmd_write(btl_packet_t *pkt)
{
	if (btl_area(pkt->addr, pkt->size))
		return -1;

	if (flash_write(pkt->addr, pkt->data, pkt->size) < 0)
		return -1;
	return 0;
}

static int btl_cmd_erase(btl_packet_t *pkt)
{
	if (btl_area(pkt->addr, pkt->size))
		return -1;

	if (flash_erase(pkt->addr, pkt->size) < 0)
		return -1;

	return 0;
}

int btl_handle_packet(btl_packet_t *pkt)
{
	uint8_t crc = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));
	int sz = -1;

	if (crc != btl_packet_crc(pkt))
		return -1;

	switch (pkt->cmd) {
		case BTL_CMD_INFO:
			sz = btl_cmd_info(pkt);
			break;
		case BTL_CMD_READ:
			sz = btl_cmd_read(pkt);
			break;
		case BTL_CMD_WRITE:
			sz = btl_cmd_write(pkt);
			break;
		case BTL_CMD_ERASE:
			sz = btl_cmd_erase(pkt);
			break;
		default:
			break;
	}

	if (sz < 0)
		btl_make_header(pkt, BTL_STATUS_ERROR, 0);
	else
		btl_make_header(pkt, BTL_STATUS_OK, sz);

	btl_packet_crc(pkt) = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));
	return sz;
}

