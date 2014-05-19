#include <stdio.h>
#include <stdlib.h>

#include "runes.h"

static void runes_display_recalculate_font_metrics(RunesTerm *t);
static int runes_display_draw_cell(RunesTerm *t, int row, int col);
static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int row, int col, int width, int height);
static void runes_display_draw_glyph(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    struct runes_cell_attrs attrs, char *buf, size_t len, int row, int col);

void runes_display_init(RunesTerm *t)
{
    runes_display_recalculate_font_metrics(t);
}

void runes_display_set_window_size(RunesTerm *t)
{
    RunesDisplay *display = &t->display;
    int width, height;
    cairo_t *old_cr = NULL;
    cairo_surface_t *surface;

    runes_window_backend_get_size(t, &width, &height);

    if (width == display->xpixel && height == display->ypixel) {
        return;
    }

    display->xpixel = width;
    display->ypixel = height;

    old_cr = display->cr;

    /* XXX this should really use cairo_surface_create_similar_image, but when
     * i did that, drawing calls would occasionally block until an X event
     * occurred for some reason. should look into this, because i think
     * create_similar_image does things that are more efficient (using some
     * xlib shm stuff) */
    surface = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, display->xpixel, display->ypixel);
    display->cr = cairo_create(surface);
    cairo_surface_destroy(surface);
    if (display->layout) {
        pango_cairo_update_layout(display->cr, display->layout);
    }
    else {
        PangoAttrList *attrs;
        PangoFontDescription *font_desc;

        attrs = pango_attr_list_new();
        font_desc = pango_font_description_from_string(t->config.font_name);

        display->layout = pango_cairo_create_layout(display->cr);
        pango_layout_set_attributes(display->layout, attrs);
        pango_layout_set_font_description(display->layout, font_desc);

        pango_attr_list_unref(attrs);
        pango_font_description_free(font_desc);
    }

    cairo_save(display->cr);

    if (old_cr) {
        cairo_set_source_surface(
            display->cr, cairo_get_target(old_cr), 0.0, 0.0);
    }
    else {
        cairo_set_source(display->cr, t->config.bgdefault);
    }

    cairo_paint(display->cr);

    cairo_restore(display->cr);

    if (old_cr) {
        cairo_destroy(old_cr);
    }

    runes_screen_set_window_size(t);
    runes_pty_backend_set_window_size(t);
}

void runes_display_draw_screen(RunesTerm *t)
{
    int r, rows;

    if (!t->scr.dirty) {
        return;
    }

    /* XXX quite inefficient */
    rows = t->scr.grid->max.row;
    for (r = 0; r < rows; ++r) {
        int c = 0, cols = t->scr.grid->max.col;

        while (c < cols) {
            c += runes_display_draw_cell(t, r, c);
        }
    }

    t->scr.dirty = 0;
}

void runes_display_draw_cursor(RunesTerm *t, cairo_t *cr)
{
    RunesDisplay *display = &t->display;

    if (!t->scr.hide_cursor) {
        int row = t->scr.grid->cur.row, col = t->scr.grid->cur.col;

        if (col >= t->scr.grid->max.col) {
            col = t->scr.grid->max.col - 1;
        }

        cairo_save(cr);
        cairo_set_source(cr, t->config.cursorcolor);
        if (display->unfocused) {
            cairo_set_line_width(cr, 1);
            cairo_rectangle(
                cr,
                col * display->fontx + 0.5,
                (row + display->row_visible_offset) * display->fonty + 0.5,
                display->fontx - 1, display->fonty - 1);
            cairo_stroke(cr);
        }
        else {
            struct runes_cell *cell = &t->scr.grid->rows[t->scr.grid->row_top + row].cells[col];

            cairo_rectangle(
                cr,
                col * display->fontx,
                (row + display->row_visible_offset) * display->fonty,
                display->fontx, display->fonty);
            cairo_fill(cr);
            runes_display_draw_glyph(
                t, cr, t->config.bgdefault, cell->attrs,
                cell->contents, cell->len,
                row + display->row_visible_offset, col);
        }
        cairo_restore(cr);
    }
}

void runes_display_cleanup(RunesTerm *t)
{
    RunesDisplay *display = &t->display;

    g_object_unref(display->layout);
    cairo_destroy(display->cr);
}

static void runes_display_recalculate_font_metrics(RunesTerm *t)
{
    RunesDisplay *display = &t->display;
    PangoFontDescription *desc;
    PangoContext *context;
    PangoFontMetrics *metrics;
    int ascent, descent;

    if (display->layout) {
        desc = (PangoFontDescription *)pango_layout_get_font_description(
            display->layout);
        context = pango_layout_get_context(display->layout);
    }
    else {
        desc = pango_font_description_from_string(t->config.font_name);
        context = pango_font_map_create_context(
            pango_cairo_font_map_get_default());
    }

    metrics = pango_context_get_metrics(context, desc, NULL);

    display->fontx = PANGO_PIXELS(
        pango_font_metrics_get_approximate_digit_width(metrics));
    ascent   = pango_font_metrics_get_ascent(metrics);
    descent  = pango_font_metrics_get_descent(metrics);
    display->fonty = PANGO_PIXELS(ascent + descent);

    pango_font_metrics_unref(metrics);
    if (!display->layout) {
        pango_font_description_free(desc);
        g_object_unref(context);
    }
}

static int runes_display_draw_cell(RunesTerm *t, int row, int col)
{
    RunesDisplay *display = &t->display;
    struct runes_loc loc = {
        row + t->scr.grid->row_top - display->row_visible_offset, col };
    struct runes_cell *cell = &t->scr.grid->rows[loc.row].cells[loc.col];
    cairo_pattern_t *bg = NULL, *fg = NULL;
    int bg_is_custom = 0, fg_is_custom = 0;
    int selected;

    selected = runes_screen_loc_is_selected(t, loc);

    switch (cell->attrs.bgcolor.type) {
    case RUNES_COLOR_DEFAULT:
        bg = t->config.bgdefault;
        break;
    case RUNES_COLOR_IDX:
        bg = t->config.colors[cell->attrs.bgcolor.idx];
        break;
    case RUNES_COLOR_RGB:
        bg = cairo_pattern_create_rgb(
            cell->attrs.bgcolor.r / 255.0,
            cell->attrs.bgcolor.g / 255.0,
            cell->attrs.bgcolor.b / 255.0);
        bg_is_custom = 1;
        break;
    }

    switch (cell->attrs.fgcolor.type) {
    case RUNES_COLOR_DEFAULT:
        fg = t->config.fgdefault;
        break;
    case RUNES_COLOR_IDX: {
        unsigned char idx = cell->attrs.fgcolor.idx;

        if (t->config.bold_is_bright && cell->attrs.bold && idx < 8) {
            idx += 8;
        }
        fg = t->config.colors[idx];
        break;
    }
    case RUNES_COLOR_RGB:
        fg = cairo_pattern_create_rgb(
            cell->attrs.fgcolor.r / 255.0,
            cell->attrs.fgcolor.g / 255.0,
            cell->attrs.fgcolor.b / 255.0);
        fg_is_custom = 1;
        break;
    }

    if (cell->attrs.inverse ^ selected) {
        if (cell->attrs.fgcolor.id == cell->attrs.bgcolor.id) {
            fg = t->config.bgdefault;
            bg = t->config.fgdefault;
        }
        else {
            cairo_pattern_t *tmp = fg;
            fg = bg;
            bg = tmp;
        }
    }

    runes_display_paint_rectangle(
        t, display->cr, bg, row, col, cell->is_wide ? 2 : 1, 1);

    if (cell->len) {
        runes_display_draw_glyph(
            t, display->cr, fg, cell->attrs, cell->contents, cell->len,
            row, col);
    }

    if (bg_is_custom) {
        cairo_pattern_destroy(bg);
    }

    if (fg_is_custom) {
        cairo_pattern_destroy(fg);
    }

    return cell->is_wide ? 2 : 1;
}

static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int row, int col, int width, int height)
{
    RunesDisplay *display = &t->display;

    cairo_save(cr);
    cairo_set_source(cr, pattern);
    cairo_rectangle(
        cr, col * display->fontx, row * display->fonty,
        width * display->fontx, height * display->fonty);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void runes_display_draw_glyph(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    struct runes_cell_attrs attrs, char *buf, size_t len, int row, int col)
{
    RunesDisplay *display = &t->display;
    PangoAttrList *pango_attrs;

    pango_attrs = pango_layout_get_attributes(display->layout);
    if (t->config.bold_is_bold) {
        pango_attr_list_change(
            pango_attrs, pango_attr_weight_new(
                attrs.bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
    }
    pango_attr_list_change(
        pango_attrs, pango_attr_style_new(
            attrs.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
    pango_attr_list_change(
        pango_attrs, pango_attr_underline_new(
            attrs.underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));

    cairo_save(cr);
    cairo_move_to(cr, col * display->fontx, row * display->fonty);
    cairo_set_source(cr, pattern);
    pango_layout_set_text(display->layout, buf, len);
    pango_cairo_update_layout(cr, display->layout);
    pango_cairo_show_layout(cr, display->layout);
    cairo_restore(cr);
}
