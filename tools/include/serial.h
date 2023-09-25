/*
 *
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

int serial_open(const char *path);

void serial_close(int fd);

int serial_setup(int fd, unsigned int baud);

int serial_write(int fd, const void *buf, unsigned int len);

int serial_read(int fd, void *buf, unsigned int len);

#endif
