#include "runes.h"

uv_loop_t *runes_loop_create(RunesTerm *t)
{
    uv_loop_t *loop;

    loop = uv_default_loop();
    runes_loop_init(t, loop);
    return loop;
}

void runes_handle_keyboard_event(RunesTerm *t, char *buf, size_t len)
{
    runes_display_glyph(t, buf, len);
}

void runes_handle_close_window(RunesTerm *t)
{
    uv_stop(t->loop);
    t->loop = NULL;
}
