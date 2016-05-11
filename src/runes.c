#include <locale.h>

#include "runes.h"

#include "loop.h"
#include "term.h"
#include "window-backend-xlib.h"

int main (int argc, char *argv[])
{
    RunesLoop *loop;
    RunesWindowBackend *wb;

    setlocale(LC_ALL, "");

    loop = runes_loop_new();
    wb = runes_window_backend_new();
    runes_term_register_with_loop(runes_term_new(argc, argv, wb), loop);

    runes_loop_run(loop);

#ifdef RUNES_VALGRIND
    runes_loop_cleanup(loop);
    runes_window_backend_delete(wb);
#endif

    return 0;
}
