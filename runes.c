#include <locale.h>
#include <stdio.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm *t;

    setlocale(LC_ALL, "");

    t = runes_term_create();

    runes_display_init(t);

    uv_run(t->loop, UV_RUN_DEFAULT);

    runes_term_destroy(t);

    return 0;
}
