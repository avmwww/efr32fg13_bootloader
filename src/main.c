/*
 *
 */

#include <string.h>
#include <stdint.h>

#include <em_device.h>
#include <em_chip.h>
#include <em_emu.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_usart.h>

#include "target.h"
#include "usart.h"
#include "timer.h"

#define USART_BUF_LEN		256
#define USART_BAUD_RATE		115200

struct bootloader_s {
	struct timer *timer;
};

static int led_timer(void *arg)
{
	(void)arg;
	led_green_toggle();
	return 0;
}

static void usart_puts(int num, const char *str)
{
	usart_write_buf(num, str, strlen(str));
}

extern const uint32_t __btl_info_start__;

struct btl_info_s {
	uint32_t magic;
};

#define BTL_MAGIC		0xe2e4

int main(void)
{
	static uint8_t usart_buf[256];
	static struct bootloader_s bt;
	struct btl_info_s *bi = (struct btl_info_s *)__btl_info_start__;
	int len;

	/* Chip errata */
	CHIP_Init();

	target_init();

	usart_init(0, USART0_BUF_LEN, USART0_BUF_LEN);
	usart_set_baudrate(0, USART_BAUD_RATE);

	bt.timer = timer_create();

	timer_add(bt.timer, led_timer, &bt, TIMER_MS(500), true);

	usart_puts(0, "\r\nBTL V1.0\r\n");

	usart_puts(0, "Reboot cause: ");
	if (bi->magic == BTL_MAGIC)
		usart_puts(0, "soft");
	else
		usart_puts(0, "hard");

	usart_puts(0, "\r\n");

	bi->magic = BTL_MAGIC;

	while (1) {
		if ((len = usart_read_buf(0, usart_buf, 16)) > 0)
			usart_write_buf(0, usart_buf, len);

		timer_handle(bt.timer);
	}
}

