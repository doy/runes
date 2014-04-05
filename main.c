#include <locale.h>
#include <stdio.h>

#include "runes.h"

int main (int argc, char *argv[])
{
    RunesTerm *t;
    unsigned long mask;

    setlocale(LC_ALL, "");

    t = runes_term_create();

    cairo_select_font_face(t->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(t->cr, 14.0);
    cairo_set_source_rgb(t->cr, 0.0, 0.0, 1.0);
    cairo_move_to(t->cr, 0.0, 14.0);

    XGetICValues(t->w->ic, XNFilterEvents, &mask, NULL);
    XSelectInput(t->w->dpy, t->w->w, mask|KeyPressMask);
    XSetICFocus(t->w->ic);

    for (;;) {
        XEvent e;
        char buf[10];

        XNextEvent(t->w->dpy, &e);
        if (XFilterEvent(&e, None)) {
            continue;
        }
        if (e.type == KeyPress) {
            Status s;
            KeySym sym;
            int chars;

            chars = Xutf8LookupString(t->w->ic, &e.xkey, buf, 9, &sym, &s);
            buf[chars] = '\0';
            switch (s) {
            case XLookupKeySym:
                fprintf(stderr, "keysym: %d\n", sym);
                break;
            case XLookupBoth:
            case XLookupChars:
                fprintf(stderr, "chars: %10s\n", buf);
                break;
            case XLookupNone:
                fprintf(stderr, "none\n");
                break;
            default:
                fprintf(stderr, "???\n");
                break;
            }
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
