/*
 *
 */

#include "queue.h"

int queue_read(queue_t *q)
{
	uint8_t d;

	if (queue_empty(q))
		return -1;

	d = q->data[q->tail & (q->size - 1)];
	q->tail++;
	return d;
}

int queue_read_buf(queue_t *q, void *buf, size_t len)
{
	size_t count = queue_count(q);
	uint8_t *p = buf;
	size_t mask = q->size - 1;

	if (len > count)
		len = count;

	while (len--) {
		*p++ = q->data[q->tail & mask];
		q->tail++;
	}
	return count;
}

int queue_write(queue_t *q, uint8_t d)
{
	if (queue_full(q))
		return -1;

	q->data[q->head & (q->size - 1)] = d;
	q->head++;
	return 0;
}

int queue_init(queue_t *q, void *buf, size_t size)
{
	q->size = size;
	q->data = buf;
	q->head = q->tail = 0;
	return 0;
}

