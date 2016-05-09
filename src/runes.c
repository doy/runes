#include <locale.h>

#include "runes.h"

#include "loop.h"
#include "term.h"

int main (int argc, char *argv[])
{
    RunesLoop loop;
    RunesTerm t;

    setlocale(LC_ALL, "");

    runes_loop_init(&loop);
    runes_term_init(&t, &loop, argc, argv);

    runes_loop_run(&loop);

#ifdef RUNES_VALGRIND
    runes_term_cleanup(&t);
    runes_loop_cleanup(&loop);
#endif

    return 0;
}
