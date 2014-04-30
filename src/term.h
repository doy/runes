#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;
    RunesScreen scr;
    RunesConfig config;

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

    char visual_bell_is_ringing: 1;
    char unfocused: 1;
};

void runes_term_init(RunesTerm *t, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
