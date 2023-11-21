/*
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "btlproto.h"

#ifdef __MINGW32__
#include "serialport.h"
#else
#include "serial.h"
#endif

#include "dump_hex.h"

#ifdef DEBUG
# define dbg			printf
# define dbg_dump_hex		dump_hex
#else
# define dbg(...)
# define dbg_dump_hex(...)
#endif

static uint8_t crc8_cal_buf(const void *data, int len)
{
	int i;
	uint8_t crc = 0;
	const uint8_t *p = data;

	for (i = 0; i < len; i++)
		crc = crc8_calc(crc, *p++, BTL_CRC_POLY);

	return crc;
}

static void btl_dump_pkt(const char *prefix, const btl_packet_t *pkt)
{
	dbg("=%s=\n", prefix);
	dbg("Prefix  : %02x(%c)\n", pkt->prefix, pkt->prefix);
	dbg("Size    : %02x(%d)\n", pkt->size, pkt->size);
	dbg("Command : %02x\n", pkt->cmd);
	dbg("Status  : %02x\n", pkt->status);
	dbg("Address : %08x\n", pkt->addr);
	dbg("CRC     : %02x\n", btl_packet_crc(pkt));
}

int btl_write(serial_handle fd, uint8_t cmd, uint8_t status, uint32_t addr,
		const void *data, unsigned int len)
{
	uint8_t buf[BTL_MAX_PKT_SIZE];
	btl_packet_t *pkt = (btl_packet_t *)buf;

	pkt->prefix = BTL_PKT_PREXIX;
	pkt->size = len;
	pkt->cmd = cmd;
	pkt->status = 0;
	pkt->addr = addr;

	if (len > BTL_MAX_DATA_SIZE)
		return -1;

	if (data && len)
		memcpy(pkt->data, data, len);

	btl_packet_crc(pkt) = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));

	btl_dump_pkt("TX", pkt);
	dbg_dump_hex(pkt, btl_size_pkt(pkt), 0);

	return serial_write(fd, pkt, btl_size_pkt(pkt));
}

static int btl_read_byte(serial_handle fd, void *buf, unsigned int len)
{
	uint8_t c, crc;
	uint8_t *p = buf;
	btl_packet_t *pkt = buf;
	int err;

	if ((err = serial_read(fd, &c, 1)) != 1)
		return err;

	p[len] = c;
	dbg("R: %02x\n", c);

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

	if (len < btl_size_pkt(pkt))
		return len;

	/* packet complete */
	btl_dump_pkt("RX", pkt);
	dbg_dump_hex(pkt, btl_size_pkt(pkt), 0);

	crc = crc8_cal_buf(btl_start_crc(pkt), btl_size_crc(pkt));
	if (btl_packet_crc(pkt) != crc) {
		dbg("CRC %02x invalid, calc %02x\n", btl_packet_crc(pkt), crc);
		return 0;
	}

	return 256;
}

int btl_read(serial_handle fd, uint8_t *cmd, uint8_t *status, uint32_t *addr, void *data)
{
	int len;
	int fail;
	int sz;
	uint8_t buf[BTL_MAX_PKT_SIZE];
	btl_packet_t *pkt = (btl_packet_t *)buf;

	sz = len = 0;
	fail = 0;
	while ((sz = btl_read_byte(fd, buf, len)) != 256) {
		if (sz < 0)
			return sz;

		len = sz;
		if (sz == 0) {
			if (++fail > 3)
				return -1;
		}
	}

	if (data)
		memcpy(data, pkt->data, pkt->size);
	if (cmd)
		*cmd = pkt->cmd;
	if (status)
		*status = pkt->status;
	if (addr)
		*addr = pkt->addr;

	return pkt->size;
}

