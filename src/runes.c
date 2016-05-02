#include <locale.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesLoop loop;
    RunesTerm t;

    setlocale(LC_ALL, "");

    runes_loop_init(&loop);
    runes_term_init(&t, &loop, argc, argv);

    runes_loop_run(&loop);

    runes_term_cleanup(&t);
    runes_loop_cleanup(&loop);

    return 0;
}
