/*
 * Bootloader for Silicon Labs erf32fg13 device
 *
 * Author
 * 2024  Andrey Mitrofanov <avmwww@gmail.com>
 *
 */

#ifndef _USART_H_
#define _USART_H_

#include "target.h"

void usart_rx_enable(int num);

void usart_half_duplex_rx(int num);

void usart_half_duplex_tx(int num);

void usart_set_msb(int num, int msb);

void usart_set_inv(int num, int rx, int tx);

void usart_set_baudrate(int num, uint32_t baud);

uint32_t usart_get_baudrate(int num);

int usart_read(int num);

int usart_read_buf(int num, void *buf, int len);

int usart_write(int num, char d);

int usart_write_buf(int num, const void *buf, int len);

int usart_init(int num, int rxlen, int txlen);

void usart_rx_irq(int num);

void usart_tx_irq(int num);

void usart_tx_complete_irq(int num);

typedef void (*usart_cb)(void *, uint8_t);

void usart_rx_set_callback(int num, usart_cb, void *arg);

void usart_tx_set_callback(int num, usart_cb, void *arg);

#endif
