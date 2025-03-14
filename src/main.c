/*
 * Bootloader for Silicon Labs erf32fg13 device
 *
 * Author
 * 2024  Andrey Mitrofanov <avmwww@gmail.com>
 *
 * Main
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

#define CMD_BUF_LEN		256

#define USART_CTRL_PORT		0
#define USART_BUF_LEN		512
/* Deafult baud rate for PC side */
#define USART0_BAUD_RATE		115200
/* Deafult baud rate for FC side */
#define USART1_BAUD_RATE		420000

static const uint32_t usart_baud_rate_default[] = {
	USART0_BAUD_RATE,
	USART1_BAUD_RATE,
};

__attribute__((__noreturn__)) void boot_app(void);


struct bootloader_s {
	struct timer *timer;
	struct btl_info_s *info;
	struct boot_port {
		uint32_t baud;
		uint32_t last_time;
		btl_if_t iface;
	} usart[USART_NUM];
	uint32_t clock;
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

/*
 * \brief print in hex format number \d on usart \num.
 * \param num usart number 0 or 1.
 * \param d decimal number.
 */
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

/*
 * \brief print in decimal format number \d on usart \num.
 * \param num usart number 0 or 1.
 * \param d decimal number.
 */
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

static void system_failure(void)
{
	for (;;) {
		led_red_toggle();
		timer_sleep_ms(250);
	}
}

/*
 * \brief Initalize system.
 * \param bt bootloader runtime context.
 */
static void system_init(struct bootloader_s *bt)
{
	int i;

	bt->info = (struct btl_info_s *)__btl_info_start__;

	for (i = 0; i < USART_NUM; i++) {
		usart_init(i, USART0_BUF_LEN, USART0_BUF_LEN);
		usart_set_baudrate(i, bt->usart[i].baud);
		bt->usart[i].baud = usart_get_baudrate(i);
	}
	bt->clock = SystemCoreClockGet();

	bt->timer = timer_create();

	timer_add(bt->timer, led_timer, bt, TIMER_MS(500), true);
}

static void usart_handle(struct bootloader_s *bt, int port)
{
	int len;
	int err;
	int c = usart_read(port);
	uint32_t ms = timer_get_ms();
	struct boot_port *bp = &bt->usart[port];

	if (c < 0) {
		if ((ms - bp->last_time) > 1) {
			/* reset input bytes by 1 mS timeout */
			bp->last_time = ms;
			bp->iface.len = 0;
		}
		return;
	}
	bp->last_time = ms;

	err = btl_read_byte(&bp->iface, c);
	if (err < 0) {
		bp->iface.len = 0;
		return;
	}

	if (err == 0)
		return;

	/* complete */
	len = btl_handle_packet(&bp->iface);
	if (len > 0)
		usart_write_buf(port, bp->iface.buf, len);

	if (bp->iface.reset) {
		/* system reset requested */
		timer_sleep_ms(20);
		__NVIC_SystemReset();
	}

	if (bp->iface.baud) {
		/* change baud rate requested */
		timer_sleep_ms(20);
		usart_set_baudrate(port, bp->iface.baud);
	}

	bp->iface.baud = 0;
	bp->iface.reset = 0;
	bp->iface.len = 0;
}

static void usart_handle_all(struct bootloader_s *bt)
{
	int i;

	for (i = 0; i < USART_NUM; i++)
		usart_handle(bt, i);
}

static void usart_puts_all(const char *str)
{
	int i;

	for (i = 0; i < USART_NUM; i++)
		usart_puts(i, str);
}

/*
 * \brief print information of bootloader on usart
 */
static void btl_print_info(struct bootloader_s *bt, int port)
{
	uint64_t id;

	id = taget_get_id();
	usart_puts(port, "\r\n" BTL_VERSION_STR "\r\n");
	usart_puts(port, "DEV ID: ");
	usart_puth(port, id);
	usart_puts(port, "\r\n");

	usart_puts(port, "CLOCK: ");
	usart_putd(port, bt->clock);
	usart_puts(port, ", ");
	usart_puts(port, "BAUD: ");
	usart_putd(port, bt->usart[port].baud);
	usart_puts(port, "\r\n");
}

static void btl_print_info_all(struct bootloader_s *bt)
{
	int i;

	for (i = 0; i < USART_NUM; i++)
		btl_print_info(bt, i);
}

/*
 * \brief copying the flash memory used for temporary recording
 *        of the application to the main area.
 *        No checksums are used.
 *        It is assumed that the bootloader emergency mode will
 *        be activated at startup if the button is pressed
 */
static int btl_flash_app(struct bootloader_s *bt)
{
	(void)bt;

	if (flash_erase(0, BTL_APP_SIZE) < 0)
		return -1;

	if (flash_write(0, (void *)BTL_FLASH_APP_ADDR, BTL_APP_SIZE) < 0)
		return -1;

	return 0;
}

static void btl_usart_enable(struct bootloader_s *bt)
{
	int i;
	(void)bt;

	for (i = 0; i < USART_NUM; i++)
		usart_rx_enable(i);
}

/*
 * \brief  main system process.
 * \param bt bootloader runtime context.
 */
static void system_run(struct bootloader_s *bt)
{
	uint32_t magic = bt->info->magic;

	btl_print_info_all(bt);

	usart_puts_all("Reboot cause: ");
	if (magic == BTL_MAGIC || boot_pin() == 0) {
		usart_puts_all("soft");
		bt->info->magic = 0;
		if (bt->info->target == BTL_FLASH_APP) {
			bt->info->target = 0;
			usart_puts_all(", flash app\r\n");
			if (btl_flash_app(bt) == 0) {
				timer_sleep_ms(30);
				boot_app();
			}
		}
	} else {
		usart_puts_all("hard\r\n");
		timer_sleep_ms(30);
		/* boot application */
		boot_app();
	}

	usart_puts_all("\r\n");

	led_red_on();
	led_green_off();

	btl_usart_enable(bt);

	for (;;) {
		usart_handle_all(bt);

		timer_handle(bt->timer);
	}
}

/*
 * main
 */
int main(void)
{
	struct bootloader_s *bt;
	int i;

	/* Chip errata */
	CHIP_Init();

	target_init();

	bt = malloc(sizeof(struct bootloader_s));
	if (!bt)
		system_failure();

	memset(bt, 0, sizeof(struct bootloader_s));

	for (i = 0; i < USART_NUM; i++)
		bt->usart[i].baud = usart_baud_rate_default[i];

	system_init(bt);

	system_run(bt);
}

