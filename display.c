#include <stdio.h>
#include <string.h>

#include "runes.h"

static void runes_display_recalculate_font_metrics(RunesTerm *t);
static void runes_display_position_cursor(RunesTerm *t, cairo_t *cr);
static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int x, int y, int width, int height);
static void runes_display_scroll_down(RunesTerm *t, int rows);

void runes_display_init(RunesTerm *t)
{
    t->font_name = "monospace 10";
    runes_display_recalculate_font_metrics(t);

    t->colors[0] = cairo_pattern_create_rgb(0.0, 0.0, 0.0);
    t->colors[1] = cairo_pattern_create_rgb(1.0, 0.0, 0.0);
    t->colors[2] = cairo_pattern_create_rgb(0.0, 1.0, 0.0);
    t->colors[3] = cairo_pattern_create_rgb(1.0, 1.0, 0.0);
    t->colors[4] = cairo_pattern_create_rgb(0.0, 0.0, 1.0);
    t->colors[5] = cairo_pattern_create_rgb(1.0, 0.0, 1.0);
    t->colors[6] = cairo_pattern_create_rgb(1.0, 1.0, 1.0);
    t->colors[7] = cairo_pattern_create_rgb(1.0, 1.0, 1.0);

    t->cursorcolor = cairo_pattern_create_rgba(0.0, 1.0, 0.0, 0.5);

    t->fgcolor = t->colors[7];
    t->bgcolor = t->colors[0];
}

void runes_display_set_window_size(RunesTerm *t)
{
    int width, height;
    cairo_t *old_cr = NULL;
    cairo_surface_t *surface;

    runes_window_backend_get_size(t, &width, &height);

    if (width == t->xpixel && height == t->ypixel) {
        return;
    }

    t->xpixel = width;
    t->ypixel = height;

    t->rows = t->ypixel / t->fonty;
    t->cols = t->xpixel / t->fontx;

    old_cr = t->cr;

    /* XXX this should really use cairo_surface_create_similar_image, but when
     * i did that, drawing calls would occasionally block until an X event
     * occurred for some reason. should look into this, because i think
     * create_similar_image does things that are more efficient (using some
     * xlib shm stuff) */
    surface = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, t->xpixel, t->ypixel);
    t->cr = cairo_create(surface);
    cairo_surface_destroy(surface);
    cairo_set_source(t->cr, t->fgcolor);
    if (t->layout) {
        pango_cairo_update_layout(t->cr, t->layout);
    }
    else {
        t->layout = pango_cairo_create_layout(t->cr);
    }
    pango_layout_set_font_description(
        t->layout, pango_font_description_from_string(t->font_name));

    cairo_save(t->cr);

    if (old_cr) {
        cairo_set_source_surface(t->cr, cairo_get_target(old_cr), 0.0, 0.0);
    }
    else {
        cairo_set_source(t->cr, t->bgcolor);
    }

    cairo_paint(t->cr);

    cairo_restore(t->cr);

    if (old_cr) {
        cairo_destroy(old_cr);
    }

    runes_pty_backend_set_window_size(t);
    runes_display_position_cursor(t, t->cr);
}

void runes_display_focus_in(RunesTerm *t)
{
    t->unfocused = 0;
}

void runes_display_focus_out(RunesTerm *t)
{
    t->unfocused = 1;
}

void runes_display_move_to(RunesTerm *t, int row, int col)
{
    int height = t->scroll_bottom - t->scroll_top;

    t->row = row + t->scroll_top;
    t->col = col;

    if (row > height) {
        runes_display_scroll_down(t, row - height);
        t->row = t->scroll_bottom;
    }
    else if (row < 0) {
        runes_display_scroll_up(t, -row);
        t->row = t->scroll_top;
    }

    runes_display_position_cursor(t, t->cr);
}

void runes_display_show_string_ascii(RunesTerm *t, char *buf, size_t len)
{
    if (len) {
        int remaining = len, space_in_row = t->cols - t->col;

        do {
            int to_write = remaining > space_in_row ? space_in_row : remaining;

            runes_display_paint_rectangle(
                t, t->cr, t->bgcolor, t->col, t->row, to_write, 1);

            pango_layout_set_text(t->layout, buf, to_write);
            pango_cairo_update_layout(t->cr, t->layout);
            pango_cairo_show_layout(t->cr, t->layout);

            if (t->font_underline) {
                double x, y;

                runes_display_position_cursor(t, t->cr);
                cairo_get_current_point(t->cr, &x, &y);
                cairo_save(t->cr);
                cairo_set_line_width(t->cr, 1);
                cairo_move_to(t->cr, x, y + t->fonty - 0.5);
                cairo_line_to(
                    t->cr,
                    x + (t->fontx * to_write), y + t->fonty - 0.5);
                cairo_stroke(t->cr);
                cairo_restore(t->cr);
            }

            buf += to_write;
            remaining -= to_write;
            space_in_row = t->cols;
            if (remaining) {
                runes_display_move_to(t, t->row + 1, 0);
            }
            else {
                t->col += len;
                runes_display_position_cursor(t, t->cr);
            }
        } while (remaining > 0);
    }
}

void runes_display_show_string_utf8(RunesTerm *t, char *buf, size_t len)
{
    /* XXX */
    runes_display_show_string_ascii(t, buf, len);
}

void runes_display_clear_screen(RunesTerm *t)
{
    runes_display_paint_rectangle(
        t, t->cr, t->bgcolor, 0, 0, t->cols, t->rows);
}

void runes_display_clear_screen_forward(RunesTerm *t)
{
    runes_display_kill_line_forward(t);
    runes_display_paint_rectangle(
        t, t->cr, t->bgcolor, 0, t->row, t->cols, t->rows - t->row);
}

void runes_display_kill_line_forward(RunesTerm *t)
{
    runes_display_paint_rectangle(
        t, t->cr, t->bgcolor, t->col, t->row, t->cols - t->col, 1);
}

void runes_display_delete_characters(RunesTerm *t, int count)
{
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    cairo_save(t->cr);
    cairo_push_group(t->cr);
    pattern = cairo_pattern_create_for_surface(cairo_get_target(t->cr));
    cairo_matrix_init_translate(&matrix, count * t->fontx, 0.0);
    cairo_pattern_set_matrix(pattern, &matrix);
    runes_display_paint_rectangle(
        t, t->cr, pattern, t->col, t->row, t->cols - t->col - count, 1);
    cairo_pattern_destroy(pattern);
    cairo_pop_group_to_source(t->cr);
    cairo_paint(t->cr);
    runes_display_paint_rectangle(
        t, t->cr, t->colors[0], t->cols - count, t->row, count, 1);
    cairo_restore(t->cr);
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
    runes_display_recalculate_font_metrics(t);
}

void runes_display_reset_bold(RunesTerm *t)
{
    t->font_bold = 0;
    runes_display_recalculate_font_metrics(t);
}

void runes_display_set_italic(RunesTerm *t)
{
    t->font_italic = 1;
    runes_display_recalculate_font_metrics(t);
}

void runes_display_reset_italic(RunesTerm *t)
{
    t->font_italic = 0;
    runes_display_recalculate_font_metrics(t);
}

void runes_display_set_underline(RunesTerm *t)
{
    t->font_underline = 1;
    runes_display_recalculate_font_metrics(t);
}

void runes_display_reset_underline(RunesTerm *t)
{
    t->font_underline = 0;
    runes_display_recalculate_font_metrics(t);
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
    t->hide_cursor = 0;
}

void runes_display_hide_cursor(RunesTerm *t)
{
    t->hide_cursor = 1;
}

void runes_display_save_cursor(RunesTerm *t)
{
    t->saved_row = t->row;
    t->saved_col = t->col;
    /* XXX do other stuff here? */
}

void runes_display_restore_cursor(RunesTerm *t)
{
    t->row = t->saved_row;
    t->col = t->saved_col;
}

void runes_display_use_alternate_buffer(RunesTerm *t)
{
    if (t->alternate) {
        return;
    }

    runes_display_save_cursor(t);
    t->alternate = 1;
    t->alternate_cr = t->cr;
    t->cr = NULL;
    t->xpixel = -1;
    t->ypixel = -1;
    runes_display_set_window_size(t);
}

void runes_display_use_normal_buffer(RunesTerm *t)
{
    if (!t->alternate) {
        return;
    }

    runes_display_restore_cursor(t);
    t->alternate = 0;
    cairo_destroy(t->cr);
    t->cr = t->alternate_cr;
    t->alternate_cr = NULL;
    t->xpixel = -1;
    t->ypixel = -1;
    runes_display_set_window_size(t);
}

void runes_display_set_scroll_region(
    RunesTerm *t, int top, int bottom, int left, int right)
{
    top    = (top    < 1       ? 1       : top)    - 1;
    bottom = (bottom > t->rows ? t->rows : bottom) - 1;
    left   = (left   < 1       ? 1       : left)   - 1;
    right  = (right  > t->cols ? t->cols : right)  - 1;

    if (left != 0 || right != t->cols - 1) {
        fprintf(stderr, "vertical scroll regions not yet supported\n");
    }

    if (top >= bottom || left >= right) {
        t->scroll_top = 0;
        t->scroll_bottom = t->rows - 1;
        return;
    }

    t->scroll_top    = top;
    t->scroll_bottom = bottom;
    runes_display_move_to(t, 0, 0);
}

void runes_display_scroll_up(RunesTerm *t, int rows)
{
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    cairo_save(t->cr);
    cairo_push_group(t->cr);
    pattern = cairo_pattern_create_for_surface(cairo_get_target(t->cr));
    cairo_matrix_init_translate(&matrix, 0.0, -rows * t->fonty);
    cairo_pattern_set_matrix(pattern, &matrix);
    runes_display_paint_rectangle(
        t, t->cr, pattern,
        0, t->scroll_top + rows,
        t->cols, t->scroll_bottom - t->scroll_top + 1 - rows);
    cairo_pattern_destroy(pattern);
    cairo_pop_group_to_source(t->cr);
    cairo_paint(t->cr);
    runes_display_paint_rectangle(
        t, t->cr, t->colors[0], 0, t->scroll_top, t->cols, rows);
    cairo_restore(t->cr);
}

void runes_display_cleanup(RunesTerm *t)
{
    int i;

    for (i = 0; i < 8; ++i) {
        cairo_pattern_destroy(t->colors[i]);
    }
    cairo_pattern_destroy(t->cursorcolor);
    cairo_destroy(t->cr);
}

static void runes_display_recalculate_font_metrics(RunesTerm *t)
{
    PangoFontDescription *desc;
    PangoContext *context;
    PangoFontMetrics *metrics;
    int ascent, descent;

    desc = pango_font_description_from_string(t->font_name);

    if (t->layout) {
        context = pango_layout_get_context(t->layout);
    }
    else {
        context = pango_font_map_create_context(
            pango_cairo_font_map_get_default());
    }

    metrics = pango_context_get_metrics(context, desc, NULL);

    t->fontx  = pango_font_metrics_get_approximate_char_width(metrics) / PANGO_SCALE;
    ascent = pango_font_metrics_get_ascent(metrics);
    descent = pango_font_metrics_get_descent(metrics);
    t->fonty  = (ascent + descent) / PANGO_SCALE;

    pango_font_description_free(desc);
    if (!t->layout) {
        g_object_unref(context);
    }
}

static void runes_display_position_cursor(RunesTerm *t, cairo_t *cr)
{
    cairo_move_to(cr, t->col * t->fontx, t->row * t->fonty);
}

static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int x, int y, int width, int height)
{
    cairo_save(cr);
    cairo_set_source(cr, pattern);
    cairo_rectangle(
        cr, x * t->fontx, y * t->fonty, width * t->fontx, height * t->fonty);
    cairo_fill(cr);
    cairo_restore(cr);
    runes_display_position_cursor(t, t->cr);
}

static void runes_display_scroll_down(RunesTerm *t, int rows)
{
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    cairo_save(t->cr);
    cairo_push_group(t->cr);
    pattern = cairo_pattern_create_for_surface(cairo_get_target(t->cr));
    cairo_matrix_init_translate(&matrix, 0.0, rows * t->fonty);
    cairo_pattern_set_matrix(pattern, &matrix);
    runes_display_paint_rectangle(
        t, t->cr, pattern,
        0, t->scroll_top, t->cols, t->scroll_bottom - t->scroll_top);
    cairo_pattern_destroy(pattern);
    cairo_pop_group_to_source(t->cr);
    cairo_paint(t->cr);
    runes_display_paint_rectangle(
        t, t->cr, t->colors[0], 0, t->scroll_bottom + 1 - rows, t->cols, rows);
    cairo_restore(t->cr);
}
