/*
 *
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

typedef int serial_handle;

serial_handle serial_open(const char *path);

void serial_close(serial_handle fd);

int serial_setup(serial_handle fd, unsigned int baud);

int serial_write(serial_handle fd, const void *buf, unsigned int len);

int serial_read(serial_handle fd, void *buf, unsigned int len);

#endif
