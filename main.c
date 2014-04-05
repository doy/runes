#include <unistd.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm *t;

    t = runes_term_create();

    cairo_select_font_face(t->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(t->cr, 14.0);
    cairo_set_source_rgb(t->cr, 0.0, 0.0, 1.0);
    cairo_move_to(t->cr, 10.0, 50.0);
    cairo_show_text(t->cr, "Hello, world");

    runes_term_flush(t);

    sleep(2);

    runes_term_destroy(t);

    return 0;
}
