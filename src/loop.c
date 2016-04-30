#include "runes.h"

void runes_loop_init(RunesTerm *t)
{
    RunesLoop *loop = &t->loop;

    loop->loop = uv_default_loop();
    runes_window_backend_init_loop(t);
    runes_pty_backend_init_loop(t);
}

void runes_loop_run(RunesTerm *t)
{
    RunesLoop *loop = &t->loop;

    uv_run(loop->loop, UV_RUN_DEFAULT);
}

void runes_loop_cleanup(RunesTerm *t)
{
    RunesLoop *loop = &t->loop;

    uv_loop_close(loop->loop);
}
