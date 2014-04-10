#include <string.h>

#include "runes.h"

void runes_display_init(RunesTerm *t)
{
    cairo_font_extents_t extents;

    cairo_select_font_face(t->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(t->cr, 14.0);
    cairo_set_source_rgb(t->cr, 0.0, 0.0, 1.0);

    cairo_font_extents(t->cr, &extents);
    cairo_move_to(t->cr, 0.0, extents.ascent);

    runes_pty_backend_set_window_size(t);
}

static void runes_display_get_font_dimensions(RunesTerm *t, double *fontx, double *fonty)
{
    cairo_font_extents_t extents;

    cairo_font_extents(t->cr, &extents);

    *fontx = extents.max_x_advance;
    *fonty = extents.height;
}

void runes_display_get_term_size(RunesTerm *t, int *row, int *col, int *xpixel, int *ypixel)
{
    double fontx, fonty;
    runes_window_backend_get_size(t, xpixel, ypixel);
    runes_display_get_font_dimensions(t, &fontx, &fonty);
    *row = (int)(*ypixel / fonty);
    *col = (int)(*xpixel / fontx);
}

void runes_display_glyph(RunesTerm *t, char *buf, size_t len)
{
    if (len) {
        cairo_font_extents_t extents;

        cairo_font_extents(t->cr, &extents);

        char *nl;
        buf[len] = '\0';
        while ((nl = strchr(buf, '\n'))) {
            double x, y;
            *nl = '\0';
            cairo_show_text(t->cr, buf);
            buf = nl + 1;
            cairo_get_current_point(t->cr, &x, &y);
            cairo_move_to(t->cr, 0.0, y + extents.height);
        }
        cairo_show_text(t->cr, buf);
        /* we have to flush manually because XNextEvent (which normally handles
         * flushing) will most likely be called again before the keystroke is
         * handled */
        runes_window_backend_flush(t);
    }
}
