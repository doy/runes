#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

#include <cairo.h>
#include <pango/pangocairo.h>
#include <vt100.h>

struct runes_display {
    cairo_t *cr;
    cairo_pattern_t *buffer;
    PangoLayout *layout;
    cairo_scaled_font_t *scaled_font;
    PangoGlyph *ascii_glyph_index_cache;

    int row_visible_offset;

    int xpixel;
    int ypixel;
    int fontx;
    int fonty;
    int font_descent;

    struct vt100_loc selection_start;
    struct vt100_loc selection_end;
    char *selection_contents;
    size_t selection_len;

    unsigned int unfocused: 1;
    unsigned int has_selection: 1;
    unsigned int dirty: 1;
};

RunesDisplay *runes_display_new(char *font_name);
void runes_display_set_context(RunesTerm *t, cairo_t *cr);
void runes_display_draw_screen(RunesTerm *t);
void runes_display_draw_cursor(RunesTerm *t);
void runes_display_set_selection(
    RunesTerm *t, struct vt100_loc *start, struct vt100_loc *end);
void runes_display_maybe_clear_selection(RunesTerm *t);
void runes_display_delete(RunesDisplay *display);

#endif
