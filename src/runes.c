#include <locale.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm t;

    setlocale(LC_ALL, "");

    runes_term_init(&t, argc, argv);

    runes_loop_run(&t);

    runes_term_cleanup(&t);

    return 0;
}
