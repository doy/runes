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

    char unfocused: 1;
    char has_selection: 1;
    char dirty: 1;
};

void runes_display_init(RunesDisplay *display, char *font_name);
void runes_display_set_context(RunesTerm *t, cairo_t *cr);
void runes_display_draw_screen(RunesTerm *t);
void runes_display_draw_cursor(RunesTerm *t);
void runes_display_cleanup(RunesDisplay *display);

#endif
