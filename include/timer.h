/*
 *
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdbool.h>

struct timer;
struct etimer;

#define TIMER_MS(x)		((x) * 1000)

typedef int (*timer_cb_t)(void *);

int timer_handle(struct timer *t);

struct timer *timer_create(void);

void timer_destroy(struct timer *t);

struct etimer *timer_add(struct timer *t, timer_cb_t cb, void *arg,
			 uint32_t period, bool run);

void timer_del(struct timer *t, struct etimer *et);

void timer_run(struct etimer *et);

void timer_stop(struct etimer *et);

void timer_set(struct etimer *et, uint32_t period);

#endif
