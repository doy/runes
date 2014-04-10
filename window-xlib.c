#include <cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "runes.h"

static char *atom_names[RUNES_NUM_ATOMS] = {
    "WM_DELETE_WINDOW",
    "_NET_WM_PING",
    "_NET_WM_PID",
    "_NET_WM_ICON_NAME",
    "_NET_WM_NAME",
    "UTF8_STRING",
    "WM_PROTOCOLS"
};

static void runes_get_next_event(uv_work_t *req)
{
    RunesXlibLoopData *data;

    data = (RunesXlibLoopData *)req->data;
    XNextEvent(data->data.t->w.dpy, &data->e);
}

static void runes_process_event(uv_work_t *req, int status)
{
    RunesXlibLoopData *data;
    XEvent *e;
    RunesTerm *t;
    RunesWindowBackend *w;
    int should_close = 0;

    UNUSED(status);

    data = ((RunesXlibLoopData *)req->data);
    e = &data->e;
    t = data->data.t;
    w = &t->w;

    if (!XFilterEvent(e, None)) {
        switch (e->type) {
        case KeyPress: {
            char *buf = NULL;
            size_t len = 8;
            KeySym sym;
            Status s;
            size_t chars;

            buf = malloc(len);

            for (;;) {
                chars = Xutf8LookupString(w->ic, &e->xkey, buf, len - 1, &sym, &s);
                if (s == XBufferOverflow) {
                    len = chars + 1;
                    buf = realloc(buf, len);
                    continue;
                }
                break;
            }

            runes_handle_keyboard_event(t, buf, chars);
            free(buf);
            break;
        }
        case ClientMessage: {
            Atom a = e->xclient.data.l[0];
            if (a == w->atoms[RUNES_ATOM_WM_DELETE_WINDOW]) {
                should_close = 1;
            }
            else if (a == w->atoms[RUNES_ATOM_NET_WM_PING]) {
                e->xclient.window = DefaultRootWindow(w->dpy);
                XSendEvent(
                    w->dpy, e->xclient.window, False,
                    SubstructureNotifyMask | SubstructureRedirectMask,
                    e
                );
            }
            break;
        }
        default:
            break;
        }
    }

    if (!should_close) {
        uv_queue_work(t->loop, req, runes_get_next_event, runes_process_event);
    }
    else {
        runes_handle_close_window(t);
        free(req);
    }
}

static void runes_init_wm_properties(RunesWindowBackend *w, int argc, char *argv[])
{
    pid_t pid;
    XClassHint class_hints = { "runes", "runes" };
    XWMHints wm_hints;
    XSizeHints normal_hints;

    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;

    /* XXX */
    normal_hints.flags = 0;

    XInternAtoms(w->dpy, atom_names, RUNES_NUM_ATOMS, False, w->atoms);
    XSetWMProtocols(w->dpy, w->w, w->atoms, RUNES_NUM_PROTOCOL_ATOMS);

    Xutf8SetWMProperties(w->dpy, w->w, "runes", "runes", argv, argc, &normal_hints, &wm_hints, &class_hints);

    pid = getpid();
    XChangeProperty(w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_PID], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&pid, 1);

    XChangeProperty(w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_ICON_NAME], w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace, (unsigned char *)"runes", 5);
    XChangeProperty(w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_NAME], w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace, (unsigned char *)"runes", 5);
}

static void runes_window_init_loop(RunesTerm *t)
{
    RunesWindowBackend *w;
    unsigned long mask;
    void *data;

    w = &t->w;

    XGetICValues(w->ic, XNFilterEvents, &mask, NULL);
    XSelectInput(w->dpy, w->w, mask|KeyPressMask|StructureNotifyMask);
    XSetICFocus(w->ic);

    data = malloc(sizeof(RunesXlibLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->t = t;

    uv_queue_work(t->loop, data, runes_get_next_event, runes_process_event);
}

void runes_window_backend_init(RunesTerm *t, int argc, char *argv[])
{
    RunesWindowBackend *w;
    unsigned long white;
    XIM im;

    w = &t->w;

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

    runes_init_wm_properties(w, argc, argv);
    runes_window_init_loop(t);
}

cairo_surface_t *runes_window_backend_surface_create(RunesTerm *t)
{
    RunesWindowBackend *w;
    Visual *vis;
    XWindowAttributes attrs;

    w = &t->w;
    XGetWindowAttributes(w->dpy, w->w, &attrs);
    vis = DefaultVisual(w->dpy, DefaultScreen(w->dpy));
    return cairo_xlib_surface_create(w->dpy, w->w, vis, attrs.width, attrs.height);
}

void runes_window_backend_flush(RunesTerm *t)
{
    XFlush(t->w.dpy);
}

void runes_window_backend_request_close(RunesTerm *t)
{
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = t->w.w;
    e.xclient.message_type = t->w.atoms[RUNES_ATOM_WM_PROTOCOLS];
    e.xclient.format = 32;
    e.xclient.data.l[0] = t->w.atoms[RUNES_ATOM_WM_DELETE_WINDOW];
    e.xclient.data.l[1] = CurrentTime;

    XSendEvent(t->w.dpy, t->w.w, False, NoEventMask, &e);
}

void runes_window_backend_cleanup(RunesTerm *t)
{
    RunesWindowBackend *w;
    XIM im;

    w = &t->w;
    im = XIMOfIC(w->ic);
    XDestroyIC(w->ic);
    XCloseIM(im);
    XFreeGC(w->dpy, w->gc);
    XDestroyWindow(w->dpy, w->w);
    XCloseDisplay(w->dpy);
}
