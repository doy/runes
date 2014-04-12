#include <string.h>

#include "runes.h"

static void runes_display_get_font_dimensions(
    RunesTerm *t, double *fontx, double *fonty, double *ascent);
static cairo_scaled_font_t *runes_display_make_font(RunesTerm *t);

void runes_display_init(RunesTerm *t)
{
    int x, y;

    t->backend_cr = cairo_create(runes_window_backend_surface_create(t));
    runes_window_backend_get_size(t, &x, &y);

    t->cr = NULL;

    t->colors[0] = cairo_pattern_create_rgb(0.0, 0.0, 0.0);
    t->colors[1] = cairo_pattern_create_rgb(1.0, 0.0, 0.0);
    t->colors[2] = cairo_pattern_create_rgb(0.0, 1.0, 0.0);
    t->colors[3] = cairo_pattern_create_rgb(1.0, 1.0, 0.0);
    t->colors[4] = cairo_pattern_create_rgb(0.0, 0.0, 1.0);
    t->colors[5] = cairo_pattern_create_rgb(1.0, 0.0, 1.0);
    t->colors[6] = cairo_pattern_create_rgb(1.0, 1.0, 1.0);
    t->colors[7] = cairo_pattern_create_rgb(1.0, 1.0, 1.0);
    t->fgcolor = t->colors[7];
    t->bgcolor = t->colors[0];

    t->cursorcolor = cairo_pattern_create_rgb(0.0, 1.0, 0.0);
    t->show_cursor = 1;

    t->font_name      = "monospace";
    t->font_size      = 14.0;
    t->font_bold      = 0;
    t->font_italic    = 0;
    t->font_underline = 0;

    t->xpixel = -1;
    t->ypixel = -1;
    runes_display_set_window_size(t, x, y);

    runes_display_reset_text_attributes(t);
    runes_display_move_to(t, 0, 0);
}

void runes_display_set_window_size(RunesTerm *t, int width, int height)
{
    double fontx, fonty, ascent;
    cairo_t *old_cr = NULL;

    if (width == t->xpixel && height == t->ypixel) {
        return;
    }

    t->xpixel = width;
    t->ypixel = height;

    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
    t->rows = t->ypixel / fonty;
    t->cols = t->xpixel / fontx;

    old_cr = t->cr;

    t->cr = cairo_create(
        cairo_surface_create_similar_image(
            cairo_get_target(t->backend_cr), CAIRO_FORMAT_RGB24,
            t->xpixel, t->ypixel));
    cairo_set_source(t->cr, t->fgcolor);
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));

    cairo_save(t->cr);
    cairo_set_source(t->cr, t->bgcolor);
    cairo_move_to(t->cr, 0.0, 0.0);
    cairo_paint(t->cr);

    if (old_cr) {
        cairo_set_source_surface(t->cr, cairo_get_target(old_cr), 0.0, 0.0);
        cairo_move_to(t->cr, 0.0, 0.0);
        cairo_paint(t->cr);
    }

    cairo_restore(t->cr);

    if (old_cr) {
        cairo_destroy(old_cr);
    }

    runes_window_backend_flush(t);
}

/* note: this uses the backend cairo context because it should be redrawn every
 * time, and shouldn't be left behind when it moves */
void runes_display_draw_cursor(RunesTerm *t)
{
    if (t->show_cursor) {
        double x, y;
        double fontx, fonty, ascent;

        cairo_save(t->backend_cr);
        cairo_set_source(t->backend_cr, t->cursorcolor);
        runes_display_move_to(t, t->row, t->col);
        cairo_get_current_point(t->cr, &x, &y);
        runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
        cairo_rectangle(t->backend_cr, x, y - ascent, fontx, fonty);
        cairo_fill(t->backend_cr);
        cairo_restore(t->backend_cr);
    }
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

        if (t->font_underline) {
            double x, y;
            double fontx, fonty, ascent;

            cairo_save(t->cr);
            cairo_get_current_point(t->cr, &x, &y);
            runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
            cairo_set_line_width(t->cr, 1);
            cairo_move_to(
                 t->cr, x - (fontx * strlen(buf)), y - ascent + fonty - 0.5);
            cairo_line_to(t->cr, x, y - ascent + fonty - 0.5);
            cairo_stroke(t->cr);
            cairo_restore(t->cr);

            runes_display_move_to(t, t->row, t->col);
        }

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
    cairo_rectangle(t->cr, x, y - ascent, fontx, fonty);
    cairo_fill(t->cr);
    runes_window_backend_flush(t);
    cairo_restore(t->cr);

    runes_display_move_to(t, t->row, t->col);
}

void runes_display_clear_screen(RunesTerm *t)
{
    cairo_save(t->cr);
    cairo_set_source(t->cr, t->bgcolor);
    cairo_paint(t->cr);
    runes_window_backend_flush(t);
    cairo_restore(t->cr);

    runes_display_move_to(t, t->row, t->col);
}

void runes_display_clear_screen_forward(RunesTerm *t)
{
    double x, y;
    double fontx, fonty, ascent;

    runes_display_kill_line_forward(t);

    cairo_save(t->cr);
    cairo_set_source(t->cr, t->bgcolor);
    cairo_get_current_point(t->cr, &x, &y);
    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
    cairo_rectangle(
        t->cr,
        0,         y - ascent + fonty,
        t->xpixel, t->ypixel - y + ascent - fonty);
    cairo_fill(t->cr);
    runes_window_backend_flush(t);
    cairo_restore(t->cr);

    runes_display_move_to(t, t->row, t->col);
}

void runes_display_kill_line_forward(RunesTerm *t)
{
    double x, y;
    double fontx, fonty, ascent;

    cairo_save(t->cr);
    cairo_set_source(t->cr, t->bgcolor);
    cairo_get_current_point(t->cr, &x, &y);
    runes_display_get_font_dimensions(t, &fontx, &fonty, &ascent);
    cairo_rectangle(t->cr, x, y - ascent, t->xpixel - x, fonty);
    cairo_fill(t->cr);
    runes_window_backend_flush(t);
    cairo_restore(t->cr);

    runes_display_move_to(t, t->row, t->col);
}

void runes_display_reset_text_attributes(RunesTerm *t)
{
    runes_display_reset_fg_color(t);
    runes_display_reset_bg_color(t);
    runes_display_reset_bold(t);
    runes_display_reset_italic(t);
    runes_display_reset_underline(t);
}

void runes_display_set_bold(RunesTerm *t)
{
    t->font_bold = 1;
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));
}

void runes_display_reset_bold(RunesTerm *t)
{
    t->font_bold = 0;
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));
}

void runes_display_set_italic(RunesTerm *t)
{
    t->font_italic = 1;
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));
}

void runes_display_reset_italic(RunesTerm *t)
{
    t->font_italic = 0;
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));
}

void runes_display_set_underline(RunesTerm *t)
{
    t->font_underline = 1;
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));
}

void runes_display_reset_underline(RunesTerm *t)
{
    t->font_underline = 0;
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));
}

void runes_display_set_fg_color(RunesTerm *t, cairo_pattern_t *color)
{
    t->fgcolor = color;
    cairo_set_source(t->cr, t->fgcolor);
}

void runes_display_reset_fg_color(RunesTerm *t)
{
    runes_display_set_fg_color(t, t->colors[7]);
}

void runes_display_set_bg_color(RunesTerm *t, cairo_pattern_t *color)
{
    t->bgcolor = color;
}

void runes_display_reset_bg_color(RunesTerm *t)
{
    runes_display_set_bg_color(t, t->colors[0]);
}

void runes_display_show_cursor(RunesTerm *t)
{
    t->show_cursor = 1;
}

void runes_display_hide_cursor(RunesTerm *t)
{
    t->show_cursor = 0;
}

static void runes_display_get_font_dimensions(
    RunesTerm *t, double *fontx, double *fonty, double *ascent)
{
    cairo_font_extents_t extents;

    cairo_scaled_font_extents(runes_display_make_font(t), &extents);

    *fontx = extents.max_x_advance;
    *fonty = extents.height;
    *ascent = extents.ascent;
}

static cairo_scaled_font_t *runes_display_make_font(RunesTerm *t)
{
    cairo_font_face_t *font_face;
    cairo_matrix_t font_matrix, ctm;

    font_face = cairo_toy_font_face_create(
        t->font_name,
        t->font_italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
        t->font_bold   ? CAIRO_FONT_WEIGHT_BOLD  : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_matrix_init_scale(&font_matrix, t->font_size, t->font_size);
    cairo_get_matrix(t->backend_cr, &ctm);
    return cairo_scaled_font_create(
        font_face, &font_matrix, &ctm, cairo_font_options_create());
}
