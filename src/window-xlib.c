#include <cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/cursorfont.h>
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
    "WM_PROTOCOLS",
    "RUNES_FLUSH",
    "RUNES_VISUAL_BELL",
    "RUNES_AUDIBLE_BELL",
    "RUNES_SELECTION"
};

struct function_key {
    KeySym sym;
    char *str;
    size_t len;
};

#define RUNES_KEY(sym, str) { sym, str, sizeof(str) - 1 }
static struct function_key keys[] = {
    RUNES_KEY(XK_Up,         "\e[A"),
    RUNES_KEY(XK_Down,       "\e[B"),
    RUNES_KEY(XK_Right,      "\e[C"),
    RUNES_KEY(XK_Left,       "\e[D"),
    RUNES_KEY(XK_Page_Up,    "\e[5~"),
    RUNES_KEY(XK_Page_Down,  "\e[6~"),
    RUNES_KEY(XK_Home,       "\e[H"),
    RUNES_KEY(XK_End,        "\e[F"),
    RUNES_KEY(XK_F1,         "\eOP"),
    RUNES_KEY(XK_F2,         "\eOQ"),
    RUNES_KEY(XK_F3,         "\eOR"),
    RUNES_KEY(XK_F4,         "\eOS"),
    RUNES_KEY(XK_F5,         "\e[15~"),
    RUNES_KEY(XK_F6,         "\e[17~"),
    RUNES_KEY(XK_F7,         "\e[18~"),
    RUNES_KEY(XK_F8,         "\e[19~"),
    RUNES_KEY(XK_F9,         "\e[20~"),
    RUNES_KEY(XK_F10,        "\e[21~"),
    RUNES_KEY(XK_F11,        "\e[23~"),
    RUNES_KEY(XK_F12,        "\e[24~"),
    RUNES_KEY(XK_F13,        "\e[25~"),
    RUNES_KEY(XK_F14,        "\e[26~"),
    RUNES_KEY(XK_F15,        "\e[28~"),
    RUNES_KEY(XK_F16,        "\e[29~"),
    RUNES_KEY(XK_F17,        "\e[31~"),
    RUNES_KEY(XK_F18,        "\e[32~"),
    RUNES_KEY(XK_F19,        "\e[33~"),
    RUNES_KEY(XK_F20,        "\e[34~"),
    /* XXX keypad keys need to go here too */
    RUNES_KEY(XK_VoidSymbol, "")
};

static struct function_key application_keypad_keys[] = {
    /* XXX i don't have a keypad on my laptop, need to get one for testing */
    RUNES_KEY(XK_VoidSymbol, "")
};

static struct function_key application_cursor_keys[] = {
    RUNES_KEY(XK_Up,    "\eOA"),
    RUNES_KEY(XK_Down,  "\eOB"),
    RUNES_KEY(XK_Right, "\eOC"),
    RUNES_KEY(XK_Left,  "\eOD"),
    /* XXX home/end? */
    RUNES_KEY(XK_VoidSymbol, "")
};
#undef RUNES_KEY

static void runes_window_backend_get_next_event(uv_work_t *req);
static void runes_window_backend_process_event(uv_work_t *req, int status);
static void runes_window_backend_resize_window(
    RunesTerm *t, int width, int height);
static void runes_window_backend_flush(RunesTerm *t);
static void runes_window_backend_visual_bell(RunesTerm *t);
static void runes_window_backend_reset_visual_bell(uv_timer_t *handle);
static void runes_window_backend_audible_bell(RunesTerm *t);
static void runes_window_backend_draw_cursor(RunesTerm *t);
static void runes_window_backend_set_urgent(RunesTerm *t);
static void runes_window_backend_clear_urgent(RunesTerm *t);
static void runes_window_backend_paste(RunesTerm *t, Time time);
static void runes_window_backend_handle_key_event(RunesTerm *t, XKeyEvent *e);
static int runes_window_backend_handle_builtin_keypress(
    RunesTerm *t, KeySym sym, XKeyEvent *e);
static struct function_key *runes_window_backend_find_key_sequence(
    RunesTerm *t, KeySym sym);
static void runes_window_backend_handle_button_event(
    RunesTerm *t, XButtonEvent *e);
static int runes_window_backend_handle_builtin_button_press(
    RunesTerm *t, XButtonEvent *e);
static void runes_window_backend_handle_expose_event(
    RunesTerm *t, XExposeEvent *e);
static void runes_window_backend_handle_configure_event(
    RunesTerm *t, XConfigureEvent *e);
static void runes_window_backend_handle_focus_event(
    RunesTerm *t, XFocusChangeEvent *e);
static void runes_window_backend_handle_selection_notify_event(
    RunesTerm *t, XSelectionEvent *e);

void runes_window_backend_create_window(RunesTerm *t, int argc, char *argv[])
{
    RunesWindowBackend *w = &t->w;
    pid_t pid;
    XClassHint class_hints = { "runes", "runes" };
    XWMHints wm_hints;
    XSizeHints normal_hints;
    unsigned long white;
    XIM im;
    Cursor cursor;
    double mouse_r, mouse_g, mouse_b;
    XColor cursor_fg, cursor_bg;
    Visual *vis;
    cairo_surface_t *surface;

    wm_hints.flags = InputHint | StateHint;
    wm_hints.input = True;
    wm_hints.initial_state = NormalState;

    normal_hints.flags = PMinSize | PResizeInc | PBaseSize;

    normal_hints.min_width   = t->fontx;
    normal_hints.min_height  = t->fonty;
    normal_hints.width_inc   = t->fontx;
    normal_hints.height_inc  = t->fonty;
    normal_hints.base_width  = t->fontx * t->default_cols;
    normal_hints.base_height = t->fonty * t->default_rows;

    XInitThreads();

    w->dpy = XOpenDisplay(NULL);
    white  = WhitePixel(w->dpy, DefaultScreen(w->dpy));
    w->w   = XCreateSimpleWindow(
        w->dpy, DefaultRootWindow(w->dpy),
        0, 0, normal_hints.base_width, normal_hints.base_height,
        0, white, white);

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

    XInternAtoms(w->dpy, atom_names, RUNES_NUM_ATOMS, False, w->atoms);
    XSetWMProtocols(w->dpy, w->w, w->atoms, RUNES_NUM_PROTOCOL_ATOMS);

    Xutf8SetWMProperties(
        w->dpy, w->w, "runes", "runes", argv, argc,
        &normal_hints, &wm_hints, &class_hints);

    pid = getpid();
    XChangeProperty(
        w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_PID],
        XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&pid, 1);

    runes_window_backend_set_icon_name(t, "runes", 5);
    runes_window_backend_set_window_title(t, "runes", 5);

    cursor = XCreateFontCursor(w->dpy, XC_xterm);
    cairo_pattern_get_rgba(
        t->mousecursorcolor, &mouse_r, &mouse_g, &mouse_b, NULL);
    cursor_fg.red   = (unsigned short)(mouse_r * 65535);
    cursor_fg.green = (unsigned short)(mouse_g * 65535);
    cursor_fg.blue  = (unsigned short)(mouse_b * 65535);
    if ((mouse_r + mouse_g + mouse_b) / 3.0 > 0.5) {
        cursor_bg.red = cursor_bg.green = cursor_bg.blue = 0;
    }
    else {
        cursor_bg.red = cursor_bg.green = cursor_bg.blue = 65535;
    }
    XRecolorCursor(w->dpy, cursor, &cursor_fg, &cursor_bg);
    XDefineCursor(w->dpy, w->w, cursor);

    vis = DefaultVisual(w->dpy, DefaultScreen(w->dpy));
    surface = cairo_xlib_surface_create(
        w->dpy, w->w, vis, normal_hints.base_width, normal_hints.base_height);
    t->backend_cr = cairo_create(surface);
    cairo_surface_destroy(surface);

    XMapWindow(w->dpy, w->w);
}

void runes_window_backend_start_loop(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    unsigned long mask;
    void *data;

    XGetICValues(w->ic, XNFilterEvents, &mask, NULL);
    XSelectInput(
        w->dpy, w->w,
        mask|KeyPressMask|ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionHintMask|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|ExposureMask|FocusChangeMask);
    XSetICFocus(w->ic);

    data = malloc(sizeof(RunesXlibLoopData));
    ((RunesLoopData *)data)->req.data = data;
    ((RunesLoopData *)data)->t = t;

    uv_queue_work(
        t->loop, data,
        runes_window_backend_get_next_event,
        runes_window_backend_process_event);
}

void runes_window_backend_request_flush(RunesTerm *t)
{
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = t->w.w;
    e.xclient.format = 32;
    e.xclient.data.l[0] = t->w.atoms[RUNES_ATOM_RUNES_FLUSH];

    XSendEvent(t->w.dpy, t->w.w, False, NoEventMask, &e);
    XLockDisplay(t->w.dpy);
    XFlush(t->w.dpy);
    XUnlockDisplay(t->w.dpy);
}

void runes_window_backend_request_visual_bell(RunesTerm *t)
{
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = t->w.w;
    e.xclient.format = 32;
    e.xclient.data.l[0] = t->w.atoms[RUNES_ATOM_RUNES_VISUAL_BELL];

    XSendEvent(t->w.dpy, t->w.w, False, NoEventMask, &e);
    XLockDisplay(t->w.dpy);
    XFlush(t->w.dpy);
    XUnlockDisplay(t->w.dpy);
}

void runes_window_backend_request_audible_bell(RunesTerm *t)
{
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = t->w.w;
    e.xclient.format = 32;
    e.xclient.data.l[0] = t->w.atoms[RUNES_ATOM_RUNES_AUDIBLE_BELL];

    XSendEvent(t->w.dpy, t->w.w, False, NoEventMask, &e);
    XLockDisplay(t->w.dpy);
    XFlush(t->w.dpy);
    XUnlockDisplay(t->w.dpy);
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
    XLockDisplay(t->w.dpy);
    XFlush(t->w.dpy);
    XUnlockDisplay(t->w.dpy);
}

unsigned long runes_window_backend_get_window_id(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    return (unsigned long)w->w;
}

void runes_window_backend_get_size(RunesTerm *t, int *xpixel, int *ypixel)
{
    cairo_surface_t *surface;

    surface = cairo_get_target(t->backend_cr);
    *xpixel = cairo_xlib_surface_get_width(surface);
    *ypixel = cairo_xlib_surface_get_height(surface);
}

void runes_window_backend_set_icon_name(RunesTerm *t, char *name, size_t len)
{
    RunesWindowBackend *w = &t->w;

    XChangeProperty(
        w->dpy, w->w, XA_WM_ICON_NAME,
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_ICON_NAME],
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_backend_set_window_title(
    RunesTerm *t, char *name, size_t len)
{
    RunesWindowBackend *w = &t->w;

    XChangeProperty(
        w->dpy, w->w, XA_WM_NAME,
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->dpy, w->w, w->atoms[RUNES_ATOM_NET_WM_NAME],
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_backend_cleanup(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    XIM im;

    cairo_destroy(t->backend_cr);
    im = XIMOfIC(w->ic);
    XDestroyIC(w->ic);
    XCloseIM(im);
    XDestroyWindow(w->dpy, w->w);
    XCloseDisplay(w->dpy);
}

static void runes_window_backend_get_next_event(uv_work_t *req)
{
    RunesXlibLoopData *data;

    data = (RunesXlibLoopData *)req->data;
    XNextEvent(data->data.t->w.dpy, &data->e);
}

static void runes_window_backend_process_event(uv_work_t *req, int status)
{
    RunesXlibLoopData *data = req->data;
    XEvent *e = &data->e;
    RunesTerm *t = data->data.t;
    RunesWindowBackend *w = &t->w;
    int should_close = 0;

    UNUSED(status);

    if (!XFilterEvent(e, None)) {
        switch (e->type) {
        case KeyPress:
        case KeyRelease:
            runes_window_backend_handle_key_event(t, &e->xkey);
            break;
        case ButtonPress:
        case ButtonRelease:
            runes_window_backend_handle_button_event(t, &e->xbutton);
            break;
        case Expose:
            runes_window_backend_handle_expose_event(t, &e->xexpose);
            break;
        case ConfigureNotify:
            runes_window_backend_handle_configure_event(t, &e->xconfigure);
            break;
        case FocusIn:
        case FocusOut:
            runes_window_backend_handle_focus_event(t, &e->xfocus);
            break;
        case SelectionNotify:
            runes_window_backend_handle_selection_notify_event(
                t, &e->xselection);
            break;
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
            else if (a == w->atoms[RUNES_ATOM_RUNES_FLUSH]) {
                runes_window_backend_flush(t);
            }
            else if (a == w->atoms[RUNES_ATOM_RUNES_VISUAL_BELL]) {
                runes_window_backend_visual_bell(t);
            }
            else if (a == w->atoms[RUNES_ATOM_RUNES_AUDIBLE_BELL]) {
                runes_window_backend_audible_bell(t);
            }
            break;
        }
        default:
            break;
        }
    }

    if (!should_close) {
        uv_queue_work(
            t->loop, req, runes_window_backend_get_next_event,
            runes_window_backend_process_event);
    }
    else {
        runes_pty_backend_request_close(t);
        free(req);
    }
}

static void runes_window_backend_resize_window(
    RunesTerm *t, int width, int height)
{
    /* XXX no idea why shrinking the window dimensions to 0 makes xlib think
     * that the dimension is 65535 */
    if (width < 1 || width >= 65535) {
        width = 1;
    }
    if (height < 1 || height >= 65535) {
        height = 1;
    }

    if (width != t->xpixel || height != t->ypixel) {
        cairo_xlib_surface_set_size(
            cairo_get_target(t->backend_cr), width, height);
        runes_display_set_window_size(t);
    }
}

static void runes_window_backend_flush(RunesTerm *t)
{
    if (t->scr.audible_bell) {
        runes_window_backend_audible_bell(t);
        t->scr.audible_bell = 0;
    }

    if (t->scr.visual_bell) {
        runes_window_backend_visual_bell(t);
        t->scr.visual_bell = 0;
    }

    if (t->scr.update_title) {
        runes_window_backend_set_window_title(
            t, t->scr.title, t->scr.title_len);
        t->scr.update_title = 0;
    }

    if (t->scr.update_icon_name) {
        runes_window_backend_set_icon_name(
            t, t->scr.icon_name, t->scr.icon_name_len);
        t->scr.update_icon_name = 0;
    }

    if (t->visual_bell_is_ringing) {
        return;
    }

    cairo_set_source_surface(t->backend_cr, cairo_get_target(t->cr), 0.0, 0.0);
    cairo_paint(t->backend_cr);
    runes_window_backend_draw_cursor(t);
    cairo_surface_flush(cairo_get_target(t->backend_cr));
}

static void runes_window_backend_visual_bell(RunesTerm *t)
{
    if (t->bell_is_urgent) {
        runes_window_backend_set_urgent(t);
    }

    if (!t->visual_bell_is_ringing) {
        RunesWindowBackend *w = &t->w;
        uv_timer_t *timer_req;

        t->visual_bell_is_ringing = 1;
        cairo_set_source(t->backend_cr, t->fgdefault);
        cairo_paint(t->backend_cr);
        cairo_surface_flush(cairo_get_target(t->backend_cr));
        XFlush(w->dpy);

        timer_req = malloc(sizeof(uv_timer_t));
        uv_timer_init(t->loop, timer_req);
        timer_req->data = (void *)t;
        uv_timer_start(
            timer_req, runes_window_backend_reset_visual_bell, 20, 0);
    }
}

static void runes_window_backend_reset_visual_bell(uv_timer_t *handle)
{
    RunesTerm *t = handle->data;

    runes_window_backend_request_flush(t);
    t->visual_bell_is_ringing = 0;
    free(handle);
}

static void runes_window_backend_audible_bell(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    if (t->bell_is_urgent) {
        runes_window_backend_set_urgent(t);
    }
    XBell(w->dpy, 0);
}

static void runes_window_backend_draw_cursor(RunesTerm *t)
{
    if (!t->scr.hide_cursor) {
        cairo_save(t->backend_cr);
        cairo_set_source(t->backend_cr, t->cursorcolor);
        if (t->unfocused) {
            cairo_set_line_width(t->backend_cr, 1);
            cairo_rectangle(
                t->backend_cr,
                t->scr.cur.col * t->fontx + 0.5,
                t->scr.cur.row * t->fonty + 0.5,
                t->fontx, t->fonty);
            cairo_stroke(t->backend_cr);
        }
        else {
            cairo_rectangle(
                t->backend_cr,
                t->scr.cur.col * t->fontx, t->scr.cur.row * t->fonty,
                t->fontx, t->fonty);
            cairo_fill(t->backend_cr);
        }
        cairo_restore(t->backend_cr);
    }
}

static void runes_window_backend_set_urgent(RunesTerm *t)
{
    XWMHints *hints;

    hints = XGetWMHints(t->w.dpy, t->w.w);
    hints->flags |= XUrgencyHint;
    XSetWMHints(t->w.dpy, t->w.w, hints);
    XFree(hints);
}

static void runes_window_backend_clear_urgent(RunesTerm *t)
{
    XWMHints *hints;

    hints = XGetWMHints(t->w.dpy, t->w.w);
    hints->flags &= ~XUrgencyHint;
    XSetWMHints(t->w.dpy, t->w.w, hints);
    XFree(hints);
}

static void runes_window_backend_paste(RunesTerm *t, Time time)
{
    RunesWindowBackend *w = &t->w;

    XConvertSelection(
        w->dpy, XA_PRIMARY, w->atoms[RUNES_ATOM_UTF8_STRING],
        w->atoms[RUNES_ATOM_RUNES_SELECTION], w->w, time);
}

static void runes_window_backend_handle_key_event(RunesTerm *t, XKeyEvent *e)
{
    RunesWindowBackend *w = &t->w;
    char *buf;
    size_t len = 8;
    KeySym sym;
    Status s;
    size_t chars;

    if (e->type == KeyRelease) {
        return;
    }

    buf = malloc(len);

    for (;;) {
        chars = Xutf8LookupString(w->ic, e, buf, len - 1, &sym, &s);
        if (s == XBufferOverflow) {
            len = chars + 1;
            buf = realloc(buf, len);
            continue;
        }
        break;
    }

    switch (s) {
    case XLookupChars:
    case XLookupBoth:
        if (e->state & Mod1Mask) {
            runes_pty_backend_write(t, "\e", 1);
        }
        runes_pty_backend_write(t, buf, chars);
        break;
    case XLookupKeySym:
        if (!runes_window_backend_handle_builtin_keypress(t, sym, e)) {
            struct function_key *key;

            key = runes_window_backend_find_key_sequence(t, sym);
            if (key->sym != XK_VoidSymbol) {
                runes_pty_backend_write(t, key->str, key->len);
            }
        }
        break;
    default:
        break;
    }
    free(buf);
}

static int runes_window_backend_handle_builtin_keypress(
    RunesTerm *t, KeySym sym, XKeyEvent *e)
{
    switch (sym) {
    case XK_Insert:
        if (e->state & ShiftMask) {
            runes_window_backend_paste(t, e->time);
            return 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static struct function_key *runes_window_backend_find_key_sequence(
    RunesTerm *t, KeySym sym)
{
    struct function_key *key;

    if (t->scr.application_keypad) {
        if (t->scr.application_cursor) {
            key = &application_cursor_keys[0];
            while (key->sym != XK_VoidSymbol) {
                if (key->sym == sym) {
                    return key;
                }
                key++;
            }
        }
        key = &application_keypad_keys[0];
        while (key->sym != XK_VoidSymbol) {
            if (key->sym == sym) {
                return key;
            }
            key++;
        }
    }
    key = &keys[0];
    while (key->sym != XK_VoidSymbol) {
        if (key->sym == sym) {
            return key;
        }
        key++;
    }

    return key;
}

static void runes_window_backend_handle_button_event(
    RunesTerm *t, XButtonEvent *e)
{
    if (e->state & ShiftMask) {
        if (runes_window_backend_handle_builtin_button_press(t, e)) {
            return;
        }
    }

    if (t->scr.mouse_reporting_press_release) {
        char response[7];
        char status = 0;

        if (e->type == ButtonRelease && e->button > 3) {
            return;
        }

        if (e->type == ButtonRelease) {
            status = 3;
        }
        else {
            switch (e->button) {
            case Button1:
                status = 0;
                break;
            case Button2:
                status = 1;
                break;
            case Button3:
                status = 2;
                break;
            case Button4:
                status = 64;
                break;
            case Button5:
                status = 65;
                break;
            }
        }

        if (e->state & ShiftMask) {
            status |= 4;
        }
        if (e->state & Mod1Mask) {
            status |= 8;
        }
        if (e->state & ControlMask) {
            status |= 16;
        }

        sprintf(
            response, "\e[M%c%c%c",
            ' ' + (status),
            ' ' + (e->x / t->fontx + 1),
            ' ' + (e->y / t->fonty + 1));
        runes_pty_backend_write(t, response, 6);
    }
    else if (t->scr.mouse_reporting_press && e->type == ButtonPress) {
        char response[7];

        sprintf(
            response, "\e[M%c%c%c",
            ' ' + (e->button - 1),
            ' ' + (e->x / t->fontx + 1),
            ' ' + (e->y / t->fonty + 1));
        runes_pty_backend_write(t, response, 6);
    }
    else {
        runes_window_backend_handle_builtin_button_press(t, e);
    }
}

static int runes_window_backend_handle_builtin_button_press(
    RunesTerm *t, XButtonEvent *e)
{
    if (e->type == ButtonRelease) {
        return 0;
    }

    switch (e->button) {
    case Button2:
        runes_window_backend_paste(t, e->time);
        return 1;
        break;
    default:
        break;
    }

    return 0;
}

static void runes_window_backend_handle_expose_event(
    RunesTerm *t, XExposeEvent *e)
{
    UNUSED(e);

    runes_window_backend_flush(t);
}

static void runes_window_backend_handle_configure_event(
    RunesTerm *t, XConfigureEvent *e)
{
    RunesWindowBackend *w = &t->w;

    while (XCheckTypedWindowEvent(w->dpy, w->w, ConfigureNotify, (XEvent *)e));
    runes_window_backend_resize_window(t, e->width, e->height);
}

static void runes_window_backend_handle_focus_event(
    RunesTerm *t, XFocusChangeEvent *e)
{
    runes_window_backend_clear_urgent(t);
    if (e->type == FocusIn) {
        t->unfocused = 0;
    }
    else {
        t->unfocused = 1;
    }
    runes_window_backend_flush(t);
}

static void runes_window_backend_handle_selection_notify_event(
    RunesTerm *t, XSelectionEvent *e)
{
    RunesWindowBackend *w = &t->w;

    if (e->property == None) {
        if (e->target == w->atoms[RUNES_ATOM_UTF8_STRING]) {
            XConvertSelection(
                w->dpy, XA_PRIMARY, XA_STRING,
                w->atoms[RUNES_ATOM_RUNES_SELECTION], w->w, e->time);
        }
    }
    else {
        Atom type;
        int format;
        unsigned long nitems, left;
        unsigned char *buf;

        XGetWindowProperty(
            w->dpy, e->requestor, e->property, 0, 0x1fffffff, 0,
            AnyPropertyType, &type, &format, &nitems, &left, &buf);
        runes_pty_backend_write(t, (char *)buf, nitems);
        XFree(buf);
    }
}
