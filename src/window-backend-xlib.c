#include <locale.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "runes.h"
#include "window-backend-xlib.h"

#include "config.h"
#include "display.h"
#include "loop.h"
#include "pty-unix.h"
#include "term.h"

static char *atom_names[RUNES_NUM_ATOMS] = {
    "WM_DELETE_WINDOW",
    "_NET_WM_PING",
    "_NET_WM_PID",
    "_NET_WM_ICON_NAME",
    "_NET_WM_NAME",
    "UTF8_STRING",
    "WM_PROTOCOLS",
    "TARGETS",
    "RUNES_FLUSH",
    "RUNES_SELECTION"
};

RunesWindowBackend *runes_window_backend_new()
{
    RunesWindowBackend *wb;

    setlocale(LC_ALL, "");
    XSetLocaleModifiers("");

    wb = calloc(1, sizeof(RunesWindowBackend));
    wb->dpy = XOpenDisplay(NULL);
    XInternAtoms(wb->dpy, atom_names, RUNES_NUM_ATOMS, False, wb->atoms);

    return wb;
}

void runes_window_backend_delete(RunesWindowBackend *wb)
{
    XCloseDisplay(wb->dpy);
    free(wb);
}
