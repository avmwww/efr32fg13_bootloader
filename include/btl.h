/*
 *
 */

#ifndef _BTL_H_
#define _BTL_H_

#include <stdint.h>

#include "btlproto.h"

typedef struct btl_if_s {
	int reset;
	unsigned int baud;
	uint8_t buf[BTL_MAX_PKT_SIZE];
	unsigned int len;
} btl_if_t;

int btl_handle_packet(btl_if_t *bi);

int btl_read_byte(btl_if_t *bi, char c);


#endif
