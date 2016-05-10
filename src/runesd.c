#include <locale.h>

#include "runes.h"

#include "loop.h"
#include "daemon.h"
#include "window-xlib.h"

int main (int argc, char *argv[])
{
    RunesLoop *loop;
    RunesDaemon *daemon;
    RunesWindowBackendGlobal *wg;

    UNUSED(argv);

    if (argc > 1) {
        runes_die("runesd takes no arguments; pass them to runesc instead.");
    }

    setlocale(LC_ALL, "");

    loop = runes_loop_new();
    wg = runes_window_backend_global_init();
    daemon = runes_daemon_new(loop, wg);
    UNUSED(daemon);

    runes_loop_run(loop);

#ifdef RUNES_VALGRIND
    runes_socket_delete(socket);
    runes_window_backend_global_cleanup(wg);
    runes_loop_delete(loop);
#endif

    return 0;
}
