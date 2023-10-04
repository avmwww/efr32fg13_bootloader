/*
 *
 */

#include <stdint.h>
#include <em_msc.h>

int flash_erase(unsigned int addr, unsigned int len)
{
	addr &= ~(FLASH_PAGE_SIZE - 1);

	while (len) {
		if (MSC_ErasePage((uint32_t *)addr) != mscReturnOk)
			return -1;
		if (len > FLASH_PAGE_SIZE)
			len -= FLASH_PAGE_SIZE;
		else
			len = 0;

		addr += FLASH_PAGE_SIZE;
	}
	return 0;
}

int flash_write(unsigned int addr, const void *data, unsigned int len)
{
	msc_Return_TypeDef err;

	MSC_Init();
	err = MSC_WriteWord((uint32_t *)addr, data, len);
	MSC_Deinit();
	if (err != mscReturnOk)
		return -1;
	return len;
}


