/*
 *
 */

#ifndef _BTLCTL_H_
#define _BTLCTL_H_

#include <stdint.h>
#include "btlproto.h"

int btl_write(int fd, uint8_t cmd, uint8_t status, uint32_t addr,
		const void *data, unsigned int len);

int btl_read(int fd, uint8_t *cmd, uint8_t *status, uint32_t *addr, void *data);

#endif
