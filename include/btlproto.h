/*
 *
 */

#ifndef _BTLPROTO_H_
#define _BTLPROTO_H_

#include <stddef.h>
#include <stdint.h>

/*
 * PACKET:
 * PREFIX          char '#'
 * SIZE            u8
 * CMD             u8
 * PAYLOAD <size>
 * CS              u8
 */
typedef struct btl_packet_s {
	uint8_t prefix;
	uint8_t size;
	uint8_t cmd;
	uint8_t status;
	uint32_t addr;
	uint8_t data[0];
} __attribute__((__packed__)) btl_packet_t;

#define BTL_HEADER_SIZE			sizeof(btl_packet_t)

#define btl_packet_crc(p)		((p)->data[(p)->size])
#define btl_start_crc(p)		(&(p)->cmd)
#define btl_size_crc(p)			((p)->size + (BTL_HEADER_SIZE - offsetof(btl_packet_t, size) - 1))
#define btl_size_pkt(p)			((p)->size + BTL_HEADER_SIZE + 1)

#define BTL_PKT_PREXIX		'#'
#define BTL_PKT_REPLY		0x80

#define BTL_CMD_INFO		0x00
#define BTL_CMD_ERASE		0x01
#define BTL_CMD_WRITE		0x02
#define BTL_CMD_READ		0x03
#define BTL_CMD_VERIFY		0x04
#define BTL_CMD_BAUD		0x05
#define BTL_CMD_RESET		0xff

#define BTL_STATUS_OK		0x00
#define BTL_STATUS_ERROR	0xff

#define BTL_CMD_ERROR		0x7f


#define BTL_MAX_DATA_SIZE	64
#define BTL_MAX_PKT_SIZE	(BTL_MAX_DATA_SIZE + BTL_HEADER_SIZE + 1)

static inline uint8_t crc8_calc(uint8_t crc, uint8_t a, uint8_t poly)
{
	int i;

	crc ^= a;
	for (i = 0; i < 8; i++) {
		if (crc & 0x80)
			crc = (crc << 1) ^ poly;
		else
			crc = crc << 1;
	}
	return crc;
}

#define BTL_CRC_POLY		0xd5

#define BTL_ADDR		0xfe10000
#define BTL_SIZE		0x4000

//extern const uint32_t __btl_info_start__;
#define __btl_info_start__		((void *)0x2000fff0)

struct btl_info_s {
	uint32_t magic;
} __attribute__((__packed__));

#define BTL_MAGIC		0xe2e4

#endif
