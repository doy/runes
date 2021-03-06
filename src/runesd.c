#include "runes.h"

#include "loop.h"
#include "daemon.h"
#include "window-backend-xlib.h"

int main (int argc, char *argv[])
{
    RunesLoop *loop;
    RunesDaemon *daemon;
    RunesWindowBackend *wb;

    UNUSED(argv);

    if (argc > 1) {
        runes_die("runesd takes no arguments; pass them to runesc instead.");
    }

    loop = runes_loop_new();
    wb = runes_window_backend_new();
    daemon = runes_daemon_new(loop, wb);
    UNUSED(daemon);

    runes_loop_run(loop);

#ifdef RUNES_VALGRIND
    runes_daemon_delete(daemon);
    runes_window_backend_delete(wb);
    runes_loop_delete(loop);
#endif

    return 0;
}
