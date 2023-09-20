/*
 *
 */

#include <stdint.h>

#define VTOR		(*(volatile uint32_t *)0xE000ED08)

__attribute__((__noreturn__)) void Reset_Handler(void);

extern const uint32_t __Vectors[];

__attribute__((__section__(".start"))) void start(void)
{
	VTOR = (uint32_t)__Vectors & ~0x7f;
	Reset_Handler();
}

