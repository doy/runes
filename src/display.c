#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runes.h"
#include "display.h"

#include "config.h"
#include "term.h"

static void runes_display_load_font(
    RunesDisplay *display, char *font_name);
static void runes_display_repaint_screen(RunesTerm *t);
static int runes_display_continue_string(
    struct vt100_cell *old, struct vt100_cell *new);
static void runes_display_draw_string(
    RunesTerm *t, int row, int col, int width, struct vt100_cell **cells,
    size_t len);
static void runes_display_paint_rectangle(
    RunesTerm *t, cairo_t *cr, cairo_pattern_t *pattern,
    int row, int col, int width, int height);
static void runes_display_draw_glyphs(
    RunesTerm *t, cairo_pattern_t *pattern, struct vt100_cell **cells,
    size_t len, int row, int col);
static void runes_display_draw_glyphs_fast(
    RunesTerm *t, cairo_pattern_t *pattern, struct vt100_cell **cells,
    size_t len, int row, int col);
static void runes_display_draw_glyphs_slow(
    RunesTerm *t, cairo_pattern_t *pattern, struct vt100_cell **cells,
    size_t len, int row, int col, size_t buflen);
static int runes_display_glyphs_are_monospace(RunesTerm *t, int width);
static int runes_display_loc_is_selected(RunesTerm *t, struct vt100_loc loc);
static int runes_display_loc_is_between(
     struct vt100_loc loc,
     struct vt100_loc start, struct vt100_loc end);

RunesDisplay *runes_display_new(char *font_name)
{
    RunesDisplay *display;

    display = calloc(1, sizeof(RunesDisplay));
    runes_display_load_font(display, font_name);

    return display;
}

void runes_display_set_window_size(RunesTerm *t, int row, int col)
{
    RunesDisplay *display = t->display;

    UNUSED(row);

    free(display->glyph_buf);
    display->glyph_buf = malloc(col * sizeof(cairo_glyph_t));
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
        PangoLayoutLine *line;
        PangoGlyphItem *glyph_item;
        PangoGlyphItemIter iter;
        PangoGlyph glyph;
        int i;

        attrs = pango_attr_list_new();
        font_desc = pango_font_description_from_string(t->config->font_name);

        display->layout = pango_cairo_create_layout(display->cr);
        pango_layout_set_attributes(display->layout, attrs);
        pango_layout_set_font_description(display->layout, font_desc);

        free(display->ascii_glyph_index_cache);
        display->ascii_glyph_index_cache = calloc(128, sizeof(PangoGlyph));
        for (i = 32; i < 128; ++i) {
            char c = i;

            pango_layout_set_text(display->layout, &c, 1);
            line = pango_layout_get_line_readonly(display->layout, 0);
            glyph_item = line->runs->data;
            pango_glyph_item_iter_init_start(&iter, glyph_item, &c);
            glyph = glyph_item->glyphs->glyphs[iter.start_glyph].glyph;
            display->ascii_glyph_index_cache[i] = glyph;
        }

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
            runes_display_maybe_clear_selection(t);
        }

        cairo_push_group(display->cr);

        /* XXX quite inefficient */
        rows = t->scr->grid->max.row;
        for (r = 0; r < rows; ++r) {
            int c = 0, cols = t->scr->grid->max.col;
            int vr = r + t->scr->grid->row_top - display->row_visible_offset;
            int start = c;
            struct vt100_cell *cell = NULL, *prev_cell = NULL;
            GPtrArray *cells;

            cells = g_ptr_array_new();

            while (c < cols) {
                cell = &t->scr->grid->rows[vr].cells[c];

                if (!runes_display_continue_string(prev_cell, cell)) {
                    runes_display_draw_string(
                        t, r, start, c - start,
                        (struct vt100_cell **)cells->pdata, cells->len);
                    g_ptr_array_set_size(cells, 0);
                    start = c;
                    while (c < cols) {
                        cell = &t->scr->grid->rows[vr].cells[c];
                        if (cell->len == 0) {
                            c++;
                        }
                        else {
                            break;
                        }
                    }
                    if (c > start) {
                        runes_display_draw_string(
                            t, r, start, c - start,
                            (struct vt100_cell **)cells->pdata, cells->len);
                        start = c;
                    }
                    prev_cell = NULL;
                }
                g_ptr_array_add(cells, cell);

                c += cell->is_wide ? 2 : 1;
                prev_cell = cell;
            }
            runes_display_draw_string(
                t, r, start, cols - start,
                (struct vt100_cell **)cells->pdata, cells->len);

            g_ptr_array_unref(cells);
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
            runes_display_draw_glyphs(
                t, t->config->bgdefault, &cell, 1,
                row + display->row_visible_offset, col);
        }
        cairo_pop_group_to_source(display->cr);
        cairo_paint(display->cr);
    }
}

void runes_display_set_selection(
    RunesTerm *t, struct vt100_loc *start, struct vt100_loc *end)
{
    RunesDisplay *display = t->display;

    display->has_selection = 1;

    display->selection_start = *start;
    display->selection_end = *end;

    if (t->display->selection_contents) {
        free(t->display->selection_contents);
        t->display->selection_contents = NULL;
    }

    if (end->row < start->row || (end->row == start->row && end->col < start->col)) {
        struct vt100_loc *tmp;

        tmp = start;
        start = end;
        end = tmp;
    }

    vt100_screen_get_string_plaintext(
        t->scr, start, end,
        &t->display->selection_contents, &t->display->selection_len);

    t->display->dirty = 1;
}

void runes_display_maybe_clear_selection(RunesTerm *t)
{
    RunesDisplay *display = t->display;
    char *contents;
    size_t len;

    if (!display->has_selection) {
        return;
    }

    vt100_screen_get_string_plaintext(
        t->scr, &display->selection_start, &display->selection_end,
        &contents, &len);
    if (len != display->selection_len
        || !contents
        || memcmp(contents, display->selection_contents, len)) {
        display->has_selection = 0;
    }
}

void runes_display_delete(RunesDisplay *display)
{
    free(display->ascii_glyph_index_cache);
    free(display->glyph_buf);
    cairo_scaled_font_destroy(display->scaled_font);
    cairo_pattern_destroy(display->buffer);
    g_object_unref(display->layout);

    free(display);
}

static void runes_display_load_font(
    RunesDisplay *display, char *font_name)
{
    PangoFontDescription *desc;
    PangoContext *context;
    PangoFontMetrics *metrics;
    PangoFontMap *fontmap;
    PangoFont *font;
    int ascent, descent;

    if (display->layout) {
        desc = (PangoFontDescription *)pango_layout_get_font_description(
            display->layout);
        context = pango_layout_get_context(display->layout);
        fontmap = pango_context_get_font_map(context);
    }
    else {
        desc = pango_font_description_from_string(font_name);
        fontmap = pango_cairo_font_map_get_default();
        context = pango_font_map_create_context(fontmap);
    }

    metrics = pango_context_get_metrics(context, desc, NULL);

    display->fontx = PANGO_PIXELS(
        pango_font_metrics_get_approximate_digit_width(metrics));
    ascent   = pango_font_metrics_get_ascent(metrics);
    descent  = pango_font_metrics_get_descent(metrics);
    display->fonty = PANGO_PIXELS(ascent + descent);
    display->font_descent = PANGO_PIXELS(descent);

    font = pango_font_map_load_font(fontmap, context, desc);
    display->scaled_font = pango_cairo_font_get_scaled_font(
        (PangoCairoFont *)font);
    cairo_scaled_font_reference(display->scaled_font);

    g_object_unref(font);
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

static int runes_display_continue_string(
    struct vt100_cell *old, struct vt100_cell *new)
{
    if (!old) {
        return 1;
    }
    if (!old->len || !new->len) {
        return 0;
    }
    return !(
        (old->attrs.fgcolor.id - new->attrs.fgcolor.id) |
        (old->attrs.bgcolor.id - new->attrs.bgcolor.id) |
        (old->attrs.attrs - new->attrs.attrs)
    );
}

static void runes_display_draw_string(
    RunesTerm *t, int row, int col, int width, struct vt100_cell **cells,
    size_t len)
{
    RunesDisplay *display = t->display;
    struct vt100_loc loc = {
        row + t->scr->grid->row_top - display->row_visible_offset,
        col };
    struct vt100_loc eloc = {
        row + t->scr->grid->row_top - display->row_visible_offset,
        col + width };
    struct vt100_cell_attrs *attrs;
    cairo_pattern_t *bg = NULL, *fg = NULL;
    int bg_is_custom = 0, fg_is_custom = 0;
    int selected;

    if (!width) {
        return;
    }

    if (len > 1
        && display->has_selection
        && (display->selection_start.row != display->selection_end.row
            || display->selection_start.col != display->selection_end.col)
        && (runes_display_loc_is_between(
                display->selection_start, loc, eloc)
            || runes_display_loc_is_between(
                display->selection_end, loc, eloc))) {
        size_t i;
        int c = col;

        for (i = 0; i < len; ++i) {
            int width = cells[i]->is_wide ? 2 : 1;

            runes_display_draw_string(t, row, c, width, &cells[i], 1);
            c += width;
        }
        return;
    }

    selected = runes_display_loc_is_selected(t, loc);

    if (len) {
        attrs = &cells[0]->attrs;

        switch (attrs->bgcolor.type) {
        case VT100_COLOR_DEFAULT:
            bg = t->config->bgdefault;
            break;
        case VT100_COLOR_IDX:
            bg = t->config->colors[attrs->bgcolor.idx];
            break;
        case VT100_COLOR_RGB:
            bg = cairo_pattern_create_rgb(
                attrs->bgcolor.r / 255.0,
                attrs->bgcolor.g / 255.0,
                attrs->bgcolor.b / 255.0);
            bg_is_custom = 1;
            break;
        }

        switch (attrs->fgcolor.type) {
        case VT100_COLOR_DEFAULT:
            fg = t->config->fgdefault;
            break;
        case VT100_COLOR_IDX: {
            unsigned char idx = attrs->fgcolor.idx;

            if (t->config->bold_is_bright && attrs->bold && idx < 8) {
                idx += 8;
            }
            fg = t->config->colors[idx];
            break;
        }
        case VT100_COLOR_RGB:
            fg = cairo_pattern_create_rgb(
                attrs->fgcolor.r / 255.0,
                attrs->fgcolor.g / 255.0,
                attrs->fgcolor.b / 255.0);
            fg_is_custom = 1;
            break;
        }

        if (attrs->inverse ^ selected) {
            if (attrs->fgcolor.id == attrs->bgcolor.id) {
                fg = t->config->bgdefault;
                bg = t->config->fgdefault;
            }
            else {
                cairo_pattern_t *tmp = fg;
                fg = bg;
                bg = tmp;
            }
        }
    }
    else {
        bg = t->config->bgdefault;
        fg = t->config->fgdefault;
        if (selected) {
            cairo_pattern_t *tmp = fg;
            fg = bg;
            bg = tmp;
        }
    }

    runes_display_paint_rectangle(t, display->cr, bg, row, col, width, 1);
    if (len) {
        runes_display_draw_glyphs(t, fg, cells, len, row, col);
    }

    if (bg_is_custom) {
        cairo_pattern_destroy(bg);
    }

    if (fg_is_custom) {
        cairo_pattern_destroy(fg);
    }
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

static void runes_display_draw_glyphs(
    RunesTerm *t, cairo_pattern_t *pattern, struct vt100_cell **cells,
    size_t len, int row, int col)
{
    struct vt100_cell_attrs *attrs = &cells[0]->attrs;
    size_t buflen = 0, i;
    int fast = 1;

    /* we only cache the normal font for now */
    if ((attrs->bold && t->config->bold_is_bold)
        || attrs->italic
        || attrs->underline) {
        fast = 0;
    }

    for (i = 0; i < len; ++i) {
        buflen += cells[i]->len;
        /* we only cache glyphs for ascii characters for now */
        if (cells[i]->len > 1) {
            fast = 0;
        }
    }

    if (fast) {
        runes_display_draw_glyphs_fast(t, pattern, cells, len, row, col);
    }
    else {
        runes_display_draw_glyphs_slow(
            t, pattern, cells, len, row, col, buflen);
    }
}

static void runes_display_draw_glyphs_fast(
    RunesTerm *t, cairo_pattern_t *pattern, struct vt100_cell **cells,
    size_t len, int row, int col)
{
    RunesDisplay *display = t->display;
    size_t i;

    for (i = 0; i < len; ++i) {
        cairo_glyph_t *glyph = &display->glyph_buf[i];
        switch (cells[i]->len) {
        case 0:
            glyph->index = display->ascii_glyph_index_cache[' '];
            break;
        case 1: {
            char c = cells[i]->contents[0];
            glyph->index = display->ascii_glyph_index_cache[(int)c];
            break;
        }
        default:
            fprintf(stderr, "runes_display_draw_glyphs_fast requires ascii\n");
            return;
        }
        glyph->x = (col + i) * display->fontx;
        glyph->y = (row + 1) * display->fonty - display->font_descent;
    }

    cairo_save(display->cr);
    cairo_move_to(display->cr, col * display->fontx, row * display->fonty);
    cairo_set_source(display->cr, pattern);
    cairo_set_scaled_font(display->cr, display->scaled_font);
    cairo_show_glyphs(display->cr, display->glyph_buf, len);
    cairo_restore(display->cr);
}

static void runes_display_draw_glyphs_slow(
    RunesTerm *t, cairo_pattern_t *pattern, struct vt100_cell **cells,
    size_t len, int row, int col, size_t buflen)
{
    RunesDisplay *display = t->display;
    struct vt100_cell_attrs *attrs = &cells[0]->attrs;
    PangoAttrList *pango_attrs;
    size_t pos = 0, i;
    char *buf;
    int width = 0;

    buf = malloc(buflen);
    for (i = 0; i < len; ++i) {
        memcpy(&buf[pos], cells[i]->contents, cells[i]->len);
        pos += cells[i]->len;
        width += cells[i]->is_wide ? 2 : 1;
    }

    pango_layout_set_text(display->layout, buf, buflen);
    if (len > 1 && !runes_display_glyphs_are_monospace(t, width)) {
        int c = col;
        for (i = 0; i < len; ++i) {
            runes_display_draw_glyphs(t, pattern, &cells[i], 1, row, c);
            c += cells[i]->is_wide ? 2 : 1;
        }
    }
    else {
        pango_attrs = pango_layout_get_attributes(display->layout);
        if (t->config->bold_is_bold) {
            pango_attr_list_change(
                pango_attrs, pango_attr_weight_new(
                    attrs->bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
        }
        pango_attr_list_change(
            pango_attrs, pango_attr_style_new(
                attrs->italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
        pango_attr_list_change(
            pango_attrs, pango_attr_underline_new(
                attrs->underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));

        cairo_save(display->cr);
        cairo_move_to(display->cr, col * display->fontx, row * display->fonty);
        cairo_set_source(display->cr, pattern);
        pango_cairo_update_layout(display->cr, display->layout);
        pango_cairo_show_layout(display->cr, display->layout);
        cairo_restore(display->cr);
    }

    free(buf);
}

static int runes_display_glyphs_are_monospace(RunesTerm *t, int width)
{
    RunesDisplay *display = t->display;
    int w, h;

    if (pango_layout_get_unknown_glyphs_count(display->layout) > 0) {
        return 0;
    }
    pango_layout_get_pixel_size(display->layout, &w, &h);
    if (w != display->fontx * width) {
        return 0;
    }
    if (h != display->fonty) {
        return 0;
    }
    return 1;
}

static int runes_display_loc_is_selected(RunesTerm *t, struct vt100_loc loc)
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

    return runes_display_loc_is_between(loc, start, end);
}

static int runes_display_loc_is_between(
     struct vt100_loc loc,
     struct vt100_loc start, struct vt100_loc end)
{
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
