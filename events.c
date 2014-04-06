#include "runes.h"

uv_loop_t *runes_loop_create(RunesTerm *t)
{
    uv_loop_t *loop;

    loop = uv_default_loop();
    runes_loop_init(t, loop);
    return loop;
}

