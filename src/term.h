#ifndef _RUNES_TERM_H
#define _RUNES_TERM_H

#include <vt100.h>

struct runes_term {
    RunesConfig *config;
    RunesDisplay *display;
    RunesWindow *w;
    RunesPty *pty;
    VT100Screen *scr;
    RunesLoop *loop;

    int refcnt;
};

RunesTerm *runes_term_new(
    int argc, char *argv[], char *envp[], char *cwd, RunesWindowBackend *wb);
void runes_term_register_with_loop(RunesTerm *t, RunesLoop *loop);
void runes_term_set_window_size(RunesTerm *t, int xpixel, int ypixel);
void runes_term_refcnt_inc(RunesTerm *t);
void runes_term_refcnt_dec(RunesTerm *t);
void runes_term_delete(RunesTerm *t);

#endif
