#include <locale.h>
#include <stdio.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm *t;

    setlocale(LC_ALL, "");

    t = runes_term_create();

    cairo_select_font_face(t->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(t->cr, 14.0);
    cairo_set_source_rgb(t->cr, 0.0, 0.0, 1.0);
    cairo_move_to(t->cr, 0.0, 14.0);

    runes_prepare_input(t);

    for (;;) {
        char *buf;
        size_t len;

        runes_read_key(t, &buf, &len);
        if (len) {
            buf[len] = '\0';
            cairo_show_text(t->cr, buf);
            cairo_surface_flush(t->surface);
        }
    }

    runes_term_destroy(t);

    return 0;
}
