#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

#include <cairo.h>
#include <pango/pangocairo.h>
#include <vt100.h>

struct runes_display {
    cairo_t *cr;
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
};

void runes_display_init(RunesTerm *t);
void runes_display_set_window_size(RunesTerm *t);
void runes_display_draw_screen(RunesTerm *t);
void runes_display_draw_cursor(RunesTerm *t, cairo_t *cr);
int runes_display_loc_is_selected(RunesTerm *t, struct vt100_loc loc);
int runes_display_loc_is_between(
    RunesTerm *t, struct vt100_loc loc,
    struct vt100_loc start, struct vt100_loc end);
void runes_display_cleanup(RunesTerm *t);

#endif
