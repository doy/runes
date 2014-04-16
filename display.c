#include <stdio.h>
#include <string.h>

#include "runes.h"

static void runes_display_calculate_font_dimensions(RunesTerm *t);
static void runes_display_position_cursor(RunesTerm *t, cairo_t *cr);
static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int x, int y, int width, int height);
static cairo_scaled_font_t *runes_display_make_font(RunesTerm *t);
static void runes_display_scroll_down(RunesTerm *t, int rows);

void runes_display_init(RunesTerm *t)
{
    t->font_name      = "monospace";
    t->font_size      = 14.0;
    runes_display_calculate_font_dimensions(t);

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
    t->cr = cairo_create(
        cairo_image_surface_create(
            CAIRO_FORMAT_RGB24,
            t->xpixel, t->ypixel));
    cairo_set_source(t->cr, t->fgcolor);
    cairo_set_scaled_font(t->cr, runes_display_make_font(t));

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

/* note: this uses the backend cairo context because it should be redrawn every
 * time, and shouldn't be left behind when it moves */
void runes_display_draw_cursor(RunesTerm *t)
{
    if (!t->hide_cursor) {
        if (t->unfocused) {
            double x, y;

            cairo_save(t->backend_cr);
            cairo_set_source(t->backend_cr, t->cursorcolor);
            cairo_get_current_point(t->cr, &x, &y);
            cairo_set_line_width(t->backend_cr, 1);
            cairo_rectangle(
                t->backend_cr,
                x + 0.5, y - t->ascent + 0.5, t->fontx, t->fonty);
            cairo_stroke(t->backend_cr);
            cairo_restore(t->backend_cr);
            runes_display_position_cursor(t, t->backend_cr);
        }
        else {
            runes_display_paint_rectangle(
                t, t->backend_cr, t->cursorcolor, t->col, t->row, 1, 1);
        }
    }
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

/* XXX broken with utf8 */
void runes_display_show_string(RunesTerm *t, char *buf, size_t len)
{
    if (len) {
        int remaining = len, space_in_row = t->cols - t->col;

        buf[len] = '\0';

        do {
            int to_write = remaining > space_in_row ? space_in_row : remaining;
            char tmp;

            runes_display_paint_rectangle(
                t, t->cr, t->bgcolor, t->col, t->row, to_write, 1);

            tmp = buf[to_write];
            buf[to_write] = '\0';
            cairo_show_text(t->cr, buf);
            buf[to_write] = tmp;

            if (t->font_underline) {
                double x, y;

                runes_display_position_cursor(t, t->cr);
                cairo_get_current_point(t->cr, &x, &y);
                cairo_save(t->cr);
                cairo_set_line_width(t->cr, 1);
                cairo_move_to(t->cr, x, y - t->ascent + t->fonty - 0.5);
                cairo_line_to(
                    t->cr,
                    x + (t->fontx * to_write), y - t->ascent + t->fonty - 0.5);
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
                /* XXX broken with utf8 */
                t->col += len;
                runes_display_position_cursor(t, t->cr);
            }
        } while (remaining > 0);
    }
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
    t->hide_cursor = 0;
}

void runes_display_hide_cursor(RunesTerm *t)
{
    t->hide_cursor = 1;
}

void runes_display_visual_bell(RunesTerm *t)
{
    runes_window_backend_visual_bell(t);
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
    cairo_pop_group_to_source(t->cr);
    cairo_paint(t->cr);
    runes_display_paint_rectangle(
        t, t->cr, t->colors[0], 0, t->scroll_top, t->cols, rows);
    cairo_restore(t->cr);
}

static void runes_display_calculate_font_dimensions(RunesTerm *t)
{
    cairo_font_extents_t extents;

    cairo_scaled_font_extents(runes_display_make_font(t), &extents);

    t->fontx = extents.max_x_advance;
    t->fonty = extents.height;
    t->ascent = extents.ascent;
}

static void runes_display_position_cursor(RunesTerm *t, cairo_t *cr)
{
    cairo_move_to(cr, t->col * t->fontx, t->row * t->fonty + t->ascent);
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

static cairo_scaled_font_t *runes_display_make_font(RunesTerm *t)
{
    cairo_font_face_t *font_face;
    cairo_matrix_t font_matrix, ctm;

    font_face = cairo_toy_font_face_create(
        t->font_name,
        t->font_italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
        t->font_bold   ? CAIRO_FONT_WEIGHT_BOLD  : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_matrix_init_scale(&font_matrix, t->font_size, t->font_size);
    cairo_matrix_init_identity(&ctm);
    return cairo_scaled_font_create(
        font_face, &font_matrix, &ctm, cairo_font_options_create());
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
    cairo_pop_group_to_source(t->cr);
    cairo_paint(t->cr);
    runes_display_paint_rectangle(
        t, t->cr, t->colors[0], 0, t->scroll_bottom + 1 - rows, t->cols, rows);
    cairo_restore(t->cr);
}
