#ifndef _RUNES_LOOP_H
#define _RUNES_LOOP_H

#include <uv.h>

struct runes_loop {
    uv_loop_t *loop;
};

void runes_loop_init(RunesLoop *loop);
void runes_loop_run(RunesLoop *loop);
void runes_loop_start_work(RunesLoop *loop, RunesTerm *t,
                           void (*work_cb)(RunesTerm*),
                           int (*after_work_cb)(RunesTerm*));
void runes_loop_timer_set(RunesLoop *loop, int timeout, int repeat,
                          RunesTerm *t, void (*cb)(RunesTerm*));
void runes_loop_cleanup(RunesLoop *loop);

#endif
