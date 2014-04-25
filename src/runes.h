#ifndef _RUNES_H
#define _RUNES_H

#include <cairo.h>
#include <pango/pangocairo.h>
#include <uv.h>

#define RUNES_READ_BUFFER_LENGTH 4096

struct runes_term;
struct runes_window;
struct runes_pty;
struct runes_screen;
struct runes_loop_data;

typedef struct runes_term RunesTerm;
typedef struct runes_window RunesWindowBackend;
typedef struct runes_pty RunesPtyBackend;
typedef struct runes_screen RunesScreen;
typedef struct runes_loop_data RunesLoopData;

struct runes_loop_data {
    uv_work_t req;
    RunesTerm *t;
};

#include "window-xlib.h"
#include "pty-unix.h"

#include "screen.h"
#include "term.h"
#include "display.h"
#include "config.h"

#define UNUSED(x) ((void)x)

#endif
