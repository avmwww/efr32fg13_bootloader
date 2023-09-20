/*
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "queue.h"
#include "timer.h"
#include "target.h"


struct etimer {
	TAILQ_ENTRY(etimer) queue;
	int (*cb)(void *);
	void *arg;
	uint32_t period;
	uint32_t last;
	bool run;
};

TAILQ_HEAD(timer, etimer);

struct etimer *timer_add(struct timer *t, timer_cb_t cb, void *arg,
			 uint32_t period, bool run)
{
	struct etimer *et;

	et = malloc(sizeof(struct etimer));
	if (!et)
		return NULL;

	memset(et, 0, sizeof(struct etimer));

	et->cb = cb;
	et->arg = arg;
	et->period = period;
	et->last = timer_get_us();
	et->run = run;

	TAILQ_INSERT_TAIL(t, et, queue);

	return et;
}

void timer_del(struct timer *t, struct etimer *et)
{
	TAILQ_REMOVE(t, et, queue);
	free(et);
}

void timer_run(struct etimer *et)
{
	et->run = true;
	et->last = timer_get_us();
}

void timer_stop(struct etimer *et)
{
	et->run = false;
}

void timer_set(struct etimer *et, uint32_t period)
{
	et->period = period;
}

/*
 *
 */
int timer_handle(struct timer *t)
{
	uint32_t now = timer_get_us();
	struct etimer *et, *ets;
	int runs = 0;

	TAILQ_FOREACH_SAFE(et, t, queue, ets) {
		if (et->run && et->period && now >= (et->last + et->period)) {
			et->cb(et->arg);
			et->last = now;
			runs++;
		}
	}
	return runs;
}

struct timer *timer_create(void)
{
	struct timer *t;

	t = malloc(sizeof(struct timer));
	if (!t)
		return NULL;

	TAILQ_INIT(t);
	return t;
}

void timer_destroy(struct timer *t)
{
	struct etimer *et, *ets;

	TAILQ_FOREACH_SAFE(et, t, queue, ets) {
		TAILQ_REMOVE(t, et, queue);
		free(et);
	}
	free(t);
}
