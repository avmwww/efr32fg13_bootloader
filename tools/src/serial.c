/*
 *
 */

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void serial_close(int fd)
{
	if (fd < 0)
		return;
	close(fd);
}

int serial_open(const char *path)
{
	int fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd < 0)
		return -1;

	fcntl(fd, F_SETFL, 0);
	return fd;
}

int serial_setup(int fd, unsigned int baud)
{
	struct termios t;
	speed_t speed;

	if (tcgetattr(fd, &t) < 0)
		return -1;

	cfmakeraw(&t);

	/* Enable receiver, ignore RTS/CTS */
	t.c_cflag |= CREAD | CLOCAL;


	/* read with timeout */
	t.c_cc[VMIN]  = 0;
	/* timeout in diciseconds for read */
	t.c_cc[VTIME] = 10;

	if (baud <= 57600)
		speed = B57600;
	else if (baud <= 115200)
		speed = B115200;
	else if (baud <= 230400)
		speed = B230400;
	else if (baud <= 460800)
		speed = B460800;
	else if (baud <= 500000)
		speed = B500000;
	else if (baud <= 576000)
		speed = B576000;
	else if (baud <= 921600)
		speed = B921600;
	else if (baud <= 1000000)
		speed = B1000000;
	else
		speed = B115200;

	cfsetspeed(&t, speed);

	return tcsetattr(fd, TCSAFLUSH, &t);
}

int serial_read(int fd, void *buf, unsigned int len)
{
	return read(fd, buf, len);
}

int serial_write(int fd, const void *buf, unsigned int len)
{
	return write(fd, buf, len);
}

