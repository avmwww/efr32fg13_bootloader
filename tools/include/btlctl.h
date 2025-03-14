/*
 * Console tool for bootloader for Silicon Labs erf32fg13 device
 *
 * Author
 * 2024  Andrey Mitrofanov <avmwww@gmail.com>
 *
 */

#ifndef _BTLCTL_H_
#define _BTLCTL_H_

#include <stdint.h>

#include "serial.h"

#include "btlproto.h"

#ifdef DEBUG
# define dbg			printf
# define dbg_dump_hex		dump_hex
#else
# define dbg(...)
# define dbg_dump_hex(...)
#endif


int btl_write(serial_handle fd, uint8_t cmd, uint8_t status, uint32_t addr,
		const void *data, unsigned int len);

int btl_read(serial_handle fd, uint8_t *cmd, uint8_t *status, uint32_t *addr, void *data);

#endif
