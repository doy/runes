#include <locale.h>

#include "runes.h"

#include "loop.h"
#include "term.h"
#include "window-xlib.h"

int main (int argc, char *argv[])
{
    RunesLoop *loop;
    RunesWindowBackendGlobal *wg;

    setlocale(LC_ALL, "");

    loop = runes_loop_new();
    wg = runes_window_backend_global_init();
    runes_term_register_with_loop(runes_term_new(argc, argv, wg), loop);

    runes_loop_run(loop);

#ifdef RUNES_VALGRIND
    runes_loop_cleanup(loop);
    runes_window_backend_global_cleanup(wg);
#endif

    return 0;
}
