#include "runes.h"

void runes_handle_keyboard_event(RunesTerm *t, char *buf, size_t len)
{
    runes_pty_backend_write(t, buf, len);
}

void runes_handle_close_window(RunesTerm *t)
{
    uv_stop(t->loop);
    t->loop = NULL;
}
