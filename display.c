#include <string.h>

#include "runes.h"

void runes_display_init(RunesTerm *t)
{
    cairo_select_font_face(t->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(t->cr, 14.0);
    cairo_set_source_rgb(t->cr, 0.0, 0.0, 1.0);
    cairo_move_to(t->cr, 0.0, 14.0);
}

void runes_display_glyph(RunesTerm *t, char *buf, size_t len)
{
    if (len) {
        char *nl;
        buf[len] = '\0';
        while ((nl = strchr(buf, '\n'))) {
            double x, y;
            *nl = '\0';
            cairo_show_text(t->cr, buf);
            buf = nl + 1;
            cairo_get_current_point(t->cr, &x, &y);
            cairo_move_to(t->cr, 0.0, y + 14.0);
        }
        cairo_show_text(t->cr, buf);
        /* we have to flush manually because XNextEvent (which normally handles
         * flushing) will most likely be called again before the keystroke is
         * handled */
        runes_window_backend_flush(t);
    }
}
