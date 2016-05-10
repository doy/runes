#ifndef _RUNES_LOOP_H
#define _RUNES_LOOP_H

#include <uv.h>

struct runes_loop {
    uv_loop_t *loop;
};

RunesLoop *runes_loop_new();
void runes_loop_run(RunesLoop *loop);
void runes_loop_start_work(RunesLoop *loop, void *t,
                           void (*work_cb)(void*),
                           int (*after_work_cb)(void*));
void runes_loop_timer_set(RunesLoop *loop, int timeout, int repeat,
                          void *t, void (*cb)(void*));
void runes_loop_delete(RunesLoop *loop);

#endif
