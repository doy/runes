#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

struct runes_term {
    RunesWindowBackend w;
    RunesPtyBackend pty;
    VT100Screen scr;
    RunesConfig config;
    RunesDisplay display;

    uv_loop_t *loop;
};

void runes_term_init(RunesTerm *t, int argc, char *argv[]);
void runes_term_cleanup(RunesTerm *t);

#endif
