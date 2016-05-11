#ifndef _RUNES_H
#define _RUNES_H

struct runes_term;
struct runes_window;
struct runes_window_backend;
struct runes_pty;
struct runes_config;
struct runes_display;
struct runes_loop;
struct runes_daemon;

typedef struct runes_term RunesTerm;
typedef struct runes_window RunesWindow;
typedef struct runes_window_backend RunesWindowBackend;
typedef struct runes_pty RunesPty;
typedef struct runes_config RunesConfig;
typedef struct runes_display RunesDisplay;
typedef struct runes_loop RunesLoop;
typedef struct runes_daemon RunesDaemon;

#include "util.h"

#endif
