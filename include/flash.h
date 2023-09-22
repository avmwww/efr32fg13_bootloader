/*
 *
 */

#ifndef _FLASH_H_
#define _FLASH_H_

#include <string.h>

int flash_erase(unsigned int addr, unsigned int len);

int flash_write(unsigned int addr, const void *data, unsigned int len);

static inline int flash_read(unsigned int addr, void *buf, unsigned int len)
{
	memcpy(buf, (void *)addr, len);
	return len;
}

#endif
