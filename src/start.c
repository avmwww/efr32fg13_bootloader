/*
 * Bootloader for Silicon Labs erf32fg13 device
 *
 * Author
 * 2024  Andrey Mitrofanov <avmwww@gmail.com>
 *
 * Startup entry
 */

#include <stdint.h>
#include <em_core.h>

//#include "silabs.h"
//#include "first_stage_main.h"

extern const uint32_t __Vectors[];

__attribute__((__section__(".start"))) __attribute__((__noreturn__))
void start(void)
{
	uint32_t *reset_vector = (uint32_t *)__Vectors;

	*(uint32_t *)0x400e0000 = 1;

	SCB->VTOR = (uint32_t)reset_vector;
	asm volatile ("movs  r3, %[reset_vector]\n"
                      "ldr.w sp, [r3]\n"
                      "ldr   r2, [r3, #4]\n"
                      "bx r2\n"
                      : /* no outputs */
                      : [reset_vector] "r" (reset_vector)
                      : /* no clobbers */
        );

	for(;;);
}

/*
 * \brief run main app
 */
__attribute__((__noreturn__)) void boot_app(void)
{
	uint32_t *reset_vector = (uint32_t *)0;

	SCB->VTOR = (uint32_t)reset_vector;
	asm volatile ("movs r3, %[reset_vector]\n"
                      "ldr.w sp, [r3]\n"
                      "ldr   r2, [r3, #4]\n"
                      "bx r2\n"
                      : /* no outputs */
                      : [reset_vector] "r" (reset_vector)
                      : /* no clobbers */
        );
	for(;;);
}
