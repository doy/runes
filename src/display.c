#include <stdio.h>
#include <stdlib.h>

#include "runes.h"
#include "display.h"

#include "config.h"
#include "term.h"

static void runes_display_recalculate_font_metrics(
    RunesDisplay *display, char *font_name);
static void runes_display_repaint_screen(RunesTerm *t);
static int runes_display_draw_cell(RunesTerm *t, int row, int col);
static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int row, int col, int width, int height);
static void runes_display_draw_glyph(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    struct vt100_cell_attrs attrs, char *buf, size_t len, int row, int col);

void runes_display_init(RunesDisplay *display, char *font_name)
{
    runes_display_recalculate_font_metrics(display, font_name);
}

void runes_display_set_context(RunesTerm *t, cairo_t *cr)
{
    RunesDisplay *display = t->display;

    display->cr = cr;
    if (display->layout) {
        pango_cairo_update_layout(display->cr, display->layout);
    }
    else {
        PangoAttrList *attrs;
        PangoFontDescription *font_desc;

        attrs = pango_attr_list_new();
        font_desc = pango_font_description_from_string(t->config->font_name);

        display->layout = pango_cairo_create_layout(display->cr);
        pango_layout_set_attributes(display->layout, attrs);
        pango_layout_set_font_description(display->layout, font_desc);

        pango_attr_list_unref(attrs);
        pango_font_description_free(font_desc);
    }
}

void runes_display_draw_screen(RunesTerm *t)
{
    RunesDisplay *display = t->display;
    int r, rows;

    if (t->scr->dirty || display->dirty) {
        if (t->scr->dirty) {
            display->has_selection = 0;
        }

        cairo_push_group(display->cr);

        /* XXX quite inefficient */
        rows = t->scr->grid->max.row;
        for (r = 0; r < rows; ++r) {
            int c = 0, cols = t->scr->grid->max.col;

            while (c < cols) {
                c += runes_display_draw_cell(t, r, c);
            }
        }

        cairo_pattern_destroy(display->buffer);
        display->buffer = cairo_pop_group(display->cr);
    }

    runes_display_repaint_screen(t);

    t->scr->dirty = 0;
    display->dirty = 0;
}

void runes_display_draw_cursor(RunesTerm *t)
{
    RunesDisplay *display = t->display;

    if (!t->scr->hide_cursor) {
        int row = t->scr->grid->cur.row, col = t->scr->grid->cur.col, width;
        struct vt100_cell *cell;

        if (col >= t->scr->grid->max.col) {
            col = t->scr->grid->max.col - 1;
        }

        cell = &t->scr->grid->rows[t->scr->grid->row_top + row].cells[col];
        width = display->fontx;
        if (cell->is_wide) {
            width *= 2;
        }

        cairo_push_group(display->cr);
        cairo_set_source(display->cr, t->config->cursorcolor);
        if (display->unfocused) {
            cairo_set_line_width(display->cr, 1);
            cairo_rectangle(
                display->cr,
                col * display->fontx + 0.5,
                (row + display->row_visible_offset) * display->fonty + 0.5,
                width - 1, display->fonty - 1);
            cairo_stroke(display->cr);
        }
        else {
            cairo_rectangle(
                display->cr,
                col * display->fontx,
                (row + display->row_visible_offset) * display->fonty,
                width, display->fonty);
            cairo_fill(display->cr);
            runes_display_draw_glyph(
                t, display->cr, t->config->bgdefault, cell->attrs,
                cell->contents, cell->len,
                row + display->row_visible_offset, col);
        }
        cairo_pop_group_to_source(display->cr);
        cairo_paint(display->cr);
    }
}

int runes_display_loc_is_selected(RunesTerm *t, struct vt100_loc loc)
{
    RunesDisplay *display = t->display;
    struct vt100_loc start = display->selection_start;
    struct vt100_loc end = display->selection_end;

    if (!display->has_selection) {
        return 0;
    }

    if (loc.row == start.row) {
        int start_max_col;

        start_max_col = vt100_screen_row_max_col(t->scr, start.row);
        if (start.col > start_max_col) {
            start.col = t->scr->grid->max.col;
        }
    }

    if (loc.row == end.row) {
        int end_max_col;

        end_max_col = vt100_screen_row_max_col(t->scr, end.row);
        if (end.col > end_max_col) {
            end.col = t->scr->grid->max.col;
        }
    }

    return runes_display_loc_is_between(t, loc, start, end);
}

int runes_display_loc_is_between(
     RunesTerm *t, struct vt100_loc loc,
     struct vt100_loc start, struct vt100_loc end)
{
    UNUSED(t);

    if (end.row < start.row || (end.row == start.row && end.col < start.col)) {
        struct vt100_loc tmp;

        tmp = start;
        start = end;
        end = tmp;
    }

    if (loc.row < start.row || loc.row > end.row) {
        return 0;
    }

    if (loc.row == start.row && loc.col < start.col) {
        return 0;
    }

    if (loc.row == end.row && loc.col >= end.col) {
        return 0;
    }

    return 1;
}

void runes_display_cleanup(RunesDisplay *display)
{
    cairo_pattern_destroy(display->buffer);
    g_object_unref(display->layout);
}

static void runes_display_recalculate_font_metrics(
    RunesDisplay *display, char *font_name)
{
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
        desc = pango_font_description_from_string(font_name);
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

static void runes_display_repaint_screen(RunesTerm *t)
{
    RunesDisplay *display = t->display;

    if (display->buffer) {
        cairo_set_source(display->cr, display->buffer);
        cairo_paint(display->cr);
    }
}

static int runes_display_draw_cell(RunesTerm *t, int row, int col)
{
    RunesDisplay *display = t->display;
    struct vt100_loc loc = {
        row + t->scr->grid->row_top - display->row_visible_offset, col };
    struct vt100_cell *cell = &t->scr->grid->rows[loc.row].cells[loc.col];
    cairo_pattern_t *bg = NULL, *fg = NULL;
    int bg_is_custom = 0, fg_is_custom = 0;
    int selected;

    selected = runes_display_loc_is_selected(t, loc);

    switch (cell->attrs.bgcolor.type) {
    case VT100_COLOR_DEFAULT:
        bg = t->config->bgdefault;
        break;
    case VT100_COLOR_IDX:
        bg = t->config->colors[cell->attrs.bgcolor.idx];
        break;
    case VT100_COLOR_RGB:
        bg = cairo_pattern_create_rgb(
            cell->attrs.bgcolor.r / 255.0,
            cell->attrs.bgcolor.g / 255.0,
            cell->attrs.bgcolor.b / 255.0);
        bg_is_custom = 1;
        break;
    }

    switch (cell->attrs.fgcolor.type) {
    case VT100_COLOR_DEFAULT:
        fg = t->config->fgdefault;
        break;
    case VT100_COLOR_IDX: {
        unsigned char idx = cell->attrs.fgcolor.idx;

        if (t->config->bold_is_bright && cell->attrs.bold && idx < 8) {
            idx += 8;
        }
        fg = t->config->colors[idx];
        break;
    }
    case VT100_COLOR_RGB:
        fg = cairo_pattern_create_rgb(
            cell->attrs.fgcolor.r / 255.0,
            cell->attrs.fgcolor.g / 255.0,
            cell->attrs.fgcolor.b / 255.0);
        fg_is_custom = 1;
        break;
    }

    if (cell->attrs.inverse ^ selected) {
        if (cell->attrs.fgcolor.id == cell->attrs.bgcolor.id) {
            fg = t->config->bgdefault;
            bg = t->config->fgdefault;
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
    RunesDisplay *display = t->display;

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
    struct vt100_cell_attrs attrs, char *buf, size_t len, int row, int col)
{
    RunesDisplay *display = t->display;
    PangoAttrList *pango_attrs;

    pango_attrs = pango_layout_get_attributes(display->layout);
    if (t->config->bold_is_bold) {
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
