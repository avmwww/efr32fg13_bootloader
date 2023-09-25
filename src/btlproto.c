/*
 *
 */

#include <string.h>

#include "btl.h"
#include "target.h"
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

static uint32_t btl_get_u32(btl_packet_t *pkt)
{
	return ((uint32_t )pkt->data[0]) | ((uint32_t)pkt->data[1] << 8) |
		((uint32_t)pkt->data[2] << 16) | ((uint32_t)pkt->data[3] << 24);
}

/*
 * Command handlers
 */
static int btl_cmd_info(btl_if_t *bi)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;
	int size = sizeof(BTL_VERSION_STR);

	memcpy(pkt->data, BTL_VERSION_STR, size);
	return size;
}

static int btl_cmd_read(btl_if_t *bi)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;
	return flash_read(pkt->addr, pkt->data, BTL_MAX_DATA_SIZE);
}

static int btl_cmd_write(btl_if_t *bi)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;

	if (btl_area(pkt->addr, pkt->size))
		return -1;

	if (flash_write(pkt->addr, pkt->data, pkt->size) < 0)
		return -1;

	return 0;
}

static int btl_cmd_verify(btl_if_t *bi)
{
	uint8_t data[BTL_MAX_DATA_SIZE];
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;

	if (btl_area(pkt->addr, pkt->size))
		return -1;

	if (flash_read(pkt->addr, data, pkt->size) < 0)
		return -1;

	if (memcmp(pkt->data, data, pkt->size))
		return -1;

	return 0;
}

static int btl_cmd_erase(btl_if_t *bi)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;
	uint32_t size = btl_get_u32(pkt);

	if (size == 0)
		return 0;

	if (btl_area(pkt->addr, size))
		return -1;

	if (flash_erase(pkt->addr, size) < 0)
		return -1;

	return 0;
}

static int btl_cmd_baud(btl_if_t *bi)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;
	uint32_t baud = btl_get_u32(pkt);

	switch (baud) {
		case 1000000:
		case  921600:
		case  576000:
		case  500000:
		case  460800:
		case  230400:
		case  115200:
		case   57600:
			break;
		default:
			return -1;
	}
	return 0;
}

static int btl_cmd_reset(btl_if_t *bi)
{
	bi->reset = 1;
	return 0;
}

int btl_handle_packet(btl_if_t *bi)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;
	uint8_t crc = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));
	int sz = -1;

	if (crc != btl_packet_crc(pkt))
		return -1;

	switch (pkt->cmd) {
		case BTL_CMD_INFO:
			sz = btl_cmd_info(bi);
			break;
		case BTL_CMD_READ:
			sz = btl_cmd_read(bi);
			break;
		case BTL_CMD_WRITE:
			sz = btl_cmd_write(bi);
			break;
		case BTL_CMD_ERASE:
			sz = btl_cmd_erase(bi);
			break;
		case BTL_CMD_VERIFY:
			sz = btl_cmd_verify(bi);
			break;
		case BTL_CMD_RESET:
			sz = btl_cmd_reset(bi);
			break;
		case BTL_CMD_BAUD:
			sz = btl_cmd_baud(bi);
			break;
		default:
			break;
	}

	if (sz < 0)
		btl_make_header(pkt, BTL_STATUS_ERROR, 0);
	else
		btl_make_header(pkt, BTL_STATUS_OK, sz);

	btl_packet_crc(pkt) = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));
	return btl_size_pkt(pkt);
}

int btl_read_byte(btl_if_t *bi, char c)
{
	btl_packet_t *pkt = (btl_packet_t *)bi->buf;

	bi->buf[bi->len++] = c;

	if (bi->len == 1) {
		/* prefix */
		if (c != BTL_PKT_PREXIX)
			bi->len = 0;
		return 0;
	}

	if (bi->len < sizeof(btl_packet_t)) /* header */
		return 0;

	if (pkt->size > BTL_MAX_DATA_SIZE) {
		/* Invalid packet size */
		bi->len = 0;
		return 0;
	}

	if (bi->len < btl_size_pkt(pkt))
		return 0;

	/* packet complete */
	return 1;
}

