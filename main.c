#include <stdio.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm *t;

    t = runes_term_create();

    cairo_select_font_face(t->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(t->cr, 14.0);
    cairo_set_source_rgb(t->cr, 0.0, 0.0, 1.0);
    cairo_move_to(t->cr, 0.0, 14.0);

    XSelectInput(t->w->dpy, t->w->w, KeyPressMask|KeyReleaseMask);

    for (;;) {
        XEvent e;
        char buf[10];

        XNextEvent(t->w->dpy, &e);
        if (e.type == KeyPress) {
            XLookupString(&e, buf, 10, NULL, NULL);
        }
        else {
            continue;
        }
        cairo_show_text(t->cr, buf);
        runes_term_flush(t);
    }

    runes_term_destroy(t);

    return 0;
}
