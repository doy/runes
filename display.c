#include <string.h>

#include "runes.h"

void runes_display_init(RunesTerm *t)
{
    cairo_font_face_t *font_face;
    cairo_matrix_t font_matrix, ctm;

    t->bgcolor = cairo_pattern_create_rgb(1.0, 1.0, 1.0);
    t->fgcolor = cairo_pattern_create_rgb(0.0, 0.0, 1.0);

    font_face = cairo_toy_font_face_create("monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_matrix_init_scale(&font_matrix, 14.0, 14.0);
    cairo_get_matrix(t->cr, &ctm);
    t->font = cairo_scaled_font_create(font_face, &font_matrix, &ctm, cairo_font_options_create());

    cairo_set_scaled_font(t->cr, t->font);

    cairo_set_source(t->cr, t->bgcolor);
    cairo_paint(t->cr);

    cairo_set_source(t->cr, t->fgcolor);

    runes_display_move_to(t, 0, 0);

    runes_pty_backend_set_window_size(t);
}

static void runes_display_get_font_dimensions(RunesTerm *t, double *fontx, double *fonty, double *ascent)
{
    cairo_font_extents_t extents;

    cairo_font_extents(t->cr, &extents);

    *fontx = extents.max_x_advance;
    *fonty = extents.height;
    *ascent = extents.ascent;
}

void runes_display_get_term_size(RunesTerm *t, int *row, int *col, int *xpixel, int *ypixel)
{
    double fontx, fonty, ascent;
    runes_window_backend_get_size(t, xpixel, ypixel);
    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
    *row = (int)(*ypixel / fonty);
    *col = (int)(*xpixel / fontx);
}

void runes_display_get_position(RunesTerm *t, int *row, int *col)
{
    *row = t->row;
    *col = t->col;
}

void runes_display_move_to(RunesTerm *t, int row, int col)
{
    double fontx, fonty, ascent;

    t->row = row;
    t->col = col;

    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);

    cairo_move_to(t->cr, col * fontx, row * fonty + ascent);
}

void runes_display_show_string(RunesTerm *t, char *buf, size_t len)
{
    if (len) {
        buf[len] = '\0';
        cairo_show_text(t->cr, buf);
        t->col += strlen(buf);
        /* we have to flush manually because XNextEvent (which normally handles
         * flushing) will most likely be called again before the keystroke is
         * handled */
        runes_window_backend_flush(t);
    }
}

void runes_display_backspace(RunesTerm *t)
{
    double x, y;
    double fontx, fonty, ascent;

    runes_display_move_to(t, t->row, t->col - 1);

    cairo_save(t->cr);
    cairo_set_source(t->cr, t->bgcolor);
    cairo_get_current_point(t->cr, &x, &y);
    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
    cairo_rectangle(t->cr, x, y - ascent, fontx, ascent);
    cairo_fill(t->cr);
    runes_window_backend_flush(t);
    cairo_restore(t->cr);

    runes_display_move_to(t, t->row, t->col);
}

void runes_display_kill_line_forward(RunesTerm *t)
{
    double x, y;
    double fontx, fonty, ascent;
    int row, col, xpixel, ypixel;

    cairo_save(t->cr);
    cairo_set_source(t->cr, t->bgcolor);
    cairo_get_current_point(t->cr, &x, &y);
    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
    runes_display_get_term_size(t, &row, &col, &xpixel, &ypixel);
    cairo_rectangle(t->cr, x, y - ascent, xpixel - x, ascent);
    cairo_fill(t->cr);
    runes_window_backend_flush(t);
    cairo_restore(t->cr);

    runes_display_move_to(t, t->row, t->col);
}
