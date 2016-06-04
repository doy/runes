#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

#include <cairo.h>
#include <pango/pangocairo.h>
#include <vt100.h>

struct runes_display {
    cairo_t *cr;
    cairo_pattern_t *buffer;
    PangoLayout *layout;

    int row_visible_offset;

    int xpixel;
    int ypixel;
    int fontx;
    int fonty;

    struct vt100_loc selection_start;
    struct vt100_loc selection_end;

    unsigned int unfocused: 1;
    unsigned int has_selection: 1;
    unsigned int dirty: 1;
};

RunesDisplay *runes_display_new(char *font_name);
void runes_display_set_context(RunesTerm *t, cairo_t *cr);
void runes_display_draw_screen(RunesTerm *t);
void runes_display_draw_cursor(RunesTerm *t);
void runes_display_delete(RunesDisplay *display);

#endif
