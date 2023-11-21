/*
 *
 */

#include <em_msc.h>

#include "target.h"

int flash_erase(unsigned int addr, unsigned int len)
{
	int err = 0;

	addr &= ~(FLASH_PAGE_SIZE - 1);

	led_flash_on();
	while (len) {
		if (MSC_ErasePage((uint32_t *)addr) != mscReturnOk) {
			err = -1;
			break;
		}
		if (len > FLASH_PAGE_SIZE)
			len -= FLASH_PAGE_SIZE;
		else
			len = 0;

		addr += FLASH_PAGE_SIZE;
	}
	led_flash_off();
	return err;
}

int flash_write(unsigned int addr, const void *data, unsigned int len)
{
	msc_Return_TypeDef err;

	led_flash_on();
	MSC_Init();
	err = MSC_WriteWord((uint32_t *)addr, data, len);
	MSC_Deinit();
	led_flash_off();

	if (err != mscReturnOk)
		return -1;
	return len;
}


