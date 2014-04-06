#ifndef _RUNES_EVENTS_H
#define _RUNES_EVENTS_H

struct loop_data {
    uv_work_t req;
    RunesTerm *t;
};

uv_loop_t *runes_loop_create(RunesTerm *t);

#endif
