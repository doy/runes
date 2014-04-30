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
    int width, height;
    cairo_t *old_cr = NULL;
    cairo_surface_t *surface;

    runes_window_backend_get_size(t, &width, &height);

    if (width == t->xpixel && height == t->ypixel) {
        return;
    }

    t->xpixel = width;
    t->ypixel = height;

    t->scr.max.row = t->ypixel / t->fonty;
    t->scr.max.col = t->xpixel / t->fontx;

    t->scr.scroll_top    = 0;
    t->scr.scroll_bottom = t->scr.max.row - 1;

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
    if (t->layout) {
        pango_cairo_update_layout(t->cr, t->layout);
    }
    else {
        PangoAttrList *attrs;
        PangoFontDescription *font_desc;

        attrs = pango_attr_list_new();
        font_desc = pango_font_description_from_string(t->config.font_name);

        t->layout = pango_cairo_create_layout(t->cr);
        pango_layout_set_attributes(t->layout, attrs);
        pango_layout_set_font_description(t->layout, font_desc);

        pango_attr_list_unref(attrs);
        pango_font_description_free(font_desc);
    }

    cairo_save(t->cr);

    if (old_cr) {
        cairo_set_source_surface(t->cr, cairo_get_target(old_cr), 0.0, 0.0);
    }
    else {
        cairo_set_source(t->cr, t->config.bgdefault);
    }

    cairo_paint(t->cr);

    cairo_restore(t->cr);

    if (old_cr) {
        cairo_destroy(old_cr);
    }

    runes_pty_backend_set_window_size(t);
}

void runes_display_draw_screen(RunesTerm *t)
{
    int r;

    /* XXX quite inefficient */
    for (r = 0; r < t->scr.max.row; ++r) {
        int c = 0;

        while (c < t->scr.max.col) {
            c += runes_display_draw_cell(t, r, c);
        }
    }
    runes_window_backend_request_flush(t);
}

void runes_display_draw_cursor(RunesTerm *t, cairo_t *cr)
{
    if (!t->scr.hide_cursor) {
        int row = t->scr.cur.row, col = t->scr.cur.col;

        if (col >= t->scr.max.col) {
            col = t->scr.max.col - 1;
        }

        cairo_save(cr);
        cairo_set_source(cr, t->config.cursorcolor);
        if (t->unfocused) {
            cairo_set_line_width(cr, 1);
            cairo_rectangle(
                cr,
                col * t->fontx + 0.5, row * t->fonty + 0.5,
                t->fontx, t->fonty);
            cairo_stroke(cr);
        }
        else {
            struct runes_cell *cell = &t->scr.rows[row].cells[col];

            cairo_rectangle(
                cr,
                col * t->fontx, row * t->fonty,
                t->fontx, t->fonty);
            cairo_fill(cr);
            runes_display_draw_glyph(
                t, cr, t->config.bgdefault, cell->attrs,
                cell->contents, cell->len, row, col);
        }
        cairo_restore(cr);
    }
}

void runes_display_cleanup(RunesTerm *t)
{
    g_object_unref(t->layout);
    cairo_destroy(t->cr);
}

static void runes_display_recalculate_font_metrics(RunesTerm *t)
{
    PangoFontDescription *desc;
    PangoContext *context;
    PangoFontMetrics *metrics;
    int ascent, descent;

    if (t->layout) {
        desc = (PangoFontDescription *)pango_layout_get_font_description(
            t->layout);
        context = pango_layout_get_context(t->layout);
    }
    else {
        desc = pango_font_description_from_string(t->config.font_name);
        context = pango_font_map_create_context(
            pango_cairo_font_map_get_default());
    }

    metrics = pango_context_get_metrics(context, desc, NULL);

    t->fontx = PANGO_PIXELS(
        pango_font_metrics_get_approximate_digit_width(metrics));
    ascent   = pango_font_metrics_get_ascent(metrics);
    descent  = pango_font_metrics_get_descent(metrics);
    t->fonty = PANGO_PIXELS(ascent + descent);

    pango_font_metrics_unref(metrics);
    if (!t->layout) {
        pango_font_description_free(desc);
        g_object_unref(context);
    }
}

static int runes_display_draw_cell(RunesTerm *t, int row, int col)
{
    struct runes_cell *cell = &t->scr.rows[row].cells[col];
    cairo_pattern_t *bg = NULL, *fg = NULL;
    int bg_is_custom = 0, fg_is_custom = 0;

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

    if (cell->attrs.inverse) {
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
        t, t->cr, bg, row, col, cell->is_wide ? 2 : 1, 1);

    if (cell->len) {
        runes_display_draw_glyph(
            t, t->cr, fg, cell->attrs, cell->contents, cell->len, row, col);
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
    cairo_save(cr);
    cairo_set_source(cr, pattern);
    cairo_rectangle(
        cr, col * t->fontx, row * t->fonty,
        width * t->fontx, height * t->fonty);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void runes_display_draw_glyph(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    struct runes_cell_attrs attrs, char *buf, size_t len, int row, int col)
{
    PangoAttrList *pango_attrs;

    pango_attrs = pango_layout_get_attributes(t->layout);
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
    cairo_move_to(cr, col * t->fontx, row * t->fonty);
    cairo_set_source(cr, pattern);
    pango_layout_set_text(t->layout, buf, len);
    pango_cairo_update_layout(cr, t->layout);
    pango_cairo_show_layout(cr, t->layout);
    cairo_restore(cr);
}
