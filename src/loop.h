#ifndef _RUNES_LOOP_H
#define _RUNES_LOOP_H

struct runes_loop {
    uv_loop_t *loop;
};

struct runes_loop_data {
    uv_work_t req;
    RunesTerm *t;
};

void runes_loop_init(RunesTerm *t);
void runes_loop_run(RunesTerm *t);
void runes_loop_cleanup(RunesTerm *t);

#endif
