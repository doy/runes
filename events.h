#ifndef _RUNES_EVENTS_H
#define _RUNES_EVENTS_H

struct loop_data {
    uv_work_t req;
    RunesTerm *t;
};

uv_loop_t *runes_loop_create(RunesTerm *t);
void runes_handle_keyboard_event(RunesTerm *t, char *buf, size_t len);

#endif
