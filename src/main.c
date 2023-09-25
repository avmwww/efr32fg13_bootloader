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
#include "btl.h"

#define USART_CTRL_PORT		0
#define USART_BUF_LEN		256
#define USART_BAUD_RATE		115200

__attribute__((__noreturn__)) void boot_app(void);

//extern const uint32_t __btl_info_start__;
#define __btl_info_start__		((void *)0x2000fff0)

struct btl_info_s {
	uint32_t magic;
};

#define BTL_MAGIC		0xe2e4


struct bootloader_s {
	struct timer *timer;
	struct btl_info_s *info;
	btl_if_t iface;
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

	usart_init(USART_CTRL_PORT, USART0_BUF_LEN, USART0_BUF_LEN);
	usart_set_baudrate(USART_CTRL_PORT, USART_BAUD_RATE);

	bt->timer = timer_create();

	timer_add(bt->timer, led_timer, bt, TIMER_MS(500), true);
}

static void usart_hanle(struct bootloader_s *bt)
{
	int c = usart_read(USART_CTRL_PORT);
	int len;
	int err;

	if (c < 0)
		return;

	err = btl_read_byte(&bt->iface, c);
	if (err < 0) {
		bt->iface.len = 0;
		return;
	}

	if (err == 0)
		return;

	/* complete */
	len = btl_handle_packet(&bt->iface);
	if (len > 0)
		usart_write_buf(USART_CTRL_PORT, bt->iface.buf, len);

	if (bt->iface.reset) {
		timer_sleep_ms(20);
		__NVIC_SystemReset();
	}

	if (bt->iface.baud) {
		timer_sleep_ms(20);
		usart_set_baudrate(USART_CTRL_PORT, bt->iface.baud);
	}

	bt->iface.baud = 0;
	bt->iface.reset = 0;
	bt->iface.len = 0;
}

static void system_run(struct bootloader_s *bt)
{
	uint32_t magic = bt->info->magic;

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

	for (;;) {
		usart_hanle(bt);

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

