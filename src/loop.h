#ifndef _RUNES_LOOP_H
#define _RUNES_LOOP_H

#include <uv.h>

struct runes_loop {
    uv_loop_t *loop;
};

struct runes_loop_data {
    uv_work_t req;
    RunesTerm *t;
    void (*work_cb)(RunesTerm*);
    int (*after_work_cb)(RunesTerm*);
};

void runes_loop_init(RunesTerm *t);
void runes_loop_run(RunesTerm *t);
void runes_loop_start_work(RunesTerm *t, void (*work_cb)(RunesTerm*),
                           int (*after_work_cb)(RunesTerm*));
void runes_loop_timer_set(RunesTerm *t, int timeout, int repeat,
                          void (*cb)(RunesTerm*));
void runes_loop_cleanup(RunesTerm *t);

#endif
