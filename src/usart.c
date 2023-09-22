/*
 *
 */

#include <stdlib.h>

#include "usart.h"
#include "queue.h"

struct usart {
	usart_hw_t *hw;
	struct usart_queue {
		queue_t queue;
		void *buf;
		int len;
		void (*cb)(void *, uint8_t);
		void *arg;
	} rx, tx;
};

static struct usart usart[2];

void usart_tx_complete_irq(int num)
{
	struct usart *u = &usart[num];
	if (u->tx.cb)
		u->tx.cb(u->tx.arg, 0);
	usart_hw_tx_complete_irq_disable(u->hw);
}

void usart_tx_irq(int num)
{
	struct usart *u = &usart[num];
	int data = queue_read(&u->tx.queue);

	if (data < 0) {
		usart_hw_tx_irq_disable(u->hw);
		return;
	}

	usart_hw_tx(u->hw, data);
}

void usart_rx_irq(int num)
{
	struct usart *u = &usart[num];
	uint8_t data = usart_hw_rx(u->hw);

	if (u->rx.cb)
		u->rx.cb(u->rx.arg, data);
	else if (u->rx.len)
		queue_write(&u->rx.queue, data);
}

static int usart_queue_init(struct usart_queue *uq, int len)
{
	void *buf;

	uq->len = len;
	if (!len)
		return 0;

	buf = malloc(len);
	if (!buf)
		return -1;

	uq->buf = buf;

	queue_init(&uq->queue, buf, len);
	return 0;
}

int usart_init(int num, int rxlen, int txlen)
{
	struct usart *u = &usart[num];

	u->hw = usart_hw_init(num);
	if (!u->hw)
		return -1;

	if (usart_queue_init(&u->rx, rxlen) < 0)
		return -1;

	if (usart_queue_init(&u->tx, txlen) < 0) {
		if (u->rx.buf)
			free(u->rx.buf);
		return -1;
	}

	/* enable tx and rx */
	usart_hw_enable(u->hw, 1, 1);
	return 0;
}

int usart_write(int num, char d)
{
	struct usart *u = &usart[num];

	if (queue_write(&u->tx.queue, (uint8_t)d) < 0)
		return -1;

	usart_hw_tx_irq_enable(u->hw);
	// TODO: this is fix USART overflow
	timer_sleep_us(100);
	return 0;
}

int usart_write_buf(int num, const void *buf, int len)
{
	struct usart *u = &usart[num];
	const uint8_t *p = buf;
	int sz = len;

	while (len) {
		if (queue_write(&u->tx.queue, *p++) < 0)
			break;
		len--;
	}

	usart_hw_tx_irq_enable(u->hw);
	return sz - len;
}

int usart_read(int num)
{
	struct usart *u = &usart[num];
	int n;

	n = queue_read(&u->rx.queue);
	if (n < 0) {
		if (usart_hw_rx_ready(u->hw))
			return usart_hw_rx(u->hw);
	}
	return n;
}

int usart_read_buf(int num, void *buf, int len)
{
	struct usart *u = &usart[num];
	uint8_t *p = buf;
	int sz = len;
	int c;

	while (len) {
		c = usart_read(num);
		if (c < 0)
			break;
#if 0
		if ((c = queue_read(&u->rx.queue)) < 0)
			break;
#endif
		*p++ = c;
		len--;
	}

	return sz - len;
}

void usart_half_duplex_tx(int num)
{
	struct usart *u = &usart[num];
	usart_hw_half_duplex_tx(u->hw);
}

void usart_half_duplex_rx(int num)
{
	struct usart *u = &usart[num];
	usart_hw_half_duplex_rx(u->hw);
}

void usart_set_inv(int num, int rx, int tx)
{
	struct usart *u = &usart[num];

	usart_hw_set_inv(u->hw, rx, tx);
}

void usart_set_msb(int num, int msb)
{
	struct usart *u = &usart[num];

	usart_hw_set_msb(u->hw, msb);
}

uint32_t usart_get_baudrate(int num)
{
	struct usart *u = &usart[num];

	return usart_hw_get_baudrate(u->hw);
}

void usart_set_baudrate(int num, uint32_t baud)
{
	struct usart *u = &usart[num];

	usart_hw_set_baudrate(u->hw, baud);
}

void usart_rx_enable(int num)
{
	struct usart *u = &usart[num];

	usart_hw_rx_irq_enable(u->hw);
}

void usart_rx_set_callback(int num, usart_cb cb, void *arg)
{
	struct usart *u = &usart[num];

	u->rx.arg = arg;
	u->rx.cb = cb;
}

void usart_tx_set_callback(int num, usart_cb cb, void *arg)
{
	struct usart *u = &usart[num];

	u->tx.arg = arg;
	u->tx.cb = cb;
}

