#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;

    cairo_t *cr;
    cairo_t *backend_cr;
    cairo_t *alternate_cr;
    uv_loop_t *loop;

    cairo_pattern_t *cursorcolor;

    cairo_pattern_t *colors[8];
    cairo_pattern_t *brightcolors[8];

    int fgcolor;
    int bgcolor;

    int row;
    int col;
    int saved_row;
    int saved_col;
    int scroll_top;
    int scroll_bottom;

    int rows;
    int cols;
    int xpixel;
    int ypixel;
    int fontx;
    int fonty;

    char *font_name;
    PangoLayout *layout;

    char bold;
    char hide_cursor;
    char unfocused;

    char application_keypad;
    char application_cursor;
};

void runes_term_init(RunesTerm *t, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
