#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

#include <vt100.h>

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;
    VT100Screen scr;
    RunesConfig config;
    RunesDisplay display;
    RunesLoop *loop;
};

void runes_term_init(RunesTerm *t, RunesLoop *loop, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
