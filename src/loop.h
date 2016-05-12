#ifndef _RUNES_LOOP_H
#define _RUNES_LOOP_H

#include <event2/event.h>

struct runes_loop {
    struct event_base *base;
};

RunesLoop *runes_loop_new(void);
void runes_loop_run(RunesLoop *loop);
void runes_loop_start_work(
    RunesLoop *loop, int fd, void *t, int (*cb)(void*));
void runes_loop_timer_set(
    RunesLoop *loop, int timeout, void *t, void (*cb)(void*));
void runes_loop_delete(RunesLoop *loop);

#endif
