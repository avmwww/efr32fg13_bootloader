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
#include "flash.h"
#include "btl.h"

#define USART_CTRL_PORT		0
#define USART_BUF_LEN		512
#define USART_BAUD_RATE		115200

__attribute__((__noreturn__)) void boot_app(void);


struct bootloader_s {
	struct timer *timer;
	struct btl_info_s *info;
	struct {
		uint32_t baud;
		int num;
		uint32_t last_time;
	} usart;
	uint32_t clock;
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

static void usart_puth(int num, uint64_t d)
{
	char buf[20];
	const char *hextab = "0123456789ABCDEF";
	int i;

	buf[0] = '0';
	buf[1] = '\0';

	i = 0;
	while (d) {
		buf[i] = hextab[d & 0xf];
		i++;
		d >>= 4;
	}

	if (i == 0) {
		usart_puts(num, buf);
	} else {
		while (i)
			usart_write(num, buf[--i]);
	}
}

static void usart_putd(int num, unsigned int d)
{
	char buf[16];
	int i;

	buf[0] = '0';
	buf[1] = '\0';

	/* reverse digits */
	i = 0;
	while (d) {
		buf[i] = (d % 10) + '0';
		i++;
		d /= 10;
	}
	if (i == 0) {
		usart_puts(num, buf);
	} else {
		while (i) {
			usart_write(num, buf[i - 1]);
			i--;
		}
	}
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

	usart_init(bt->usart.num, USART0_BUF_LEN, USART0_BUF_LEN);
	usart_set_baudrate(bt->usart.num, bt->usart.baud);
	bt->usart.baud = usart_get_baudrate(bt->usart.num);
	bt->clock = SystemCoreClockGet();

	bt->timer = timer_create();

	timer_add(bt->timer, led_timer, bt, TIMER_MS(500), true);
}

static void usart_handle(struct bootloader_s *bt)
{
	int len;
	int err;
	int c = usart_read(bt->usart.num);
	uint32_t ms = timer_get_ms();

	if (c < 0) {
		if ((ms - bt->usart.last_time) > 1) {
			/* reset input bytes by 1 mS timeout */
			bt->usart.last_time = ms;
			bt->iface.len = 0;
		}
		return;
	}
	bt->usart.last_time = ms;

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
		usart_write_buf(bt->usart.num, bt->iface.buf, len);

	if (bt->iface.reset) {
		/* system reset requested */
		timer_sleep_ms(20);
		__NVIC_SystemReset();
	}

	if (bt->iface.baud) {
		/* change baud rate requested */
		timer_sleep_ms(20);
		usart_set_baudrate(bt->usart.num, bt->iface.baud);
	}

	bt->iface.baud = 0;
	bt->iface.reset = 0;
	bt->iface.len = 0;
}

static void btl_print_info(struct bootloader_s *bt)
{
	uint64_t id;

	id = taget_get_id();
	usart_puts(bt->usart.num, "\r\n" BTL_VERSION_STR "\r\n");
	usart_puts(bt->usart.num, "DEV ID: ");
	usart_puth(bt->usart.num, id);
	usart_puts(bt->usart.num, "\r\n");

	usart_puts(bt->usart.num, "CLOCK: ");
	usart_putd(bt->usart.num, bt->clock);
	usart_puts(bt->usart.num, ", ");
	usart_puts(bt->usart.num, "BAUD: ");
	usart_putd(bt->usart.num, bt->usart.baud);
	usart_puts(bt->usart.num, "\r\n");
}

static int btl_flash_app(struct bootloader_s *bt)
{
	if (flash_erase(0, BTL_APP_SIZE) < 0)
		return -1;

	if (flash_write(0, BTL_FLASH_APP_ADDR, BTL_APP_SIZE) < 0)
		return -1;

	return 0;
}

static void system_run(struct bootloader_s *bt)
{
	uint32_t magic = bt->info->magic;

	btl_print_info(bt);

	usart_puts(bt->usart.num, "Reboot cause: ");
	if (magic == BTL_MAGIC || boot_pin() == 0) {
		usart_puts(bt->usart.num, "soft");
		bt->info->magic = 0;
		if (bt->info->target == BTL_FLASH_APP) {
			bt->info->target = 0;
			usart_puts(bt->usart.num, ", flash app\r\n");
			if (btl_flash_app(bt) == 0) {
				timer_sleep_ms(30);
				boot_app();
			}
		}
	} else {
		usart_puts(bt->usart.num, "hard\r\n");
		timer_sleep_ms(30);
		/* boot application */
		boot_app();
	}

	usart_puts(bt->usart.num, "\r\n");

	led_red_on();
	led_green_off();
	usart_rx_enable(bt->usart.num);

	for (;;) {
		usart_handle(bt);

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

	bt->usart.num = USART_CTRL_PORT;
	bt->usart.baud = USART_BAUD_RATE;

	system_init(bt);

	system_run(bt);
}

