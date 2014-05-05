#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

struct runes_display {
    cairo_t *cr;
    PangoLayout *layout;

    int row_visible_offset;

    int xpixel;
    int ypixel;
    int fontx;
    int fonty;

    char unfocused: 1;
};

void runes_display_init(RunesTerm *t);
void runes_display_set_window_size(RunesTerm *t);
void runes_display_draw_screen(RunesTerm *t);
void runes_display_draw_cursor(RunesTerm *t, cairo_t *cr);
void runes_display_cleanup(RunesTerm *t);

#endif
