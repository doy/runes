#include <cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "xlib.h"

RunesWindow *runes_window_create()
{
    RunesWindow *w;
    unsigned long white;
    XIM im;

    w = malloc(sizeof(RunesWindow));

    w->dpy = XOpenDisplay(NULL);
    white  = WhitePixel(w->dpy, DefaultScreen(w->dpy));
    w->w   = XCreateSimpleWindow(
        w->dpy, DefaultRootWindow(w->dpy),
        0, 0, 240, 80, 0, white, white
    );

    XSelectInput(w->dpy, w->w, StructureNotifyMask);
    XMapWindow(w->dpy, w->w);
    w->gc = XCreateGC(w->dpy, w->w, 0, NULL);
    XSetForeground(w->dpy, w->gc, white);

    for (;;) {
        XEvent e;

        XNextEvent(w->dpy, &e);
        if (e.type == MapNotify) {
            break;
        }
    }

    XSetLocaleModifiers("");
    im = XOpenIM(w->dpy, NULL, NULL, NULL);
    w->ic = XCreateIC(
        im,
        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, w->w,
        XNFocusWindow, w->w,
        NULL
    );
    if (w->ic == NULL) {
        fprintf(stderr, "failed\n");
        exit(1);
    }

    return w;
}

cairo_surface_t *runes_surface_create(RunesWindow *w)
{
    Visual *vis;

    vis = DefaultVisual(w->dpy, DefaultScreen(w->dpy));
    return cairo_xlib_surface_create(w->dpy, w->w, vis, 240, 80);
}

void runes_window_prepare_input(RunesWindow *w)
{
    unsigned long mask;

    XGetICValues(w->ic, XNFilterEvents, &mask, NULL);
    XSelectInput(w->dpy, w->w, mask|KeyPressMask);
    XSetICFocus(w->ic);
}

void runes_window_read_key(RunesWindow *w, char **outbuf, size_t *outlen)
{
    static char *buf = NULL;
    static size_t len = 8;
    XEvent e;
    Status s;
    KeySym sym;
    size_t chars;

    if (!buf) {
        buf = malloc(len);
    }

    XNextEvent(w->dpy, &e);
    if (XFilterEvent(&e, None) || e.type != KeyPress) {
        *outlen = 0;
        *outbuf = buf;
        return;
    }

    for (;;) {
        chars = Xutf8LookupString(w->ic, &e.xkey, buf, len - 1, &sym, &s);
        if (s == XBufferOverflow) {
            len = chars + 1;
            buf = realloc(buf, len);
            continue;
        }
        break;
    }

    *outlen = chars;
    *outbuf = buf;
}

void runes_window_destroy(RunesWindow *w)
{
    XDestroyWindow(w->dpy, w->w);
    XCloseDisplay(w->dpy);

    free(w);
}
