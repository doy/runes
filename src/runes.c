#include <locale.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm t;

    setlocale(LC_ALL, "");

    runes_term_init(&t, argc, argv);

    uv_run(t.loop, UV_RUN_DEFAULT);

    runes_term_cleanup(&t);

    return 0;
}
