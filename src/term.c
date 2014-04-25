#include "runes.h"

void runes_term_init(RunesTerm *t, int argc, char *argv[])
{
    runes_config_init(t, argc, argv);
    runes_display_init(t);

    runes_window_backend_create_window(t, argc, argv);
    runes_pty_backend_spawn_subprocess(t);

    runes_display_set_window_size(t);
    runes_screen_init(t);

    t->loop = uv_default_loop();
    runes_window_backend_start_loop(t);
    runes_pty_backend_start_loop(t);
}

void runes_term_cleanup(RunesTerm *t)
{
    runes_config_cleanup(t);
    runes_display_cleanup(t);
    runes_window_backend_cleanup(t);
    runes_pty_backend_cleanup(t);
}
