/*
 *
 */

#ifndef _BTLCTL_H_
#define _BTLCTL_H_

#include <stdint.h>

#ifdef __MINGW32__
# include "serialport.h"
#else
# include "serial.h"
#endif

#include "btlproto.h"

int btl_write(serial_handle fd, uint8_t cmd, uint8_t status, uint32_t addr,
		const void *data, unsigned int len);

int btl_read(serial_handle fd, uint8_t *cmd, uint8_t *status, uint32_t *addr, void *data);

#endif
