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

static uint32_t btl_flash_size(btl_packet_t *pkt)
{
	return ((uint32_t )pkt->data[0]) | ((uint32_t)pkt->data[1] << 8) |
		((uint32_t)pkt->data[2] << 16) | ((uint32_t)pkt->data[3] << 24);
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
	uint32_t size = btl_flash_size(pkt);

	if (size == 0)
		return 0;

	if (btl_area(pkt->addr, size))
		return -1;

	if (flash_erase(pkt->addr, size) < 0)
		return -1;

	return 0;
}

static int btl_cmd_reset(btl_packet_t *pkt)
{
	(void)pkt;
	__NVIC_SystemReset();
	return 0;
}

int btl_handle_packet(void *buf)
{
	btl_packet_t *pkt = buf;
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
		case BTL_CMD_RESET:
			sz = btl_cmd_reset(pkt);
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

int btl_read_byte(char c, void *buf, unsigned int len)
{
	uint8_t *p = buf;
	btl_packet_t *pkt = buf;

	p[len] = c;

	if (len == 0) {
		/* prefix */
		if (c != BTL_PKT_PREXIX)
			return 0;
		return 1;
	}

	len++;
	if (len < sizeof(btl_packet_t)) {
		/* header */
		return len;
	}

	if (pkt->size > BTL_MAX_DATA_SIZE)
		return 0;

	if (len < btl_size_pkt(pkt))
		return len;

	/* packet complete */
	return 256;
}

