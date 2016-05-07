#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct vt100_screen;
typedef struct vt100_screen VT100Screen;

struct runes_term {
    RunesConfig *config;
    RunesDisplay *display;
    RunesWindowBackend *w;
    RunesPtyBackend *pty;
    VT100Screen *scr;
    RunesLoop *loop;
};

void runes_term_init(RunesTerm *t, RunesLoop *loop, int argc, char *argv[]);
void runes_term_set_window_size(RunesTerm *t, int xpixel, int ypixel);
void runes_term_cleanup(RunesTerm *t);

#endif
