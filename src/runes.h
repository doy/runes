#ifndef _RUNES_H
#define _RUNES_H

#define RUNES_READ_BUFFER_LENGTH 4096

struct runes_term;
struct runes_window;
struct runes_pty;
struct runes_config;
struct runes_display;
struct runes_loop;

typedef struct runes_term RunesTerm;
typedef struct runes_window RunesWindowBackend;
typedef struct runes_pty RunesPtyBackend;
typedef struct runes_config RunesConfig;
typedef struct runes_display RunesDisplay;
typedef struct runes_loop RunesLoop;

#include "util.h"

#include "loop.h"

#include "window-xlib.h"
#include "pty-unix.h"

#include "config.h"
#include "display.h"

#include "term.h"

#endif
