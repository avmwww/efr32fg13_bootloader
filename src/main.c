/*
 *
 */

#include <stdlib.h>
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
#include "btlproto.h"

#define USART_CTRL_PORT		0
#define USART_BUF_LEN		256
#define USART_BAUD_RATE		115200

//extern const uint32_t __btl_info_start__;
#define __btl_info_start__		((void *)0x2000fff0)

struct btl_info_s {
	uint32_t magic;
};

#define BTL_MAGIC		0xe2e4


struct bootloader_s {
	struct timer *timer;
	struct btl_info_s *info;
	void *cmd_buf;
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

#define CMD_BUF_LEN		256

static void system_failure(void)
{
	for (;;) {
		led_red_toggle();
		timer_sleep_ms(250);
	}
}

static void system_init(struct bootloader_s *bt)
{
	bt->info = (struct btl_info_s *)__btl_info_start__;
	bt->cmd_buf = malloc(CMD_BUF_LEN);

	if (!bt->cmd_buf)
		system_failure();

	usart_init(USART_CTRL_PORT, USART0_BUF_LEN, USART0_BUF_LEN);
	usart_set_baudrate(USART_CTRL_PORT, USART_BAUD_RATE);

	bt->timer = timer_create();

	timer_add(bt->timer, led_timer, bt, TIMER_MS(500), true);
}

__attribute__((__noreturn__)) void boot_app(void);

static void system_run(struct bootloader_s *bt)
{
	uint32_t magic = bt->info->magic;
	int len;

	usart_puts(USART_CTRL_PORT, "\r\n" BTL_VERSION_STR "\r\n");

	usart_puts(USART_CTRL_PORT, "Reboot cause: ");
	if (magic == BTL_MAGIC || boot_pin() == 0) {
		usart_puts(USART_CTRL_PORT, "soft");
		bt->info->magic = 0;
	} else {
		usart_puts(USART_CTRL_PORT, "hard\r\n");
		timer_sleep_ms(10);
		/* boot application */
		boot_app();
	}

	usart_puts(USART_CTRL_PORT, "\r\n");

	led_red_on();
	led_green_off();
	usart_rx_enable(USART_CTRL_PORT);

	len = 0;
	for (;;) {
		int c = usart_read(USART_CTRL_PORT);

		if (c >= 0) {
			len = btl_read_byte(c, bt->cmd_buf, len);
			if (len == 256) {
				/* complete */
				len = btl_handle_packet(bt->cmd_buf);
				if (len > 0)
					usart_write_buf(USART_CTRL_PORT, bt->cmd_buf, len);

				len = 0;
			}

		}

		timer_handle(bt->timer);
	}
}

int main(void)
{
	struct bootloader_s *bt;

	/* Chip errata */
	CHIP_Init();

	target_init();

	bt = malloc(sizeof(struct bootloader_s));
	if (!bt)
		system_failure();

	memset(bt, 0, sizeof(struct bootloader_s));

	system_init(bt);

	system_run(bt);
}

