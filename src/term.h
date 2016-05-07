#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

#include <vt100.h>

#include "config.h"
#include "display.h"
#include "pty-unix.h"
#include "window-xlib.h"

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;
    VT100Screen scr;
    RunesConfig config;
    RunesDisplay display;
    RunesLoop *loop;
};

void runes_term_init(RunesTerm *t, RunesLoop *loop, int argc, char *argv[]);
void runes_term_set_window_size(RunesTerm *t, int xpixel, int ypixel);
void runes_term_cleanup(RunesTerm *t);

#endif
