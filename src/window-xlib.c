#include <cairo-xlib.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "runes.h"
#include "window-xlib.h"
#include "loop.h"

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

static void runes_window_backend_get_next_event(RunesTerm *t);
static int runes_window_backend_process_event(RunesTerm *t);
static Bool runes_window_backend_find_flush_events(
    Display *dpy, XEvent *e, XPointer arg);
static void runes_window_backend_resize_window(
    RunesTerm *t, int width, int height);
static void runes_window_backend_flush(RunesTerm *t);
static int runes_window_backend_check_recent(RunesTerm *t);
static void runes_window_backend_delay_cb(RunesTerm *t);
static void runes_window_backend_visible_scroll(RunesTerm *t, int count);
static void runes_window_backend_visual_bell(RunesTerm *t);
static void runes_window_backend_reset_visual_bell(RunesTerm *t);
static void runes_window_backend_audible_bell(RunesTerm *t);
static void runes_window_backend_set_urgent(RunesTerm *t);
static void runes_window_backend_clear_urgent(RunesTerm *t);
static void runes_window_backend_paste(RunesTerm *t, Time time);
static void runes_window_backend_start_selection(
    RunesTerm *t, int xpixel, int ypixel, Time time);
static void runes_window_backend_update_selection(
    RunesTerm *t, int xpixel, int ypixel);
static void runes_window_backend_clear_selection(RunesTerm *t);
static void runes_window_backend_handle_key_event(RunesTerm *t, XKeyEvent *e);
static void runes_window_backend_handle_button_event(
    RunesTerm *t, XButtonEvent *e);
static void runes_window_backend_handle_motion_event(
    RunesTerm *t, XMotionEvent *e);
static void runes_window_backend_handle_expose_event(
    RunesTerm *t, XExposeEvent *e);
static void runes_window_backend_handle_configure_event(
    RunesTerm *t, XConfigureEvent *e);
static void runes_window_backend_handle_focus_event(
    RunesTerm *t, XFocusChangeEvent *e);
static void runes_window_backend_handle_selection_notify_event(
    RunesTerm *t, XSelectionEvent *e);
static void runes_window_backend_handle_selection_clear_event(
    RunesTerm *t, XSelectionClearEvent *e);
static void runes_window_backend_handle_selection_request_event(
    RunesTerm *t, XSelectionRequestEvent *e);
static int runes_window_backend_handle_builtin_keypress(
    RunesTerm *t, KeySym sym, XKeyEvent *e);
static int runes_window_backend_handle_builtin_button_press(
    RunesTerm *t, XButtonEvent *e);
static struct function_key *runes_window_backend_find_key_sequence(
    RunesTerm *t, KeySym sym);
static struct vt100_loc runes_window_backend_get_mouse_position(
    RunesTerm *t, int xpixel, int ypixel);

void runes_window_backend_create_window(RunesTerm *t, int argc, char *argv[])
{
    RunesWindowBackend *w = &t->w;
    pid_t pid;
    XClassHint class_hints = { "runes", "runes" };
    XWMHints wm_hints;
    XSizeHints normal_hints;
    double bg_r, bg_g, bg_b;
    XColor bgcolor;
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

    normal_hints.min_width   = t->display.fontx + 4;
    normal_hints.min_height  = t->display.fonty + 4;
    normal_hints.width_inc   = t->display.fontx;
    normal_hints.height_inc  = t->display.fonty;
    normal_hints.base_width  = t->display.fontx * t->config.default_cols + 4;
    normal_hints.base_height = t->display.fonty * t->config.default_rows + 4;

    cairo_pattern_get_rgba(t->config.bgdefault, &bg_r, &bg_g, &bg_b, NULL);
    bgcolor.red   = bg_r * 65535;
    bgcolor.green = bg_g * 65535;
    bgcolor.blue  = bg_b * 65535;

    XInitThreads();

    w->dpy = XOpenDisplay(NULL);
    XAllocColor(
        w->dpy, DefaultColormap(w->dpy, DefaultScreen(w->dpy)), &bgcolor);
    w->border_w = XCreateSimpleWindow(
        w->dpy, DefaultRootWindow(w->dpy),
        0, 0, normal_hints.base_width, normal_hints.base_height,
        0, bgcolor.pixel, bgcolor.pixel);
    w->w = XCreateSimpleWindow(
        w->dpy, w->border_w,
        2, 2, normal_hints.base_width - 4, normal_hints.base_height - 4,
        0, bgcolor.pixel, bgcolor.pixel);

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
        runes_die("failed");
    }

    XInternAtoms(w->dpy, atom_names, RUNES_NUM_ATOMS, False, w->atoms);
    XSetWMProtocols(w->dpy, w->border_w, w->atoms, RUNES_NUM_PROTOCOL_ATOMS);

    Xutf8SetWMProperties(
        w->dpy, w->border_w, "runes", "runes", argv, argc,
        &normal_hints, &wm_hints, &class_hints);

    pid = getpid();
    XChangeProperty(
        w->dpy, w->border_w, w->atoms[RUNES_ATOM_NET_WM_PID],
        XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&pid, 1);

    runes_window_backend_set_icon_name(t, "runes", 5);
    runes_window_backend_set_window_title(t, "runes", 5);

    cursor = XCreateFontCursor(w->dpy, XC_xterm);
    cairo_pattern_get_rgba(
        t->config.mousecursorcolor, &mouse_r, &mouse_g, &mouse_b, NULL);
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
    w->backend_cr = cairo_create(surface);
    cairo_surface_destroy(surface);

    XMapWindow(w->dpy, w->w);
    XMapWindow(w->dpy, w->border_w);
}

void runes_window_backend_init_loop(RunesTerm *t, RunesLoop *loop)
{
    RunesWindowBackend *w = &t->w;
    unsigned long xim_mask, common_mask;

    XGetICValues(w->ic, XNFilterEvents, &xim_mask, NULL);
    /* we always want to receive keyboard events, and enter/leave window events
     * are what allows keyboard focus switching to work when the mouse is over
     * the window but hidden, for whatever reason */
    common_mask = KeyPressMask|EnterWindowMask|LeaveWindowMask;

    /* the top level window is the only one that needs to worry about window
     * size and focus changes */
    XSelectInput(
        w->dpy, w->border_w,
        xim_mask|common_mask|StructureNotifyMask|FocusChangeMask);
    /* we only care about mouse events if they are over the actual terminal
     * area, and the terminal area is the only area we need to redraw, so it's
     * the only thing we care about exposure events for */
    XSelectInput(
        w->dpy, w->w,
        xim_mask|common_mask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ExposureMask);
    XSetICFocus(w->ic);

    runes_loop_start_work(loop, t, runes_window_backend_get_next_event,
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
    RunesWindowBackend *w = &t->w;
    cairo_surface_t *surface;

    surface = cairo_get_target(w->backend_cr);
    *xpixel = cairo_xlib_surface_get_width(surface);
    *ypixel = cairo_xlib_surface_get_height(surface);
}

void runes_window_backend_set_icon_name(RunesTerm *t, char *name, size_t len)
{
    RunesWindowBackend *w = &t->w;

    XChangeProperty(
        w->dpy, w->border_w, XA_WM_ICON_NAME,
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->dpy, w->border_w, w->atoms[RUNES_ATOM_NET_WM_ICON_NAME],
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_backend_set_window_title(
    RunesTerm *t, char *name, size_t len)
{
    RunesWindowBackend *w = &t->w;

    XChangeProperty(
        w->dpy, w->border_w, XA_WM_NAME,
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->dpy, w->border_w, w->atoms[RUNES_ATOM_NET_WM_NAME],
        w->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_backend_cleanup(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    XIM im;

    cairo_destroy(w->backend_cr);
    im = XIMOfIC(w->ic);
    XDestroyIC(w->ic);
    XCloseIM(im);
    XDestroyWindow(w->dpy, w->w);
    XDestroyWindow(w->dpy, w->border_w);
    XCloseDisplay(w->dpy);
}

static void runes_window_backend_get_next_event(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    XNextEvent(w->dpy, &w->event);
}

static int runes_window_backend_process_event(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    XEvent *e = &w->event;
    int should_close = 0;

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
        case MotionNotify:
            runes_window_backend_handle_motion_event(t, &e->xmotion);
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
        case SelectionRequest:
            runes_window_backend_handle_selection_request_event(
                t, &e->xselectionrequest);
            break;
        case SelectionClear:
            runes_window_backend_handle_selection_clear_event(
                t, &e->xselectionclear);
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
                Bool res = True;

                while (res) {
                    res = XCheckIfEvent(
                        w->dpy, e, runes_window_backend_find_flush_events,
                        (XPointer)w);
                }
                runes_window_backend_flush(t);
            }
            break;
        }
        default:
            break;
        }
    }

    if (should_close) {
        runes_pty_backend_request_close(t);
    }

    return !should_close;
}

static Bool runes_window_backend_find_flush_events(
    Display *dpy, XEvent *e, XPointer arg)
{
    RunesWindowBackend *w = (RunesWindowBackend *)arg;

    UNUSED(dpy);

    if (e->type == ClientMessage) {
        Atom a = e->xclient.data.l[0];
        return a == w->atoms[RUNES_ATOM_RUNES_FLUSH] ? True : False;
    }
    else {
        return False;
    }
}

static void runes_window_backend_resize_window(
    RunesTerm *t, int width, int height)
{
    RunesWindowBackend *w = &t->w;

    /* XXX no idea why shrinking the window dimensions to 0 makes xlib think
     * that the dimension is 65535 */
    if (width < 1 || width >= 65535) {
        width = 1;
    }
    if (height < 1 || height >= 65535) {
        height = 1;
    }

    if (width != t->display.xpixel || height != t->display.ypixel) {
        int dwidth = width - 4, dheight = height - 4;

        XResizeWindow(w->dpy, w->w, dwidth, dheight);
        cairo_xlib_surface_set_size(
            cairo_get_target(w->backend_cr), dwidth, dheight);
        runes_term_set_window_size(t, dwidth, dheight);
        runes_window_backend_clear_selection(t);
    }
}

static void runes_window_backend_flush(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    if (runes_window_backend_check_recent(t)) {
        return;
    }

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

    if (w->visual_bell_is_ringing) {
        return;
    }

    runes_display_draw_screen(t);

    cairo_set_source_surface(
        w->backend_cr, cairo_get_target(t->display.cr), 0.0, 0.0);
    cairo_paint(w->backend_cr);
    runes_display_draw_cursor(t, w->backend_cr);
    cairo_surface_flush(cairo_get_target(w->backend_cr));

    clock_gettime(CLOCK_REALTIME, &w->last_redraw);
}

static int runes_window_backend_check_recent(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;
    struct timespec now;
    int rate = t->config.redraw_rate;

    if (rate == 0) {
        return 0;
    }

    clock_gettime(CLOCK_REALTIME, &now);
    now.tv_nsec -= rate * 1000000;
    if (now.tv_nsec < 0) {
        now.tv_sec -= 1;
        now.tv_nsec += 1000000000;
    }
    if (now.tv_sec < w->last_redraw.tv_sec || (now.tv_sec == w->last_redraw.tv_sec && now.tv_nsec < w->last_redraw.tv_nsec)) {
        if (!w->delaying) {
            runes_loop_timer_set(
                t->loop, rate, 0, t, runes_window_backend_delay_cb);
            w->delaying = 1;
        }
        return 1;
    }
    else {
        return 0;
    }

}

static void runes_window_backend_delay_cb(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    w->delaying = 0;
    runes_window_backend_request_flush(t);
}

static void runes_window_backend_visible_scroll(RunesTerm *t, int count)
{
    int min = 0, max = t->scr.grid->row_count - t->scr.grid->max.row;
    int old_offset = t->display.row_visible_offset;

    t->display.row_visible_offset += count;
    if (t->display.row_visible_offset < min) {
        t->display.row_visible_offset = min;
    }
    if (t->display.row_visible_offset > max) {
        t->display.row_visible_offset = max;
    }

    if (t->display.row_visible_offset == old_offset) {
        return;
    }

    t->display.dirty = 1;
    runes_window_backend_flush(t);
}

static void runes_window_backend_visual_bell(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    if (t->config.bell_is_urgent) {
        runes_window_backend_set_urgent(t);
    }

    if (!w->visual_bell_is_ringing) {
        w->visual_bell_is_ringing = 1;
        cairo_set_source(w->backend_cr, t->config.fgdefault);
        cairo_paint(w->backend_cr);
        cairo_surface_flush(cairo_get_target(w->backend_cr));
        XFlush(w->dpy);

        runes_loop_timer_set(t->loop, 20, 0, t,
                             runes_window_backend_reset_visual_bell);
    }
}

static void runes_window_backend_reset_visual_bell(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    runes_window_backend_request_flush(t);
    w->visual_bell_is_ringing = 0;
}

static void runes_window_backend_audible_bell(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    if (t->config.audible_bell) {
        if (t->config.bell_is_urgent) {
            runes_window_backend_set_urgent(t);
        }
        XBell(w->dpy, 0);
    }
    else {
        runes_window_backend_visual_bell(t);
    }
}

static void runes_window_backend_set_urgent(RunesTerm *t)
{
    XWMHints *hints;

    hints = XGetWMHints(t->w.dpy, t->w.border_w);
    hints->flags |= XUrgencyHint;
    XSetWMHints(t->w.dpy, t->w.border_w, hints);
    XFree(hints);
}

static void runes_window_backend_clear_urgent(RunesTerm *t)
{
    XWMHints *hints;

    hints = XGetWMHints(t->w.dpy, t->w.border_w);
    hints->flags &= ~XUrgencyHint;
    XSetWMHints(t->w.dpy, t->w.border_w, hints);
    XFree(hints);
}

static void runes_window_backend_paste(RunesTerm *t, Time time)
{
    RunesWindowBackend *w = &t->w;

    XConvertSelection(
        w->dpy, XA_PRIMARY, w->atoms[RUNES_ATOM_UTF8_STRING],
        w->atoms[RUNES_ATOM_RUNES_SELECTION], w->w, time);
}

static void runes_window_backend_start_selection(
    RunesTerm *t, int xpixel, int ypixel, Time time)
{
    RunesWindowBackend *w = &t->w;
    struct vt100_loc *start = &t->display.selection_start;
    struct vt100_loc *end   = &t->display.selection_end;

    *start = runes_window_backend_get_mouse_position(t, xpixel, ypixel);
    *end   = *start;

    XSetSelectionOwner(w->dpy, XA_PRIMARY, w->w, time);
    t->display.has_selection = (XGetSelectionOwner(w->dpy, XA_PRIMARY) == w->w);

    t->display.dirty = 1;
    runes_window_backend_request_flush(t);
}

static void runes_window_backend_update_selection(
    RunesTerm *t, int xpixel, int ypixel)
{
    RunesWindowBackend *w = &t->w;
    struct vt100_loc *start = &t->display.selection_start;
    struct vt100_loc *end = &t->display.selection_end;
    struct vt100_loc orig_end = *end;

    if (!t->display.has_selection) {
        return;
    }

    *end = runes_window_backend_get_mouse_position(t, xpixel, ypixel);

    if (orig_end.row != end->row || orig_end.col != end->col) {
        if (end->row < start->row || (end->row == start->row && end->col < start->col)) {
            struct vt100_loc *tmp;

            tmp = start;
            start = end;
            end = tmp;
        }

        if (w->selection_contents) {
            free(w->selection_contents);
            w->selection_contents = NULL;
        }
        vt100_screen_get_string_plaintext(
            &t->scr, start, end, &w->selection_contents, &w->selection_len);
        t->display.dirty = 1;
        runes_window_backend_request_flush(t);
    }
}

static void runes_window_backend_clear_selection(RunesTerm *t)
{
    RunesWindowBackend *w = &t->w;

    XSetSelectionOwner(w->dpy, XA_PRIMARY, None, CurrentTime);
    t->display.has_selection = 0;
    if (w->selection_contents) {
        free(w->selection_contents);
        w->selection_contents = NULL;
    }
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
        struct vt100_loc loc;

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

        loc = runes_window_backend_get_mouse_position(t, e->x, e->y);
        sprintf(
            response, "\e[M%c%c%c",
            ' ' + (status), ' ' + loc.col + 1, ' ' + loc.row + 1);
        runes_pty_backend_write(t, response, 6);
    }
    else if (t->scr.mouse_reporting_press && e->type == ButtonPress) {
        char response[7];
        struct vt100_loc loc;

        loc = runes_window_backend_get_mouse_position(t, e->x, e->y);
        sprintf(
            response, "\e[M%c%c%c",
            ' ' + (e->button - 1), ' ' + loc.col + 1, ' ' + loc.row + 1);
        runes_pty_backend_write(t, response, 6);
    }
    else {
        runes_window_backend_handle_builtin_button_press(t, e);
    }
}

static void runes_window_backend_handle_motion_event(
    RunesTerm *t, XMotionEvent *e)
{
    if (!(e->state & Button1Mask)) {
        return;
    }

    runes_window_backend_update_selection(t, e->x, e->y);
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

    if (e->window != w->border_w) {
        return;
    }

    while (XCheckTypedWindowEvent(
        w->dpy, w->border_w, ConfigureNotify, (XEvent *)e));
    runes_window_backend_resize_window(t, e->width, e->height);
}

static void runes_window_backend_handle_focus_event(
    RunesTerm *t, XFocusChangeEvent *e)
{
    /* we don't care about focus events that are only sent because the pointer
     * is in the window, if focus is changing between two other unrelated
     * windows */
    if (e->detail == NotifyPointer) {
        return;
    }

    if (e->type == FocusIn && !t->display.unfocused) {
        return;
    }
    if (e->type == FocusOut && t->display.unfocused) {
        return;
    }

    runes_window_backend_clear_urgent(t);
    if (e->type == FocusIn) {
        t->display.unfocused = 0;
    }
    else {
        t->display.unfocused = 1;
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
        if (t->scr.bracketed_paste) {
            runes_pty_backend_write(t, "\e[200~", 6);
        }
        runes_pty_backend_write(t, (char *)buf, nitems);
        if (t->scr.bracketed_paste) {
            runes_pty_backend_write(t, "\e[201~", 6);
        }
        XFree(buf);
    }
}

static void runes_window_backend_handle_selection_clear_event(
    RunesTerm *t, XSelectionClearEvent *e)
{
    UNUSED(e);

    t->display.has_selection = 0;
    t->display.dirty = 1;
    runes_window_backend_flush(t);
}

static void runes_window_backend_handle_selection_request_event(
    RunesTerm *t, XSelectionRequestEvent *e)
{
    RunesWindowBackend *w = &t->w;
    XSelectionEvent selection;

    selection.type       = SelectionNotify;
    selection.serial     = e->serial;
    selection.send_event = e->send_event;
    selection.display    = e->display;
    selection.requestor  = e->requestor;
    selection.selection  = e->selection;
    selection.target     = e->target;
    selection.property   = e->property;
    selection.time       = e->time;

    if (e->target == w->atoms[RUNES_ATOM_TARGETS]) {
        Atom targets[2] = { XA_STRING, w->atoms[RUNES_ATOM_UTF8_STRING] };

        XChangeProperty(
            w->dpy, e->requestor, e->property,
            XA_ATOM, 32, PropModeReplace,
            (unsigned char *)&targets, 2);
    }
    else if (e->target == XA_STRING || e->target == w->atoms[RUNES_ATOM_UTF8_STRING]) {
        if (w->selection_contents) {
            XChangeProperty(
                w->dpy, e->requestor, e->property,
                e->target, 8, PropModeReplace,
                (unsigned char *)w->selection_contents, w->selection_len);
        }
    }
    else {
        selection.property = None;
    }

    XSendEvent(w->dpy, e->requestor, False, NoEventMask, (XEvent *)&selection);
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
    case XK_Page_Up:
        if (e->state & ShiftMask) {
            runes_window_backend_visible_scroll(
                t, t->scr.grid->max.row - 1);
            return 1;
        }
        break;
    case XK_Page_Down:
        if (e->state & ShiftMask) {
            runes_window_backend_visible_scroll(
                t, -(t->scr.grid->max.row - 1));
            return 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int runes_window_backend_handle_builtin_button_press(
    RunesTerm *t, XButtonEvent *e)
{
    if (e->type != ButtonRelease) {
        switch (e->button) {
        case Button1:
            runes_window_backend_start_selection(t, e->x, e->y, e->time);
            return 1;
            break;
        case Button2:
            runes_window_backend_paste(t, e->time);
            return 1;
            break;
        case Button4:
            runes_window_backend_visible_scroll(t, t->config.scroll_lines);
            return 1;
            break;
        case Button5:
            runes_window_backend_visible_scroll(t, -t->config.scroll_lines);
            return 1;
            break;
        default:
            break;
        }
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

static struct vt100_loc runes_window_backend_get_mouse_position(
    RunesTerm *t, int xpixel, int ypixel)
{
    struct vt100_loc ret;

    ret.row = ypixel / t->display.fonty;
    ret.col = xpixel / t->display.fontx;

    ret.row = ret.row < 0                        ? 0
            : ret.row > t->scr.grid->max.row - 1 ? t->scr.grid->max.row - 1
            :                                      ret.row;
    ret.col = ret.col < 0                    ? 0
            : ret.col > t->scr.grid->max.col ? t->scr.grid->max.col
            :                                  ret.col;

    ret.row = ret.row - t->display.row_visible_offset + t->scr.grid->row_count - t->scr.grid->max.row;

    return ret;
}
