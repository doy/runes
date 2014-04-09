#ifndef _RUNES_H
#define _RUNES_H

#include <cairo.h>
#include <uv.h>

struct runes_term;
struct runes_window;
struct runes_loop_data;

typedef struct runes_term RunesTerm;
typedef struct runes_window RunesWindowBackend;
typedef struct runes_loop_data RunesLoopData;

#include "events.h"

#include "window-xlib.h"

#include "term.h"
#include "display.h"

#define UNUSED(x) ((void)x)

#endif
