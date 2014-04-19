#include <stdio.h>
#include <stdlib.h>
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
        pango_layout_set_attributes(t->layout, pango_attr_list_new());
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

/* XXX this is gross and kind of slow, but i can't find a better way to do it.
 * i need to be able to convert the string into clusters before laying it out,
 * since i'm not going to be using the full layout anyway, but the only way i
 * can see to do that is with pango_get_log_attrs, which only returns character
 * (not byte) offsets, so i have to reparse the utf8 myself once i get the
 * results. not ideal. */
void runes_display_show_string_utf8(RunesTerm *t, char *buf, size_t len)
{
    size_t i, pos, char_len;
    PangoLogAttr *attrs;

    char_len = g_utf8_strlen(buf, len);
    attrs = malloc(sizeof(PangoLogAttr) * (char_len + 1));
    pango_get_log_attrs(buf, len, -1, NULL, attrs, char_len + 1);

    pos = 0;
    for (i = 1; i < char_len + 1; ++i) {
        if (attrs[i].is_cursor_position) {
            char *startpos, *c;
            size_t cluster_len;
            char width = 1;

            startpos = g_utf8_offset_to_pointer(buf, pos);
            cluster_len = g_utf8_offset_to_pointer(buf, i) - startpos;

            for (c = startpos;
                 (size_t)(c - startpos) < cluster_len;
                 c = g_utf8_next_char(c)) {
                if (g_unichar_iswide(g_utf8_get_char(c))) {
                    width = 2;
                }
            }

            runes_display_paint_rectangle(
                t, t->cr, t->bgcolor, t->col, t->row, width, 1);

            pango_layout_set_text(t->layout, startpos, cluster_len);
            pango_cairo_update_layout(t->cr, t->layout);
            pango_cairo_show_layout(t->cr, t->layout);

            if (t->col + width >= t->cols) {
                runes_display_move_to(t, t->row + 1, 0);
            }
            else {
                runes_display_move_to(t, t->row, t->col + width);
            }
            pos = i;
        }
    }

    free(attrs);
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
    PangoAttrList *attrs;

    attrs = pango_layout_get_attributes(t->layout);
    pango_attr_list_change(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
}

void runes_display_reset_bold(RunesTerm *t)
{
    PangoAttrList *attrs;

    attrs = pango_layout_get_attributes(t->layout);
    pango_attr_list_change(attrs, pango_attr_weight_new(PANGO_WEIGHT_NORMAL));
}

void runes_display_set_italic(RunesTerm *t)
{
    PangoAttrList *attrs;

    attrs = pango_layout_get_attributes(t->layout);
    pango_attr_list_change(attrs, pango_attr_style_new(PANGO_STYLE_ITALIC));
}

void runes_display_reset_italic(RunesTerm *t)
{
    PangoAttrList *attrs;

    attrs = pango_layout_get_attributes(t->layout);
    pango_attr_list_change(attrs, pango_attr_style_new(PANGO_STYLE_NORMAL));
}

void runes_display_set_underline(RunesTerm *t)
{
    PangoAttrList *attrs;

    attrs = pango_layout_get_attributes(t->layout);
    pango_attr_list_change(
        attrs, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
}

void runes_display_reset_underline(RunesTerm *t)
{
    PangoAttrList *attrs;

    attrs = pango_layout_get_attributes(t->layout);
    pango_attr_list_change(
        attrs, pango_attr_underline_new(PANGO_UNDERLINE_NONE));
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

    t->fontx = PANGO_PIXELS(
        pango_font_metrics_get_approximate_digit_width(metrics));
    ascent   = pango_font_metrics_get_ascent(metrics);
    descent  = pango_font_metrics_get_descent(metrics);
    t->fonty = PANGO_PIXELS(ascent + descent);

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
