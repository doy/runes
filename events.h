#ifndef _RUNES_EVENTS_H
#define _RUNES_EVENTS_H

struct runes_loop_data {
    uv_work_t req;
    RunesTerm *t;
};

void runes_handle_keyboard_event(RunesTerm *t, char *buf, size_t len);
void runes_handle_close_window(RunesTerm *t);

#endif
