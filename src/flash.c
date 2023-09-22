/*
 *
 */

#include <stdint.h>
#include <em_msc.h>

#define PAGE_SIZE		2048

int flash_erase(unsigned int addr, unsigned int len)
{
	addr &= ~(PAGE_SIZE - 1);

	while (len) {
		if (MSC_ErasePage((uint32_t *)addr) != mscReturnOk)
			return -1;
		if (len > PAGE_SIZE)
			len -= PAGE_SIZE;
		else
			len = 0;

		addr += PAGE_SIZE;
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


