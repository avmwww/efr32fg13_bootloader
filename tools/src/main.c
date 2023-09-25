/*
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "failure.h"
#include "progopt.h"
#include "serial.h"
#include "btlctl.h"

//#include "debug.h"

#define XINTSTR(s)		INTSTR(s)
#define INTSTR(s)		#s

#define BTLCTL_DEVICE_DEFAULT		"/dev/ttyUSB0"
#define BAUD_RATE_DEFAULT		115200

struct btlctl_conf {
	char *dev;
	unsigned int baud;
	int fd;
	int info;
	int help;
	char *flash;
	int addr;
	int skip;
};

#define BTLCTL_OPT(s, l, d, t, o, v) \
		PROG_OPT(s, l, d, t, struct btlctl_conf, o, v)

#define BTLCTL_OPT_NO(s, l, d, o, v)		BTLCTL_OPT(s, l, d, OPT_NO, o, v)
#define BTLCTL_OPT_INT(s, l, d, o)		BTLCTL_OPT(s, l, d, OPT_INT, o, 0)
#define BTLCTL_OPT_STR(s, l, d, o)		BTLCTL_OPT(s, l, d, OPT_STRING, o, 0)

static struct prog_option btlctl_options[] = {
	BTLCTL_OPT_NO('h', "help", "help usage", help, 1),
	BTLCTL_OPT_NO('i', "info", "bootloader info", info, 1),
	BTLCTL_OPT_STR('d', "device", "serial device, default " BTLCTL_DEVICE_DEFAULT, dev),
	BTLCTL_OPT_INT('b', "baud", "baud rate, default " XINTSTR(BAUD_RATE_DEFAULT), baud),
	BTLCTL_OPT_STR('f', "flash", "flash binary file", flash),
	BTLCTL_OPT_INT('a', "addr", "address of flash offset, default 0", addr),
	BTLCTL_OPT_NO('s', "skip", "skip erase of flash", skip, 1),
	PROG_END,
};

#define OPT_LEN		(sizeof(btlctl_options) / sizeof(btlctl_options[0]))

static void usage(char *prog, struct prog_option *opt)
{
	fprintf(stderr, "Usage: %s [options]\n", prog);
	prog_option_printf(stderr, opt);
	exit(EXIT_FAILURE);
}

static int btl_transfer(int fd, uint8_t cmd, uint32_t addr,
		void *out, int out_len, void *in)
{
	uint8_t c, sz, st;
	uint32_t a;

	if (btl_write(fd, cmd, 0, addr, out, out_len) < 0)
		return -1;

	if ((sz = btl_read(fd, &c, &st, &a, in)) < 0)
		return -1;

	if ((c & 0x7f) != cmd)
		return -1;

	if (st != BTL_STATUS_OK)
		return -1;

	return sz;
}

static void bootloader_info(struct btlctl_conf *cfg)
{
	char info[256];
	int len;

	if ((len = btl_transfer(cfg->fd, BTL_CMD_INFO, 0, NULL, 0, info)) < 0)
		failure(errno, "Request bootloader information failed");

	printf("Bootloader information:\n");
	info[len] = '\0';
	printf("%s\n", info);
}

static void flash_erase(struct btlctl_conf *cfg, int len)
{
	uint8_t buf[4];

	/* Erase flash area */
	printf("Erasing flash at address 0x%x, %d bytes ... ", cfg->addr, len);
	fflush(stdout);
	fflush(stderr);
	/* address */
	buf[0] = (uint8_t)len;
	buf[1] = (uint8_t)(len >> 8);
	buf[2] = (uint8_t)(len >> 16);
	buf[3] = (uint8_t)(len >> 24);
	if (btl_transfer(cfg->fd, BTL_CMD_ERASE, cfg->addr, buf, 4, NULL) < 0)
		failure(errno, "\nFlash erase failed");

	printf("Done\n");
}

static void flash_file(struct btlctl_conf *cfg)
{
	int len, sz;
	uint8_t buf[BTL_MAX_DATA_SIZE];
	uint32_t addr;
	int bytes;
	struct stat stat;
	int fd = open(cfg->flash, O_RDONLY);

	if (fd < 0)
		failure(errno, "Can't open flash file %s", cfg->flash);

	if (fstat(fd, &stat) < 0)
		failure(errno, "Can't get file %s size", cfg->flash);

	len = stat.st_size;

	if (!cfg->skip)
		flash_erase(cfg, len);

	addr = cfg->addr;

	printf("Start programm %d bytes\n", len);
	while (len > 0) {
		sz = read(fd, buf, sizeof(buf));
		if (sz <= 0)
			break;

		if (btl_transfer(cfg->fd, BTL_CMD_WRITE, addr, buf, sz, NULL) < 0)
			failure(errno, "\nFlash write failed");

		addr += sz;
		len -= sz;
		bytes += sz;
		printf("%ld %%\r", (bytes * 100) / stat.st_size);
		//printf("+");
		fflush(stdout);
		fflush(stderr);
	}
	printf("\nDone\n");

	printf("Reseting system ... ");
	btl_write(cfg->fd, BTL_CMD_RESET, 0, 0, NULL, 0);
	printf("\nDone\n");
}

int main(int argc, char **argv)
{
	struct option opt[OPT_LEN + 1];
	char optstr[2 * OPT_LEN + 1];
	struct btlctl_conf conf;

	memset(&conf, 0, sizeof(struct btlctl_conf));

	/* set default values */
	conf.dev = BTLCTL_DEVICE_DEFAULT;
	conf.baud = BAUD_RATE_DEFAULT;

	if (prog_option_make(btlctl_options, opt, optstr, OPT_LEN) < 0)
		failure(0, "Invalid options");

	if (prog_option_load(argc, argv, btlctl_options, opt, optstr, &conf) < 0)
		usage(argv[0], btlctl_options);

	if (conf.help)
		usage(argv[0], btlctl_options);

	if ((conf.fd = serial_open(conf.dev)) < 0)
		failure(errno, "Can't open serial port %s", conf.dev);

	if (serial_setup(conf.fd, conf.baud) < 0)
		failure(errno, "Can't set serial port %s parameters", conf.dev);

	if (conf.info)
		bootloader_info(&conf);

	if (conf.flash)
		flash_file(&conf);

	serial_close(conf.fd);
	exit(EXIT_SUCCESS);
}

