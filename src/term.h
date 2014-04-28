#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;
    RunesScreen scr;

    cairo_t *cr;
    cairo_t *backend_cr;
    uv_loop_t *loop;

    PangoLayout *layout;

    char readbuf[RUNES_READ_BUFFER_LENGTH];
    int readlen;
    int remaininglen;

    int xpixel;
    int ypixel;
    int fontx;
    int fonty;

    char visual_bell_is_ringing;
    char unfocused;

    cairo_pattern_t *mousecursorcolor;
    cairo_pattern_t *cursorcolor;

    cairo_pattern_t *fgdefault;
    cairo_pattern_t *bgdefault;
    cairo_pattern_t *colors[256];

    int default_rows;
    int default_cols;

    char *cmd;
    char *font_name;

    char bell_is_urgent;
    char bold_is_bright;
    char bold_is_bold;
    char audible_bell;
};

void runes_term_init(RunesTerm *t, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
