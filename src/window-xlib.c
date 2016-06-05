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

#include "config.h"
#include "display.h"
#include "loop.h"
#include "pty-unix.h"
#include "term.h"
#include "window-backend-xlib.h"

struct function_key {
    KeySym sym;
    char *str;
    size_t len;
};

#define RUNES_KEY(sym, str) { sym, str, sizeof(str) - 1 }
static struct function_key keys[] = {
    RUNES_KEY(XK_Up,         "\033[A"),
    RUNES_KEY(XK_Down,       "\033[B"),
    RUNES_KEY(XK_Right,      "\033[C"),
    RUNES_KEY(XK_Left,       "\033[D"),
    RUNES_KEY(XK_Page_Up,    "\033[5~"),
    RUNES_KEY(XK_Page_Down,  "\033[6~"),
    RUNES_KEY(XK_Home,       "\033[H"),
    RUNES_KEY(XK_End,        "\033[F"),
    RUNES_KEY(XK_F1,         "\033OP"),
    RUNES_KEY(XK_F2,         "\033OQ"),
    RUNES_KEY(XK_F3,         "\033OR"),
    RUNES_KEY(XK_F4,         "\033OS"),
    RUNES_KEY(XK_F5,         "\033[15~"),
    RUNES_KEY(XK_F6,         "\033[17~"),
    RUNES_KEY(XK_F7,         "\033[18~"),
    RUNES_KEY(XK_F8,         "\033[19~"),
    RUNES_KEY(XK_F9,         "\033[20~"),
    RUNES_KEY(XK_F10,        "\033[21~"),
    RUNES_KEY(XK_F11,        "\033[23~"),
    RUNES_KEY(XK_F12,        "\033[24~"),
    RUNES_KEY(XK_F13,        "\033[25~"),
    RUNES_KEY(XK_F14,        "\033[26~"),
    RUNES_KEY(XK_F15,        "\033[28~"),
    RUNES_KEY(XK_F16,        "\033[29~"),
    RUNES_KEY(XK_F17,        "\033[31~"),
    RUNES_KEY(XK_F18,        "\033[32~"),
    RUNES_KEY(XK_F19,        "\033[33~"),
    RUNES_KEY(XK_F20,        "\033[34~"),
    RUNES_KEY(XK_BackSpace,  "\x7f"),
    RUNES_KEY(XK_Delete,     "\033[3~"),
    /* XXX keypad keys need to go here too */
    RUNES_KEY(XK_VoidSymbol, "")
};

static struct function_key application_keypad_keys[] = {
    /* XXX i don't have a keypad on my laptop, need to get one for testing */
    RUNES_KEY(XK_VoidSymbol, "")
};

static struct function_key application_cursor_keys[] = {
    RUNES_KEY(XK_Up,    "\033OA"),
    RUNES_KEY(XK_Down,  "\033OB"),
    RUNES_KEY(XK_Right, "\033OC"),
    RUNES_KEY(XK_Left,  "\033OD"),
    /* XXX home/end? */
    RUNES_KEY(XK_VoidSymbol, "")
};
#undef RUNES_KEY

static int runes_window_event_cb(void *t);
static int runes_window_handle_event(RunesTerm *t, XEvent *e);
static Bool runes_window_wants_event(
    Display *dpy, XEvent *event, XPointer arg);
static void runes_window_resize_window(
    RunesTerm *t, int width, int height);
static void runes_window_write_to_pty(
    RunesTerm *t, char *buf, size_t len);
static int runes_window_check_recent(RunesTerm *t);
static void runes_window_delay_cb(void *t);
static void runes_window_visible_scroll(RunesTerm *t, int count);
static void runes_window_visual_bell(RunesTerm *t);
static void runes_window_reset_visual_bell(void *t);
static void runes_window_audible_bell(RunesTerm *t);
static void runes_window_set_urgent(RunesTerm *t);
static void runes_window_clear_urgent(RunesTerm *t);
static void runes_window_paste(RunesTerm *t, Time time);
static void runes_window_start_selection(
    RunesTerm *t, int xpixel, int ypixel);
static void runes_window_start_selection_loc(
    RunesTerm *t, struct vt100_loc *start);
static void runes_window_update_selection(
    RunesTerm *t, int xpixel, int ypixel, Time time);
static void runes_window_update_selection_loc(
    RunesTerm *t, struct vt100_loc *end, Time time);
static void runes_window_acquire_selection(RunesTerm *t, Time time);
static void runes_window_clear_selection(RunesTerm *t);
static void runes_window_handle_key_event(RunesTerm *t, XKeyEvent *e);
static void runes_window_handle_button_event(RunesTerm *t, XButtonEvent *e);
static void runes_window_handle_motion_event(RunesTerm *t, XMotionEvent *e);
static void runes_window_handle_expose_event(RunesTerm *t, XExposeEvent *e);
static void runes_window_handle_configure_event(
    RunesTerm *t, XConfigureEvent *e);
static void runes_window_handle_focus_event(
    RunesTerm *t, XFocusChangeEvent *e);
static void runes_window_handle_selection_notify_event(
    RunesTerm *t, XSelectionEvent *e);
static void runes_window_handle_selection_clear_event(
    RunesTerm *t, XSelectionClearEvent *e);
static void runes_window_handle_selection_request_event(
    RunesTerm *t, XSelectionRequestEvent *e);
static int runes_window_handle_builtin_keypress(
    RunesTerm *t, KeySym sym, XKeyEvent *e);
static int runes_window_handle_builtin_button_press(
    RunesTerm *t, XButtonEvent *e);
static void runes_window_handle_multi_click(RunesTerm *t, XButtonEvent *e);
static void runes_window_beginning_of_word(RunesTerm *t, struct vt100_loc *loc);
static void runes_window_end_of_word(RunesTerm *t, struct vt100_loc *loc);
static int runes_window_is_word_char(char *buf, size_t len);
static void runes_window_multi_click_cb(void *t);
static struct function_key *runes_window_find_key_sequence(
    RunesTerm *t, KeySym sym);
static struct vt100_loc runes_window_get_mouse_position(
    RunesTerm *t, int xpixel, int ypixel);

RunesWindow *runes_window_new(RunesWindowBackend *wb)
{
    RunesWindow *w;

    w = calloc(1, sizeof(RunesWindow));
    w->wb = wb;

    return w;
}

void runes_window_create_window(RunesTerm *t, int argc, char *argv[])
{
    RunesWindow *w = t->w;
    pid_t pid;
    XClassHint class_hints = { RUNES_PROGRAM_NAME, RUNES_PROGRAM_NAME };
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

    normal_hints.min_width   = t->display->fontx + 4;
    normal_hints.min_height  = t->display->fonty + 4;
    normal_hints.width_inc   = t->display->fontx;
    normal_hints.height_inc  = t->display->fonty;
    normal_hints.base_width  = t->display->fontx * t->config->default_cols + 4;
    normal_hints.base_height = t->display->fonty * t->config->default_rows + 4;

    cairo_pattern_get_rgba(t->config->bgdefault, &bg_r, &bg_g, &bg_b, NULL);
    bgcolor.red   = bg_r * 65535;
    bgcolor.green = bg_g * 65535;
    bgcolor.blue  = bg_b * 65535;

    XAllocColor(
        w->wb->dpy, DefaultColormap(w->wb->dpy, DefaultScreen(w->wb->dpy)),
        &bgcolor);
    w->border_w = XCreateSimpleWindow(
        w->wb->dpy, DefaultRootWindow(w->wb->dpy),
        0, 0, normal_hints.base_width, normal_hints.base_height,
        0, bgcolor.pixel, bgcolor.pixel);
    w->w = XCreateSimpleWindow(
        w->wb->dpy, w->border_w,
        2, 2, normal_hints.base_width - 4, normal_hints.base_height - 4,
        0, bgcolor.pixel, bgcolor.pixel);

    im = XOpenIM(w->wb->dpy, NULL, NULL, NULL);
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

    XSetWMProtocols(
        w->wb->dpy, w->border_w, w->wb->atoms, RUNES_NUM_PROTOCOL_ATOMS);

    Xutf8SetWMProperties(
        w->wb->dpy, w->border_w, RUNES_PROGRAM_NAME, RUNES_PROGRAM_NAME,
        argv, argc,
        &normal_hints, &wm_hints, &class_hints);

    pid = getpid();
    XChangeProperty(
        w->wb->dpy, w->border_w, w->wb->atoms[RUNES_ATOM_NET_WM_PID],
        XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&pid, 1);

    runes_window_set_icon_name(t, RUNES_PROGRAM_NAME, 5);
    runes_window_set_window_title(t, RUNES_PROGRAM_NAME, 5);

    cursor = XCreateFontCursor(w->wb->dpy, XC_xterm);
    cairo_pattern_get_rgba(
        t->config->mousecursorcolor, &mouse_r, &mouse_g, &mouse_b, NULL);
    cursor_fg.red   = (unsigned short)(mouse_r * 65535);
    cursor_fg.green = (unsigned short)(mouse_g * 65535);
    cursor_fg.blue  = (unsigned short)(mouse_b * 65535);
    if ((mouse_r + mouse_g + mouse_b) / 3.0 > 0.5) {
        cursor_bg.red = cursor_bg.green = cursor_bg.blue = 0;
    }
    else {
        cursor_bg.red = cursor_bg.green = cursor_bg.blue = 65535;
    }
    XRecolorCursor(w->wb->dpy, cursor, &cursor_fg, &cursor_bg);
    XDefineCursor(w->wb->dpy, w->w, cursor);

    vis = DefaultVisual(w->wb->dpy, DefaultScreen(w->wb->dpy));
    surface = cairo_xlib_surface_create(
        w->wb->dpy, w->w, vis, normal_hints.base_width,
        normal_hints.base_height);
    w->backend_cr = cairo_create(surface);
    cairo_surface_destroy(surface);

    XMapWindow(w->wb->dpy, w->w);
    XMapWindow(w->wb->dpy, w->border_w);
}

void runes_window_init_loop(RunesTerm *t, RunesLoop *loop)
{
    RunesWindow *w = t->w;
    unsigned long xim_mask, common_mask;

    XGetICValues(w->ic, XNFilterEvents, &xim_mask, NULL);
    /* we always want to receive keyboard events, and enter/leave window events
     * are what allows keyboard focus switching to work when the mouse is over
     * the window but hidden, for whatever reason */
    common_mask = KeyPressMask|EnterWindowMask|LeaveWindowMask;

    /* the top level window is the only one that needs to worry about window
     * size and focus changes */
    XSelectInput(
        w->wb->dpy, w->border_w,
        xim_mask|common_mask|StructureNotifyMask|FocusChangeMask);
    /* we only care about mouse events if they are over the actual terminal
     * area, and the terminal area is the only area we need to redraw, so it's
     * the only thing we care about exposure events for */
    XSelectInput(
        w->wb->dpy, w->w,
        xim_mask|common_mask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ExposureMask);
    XSetICFocus(w->ic);

    runes_term_refcnt_inc(t);
    runes_loop_start_work(
        loop, XConnectionNumber(w->wb->dpy), t, runes_window_event_cb);
}

void runes_window_flush(RunesTerm *t)
{
    RunesWindow *w = t->w;

    if (runes_window_check_recent(t)) {
        return;
    }

    if (t->scr->audible_bell) {
        runes_window_audible_bell(t);
        t->scr->audible_bell = 0;
    }

    if (t->scr->visual_bell) {
        runes_window_visual_bell(t);
        t->scr->visual_bell = 0;
    }

    if (t->scr->update_title) {
        runes_window_set_window_title(t, t->scr->title, t->scr->title_len);
        t->scr->update_title = 0;
    }

    if (t->scr->update_icon_name) {
        runes_window_set_icon_name(
            t, t->scr->icon_name, t->scr->icon_name_len);
        t->scr->update_icon_name = 0;
    }

    if (w->visual_bell_is_ringing) {
        return;
    }

    runes_display_draw_screen(t);
    runes_display_draw_cursor(t);
    cairo_surface_flush(cairo_get_target(w->backend_cr));
    XFlush(w->wb->dpy);

    clock_gettime(CLOCK_REALTIME, &w->last_redraw);
}

void runes_window_request_close(RunesTerm *t)
{
    RunesWindow *w = t->w;
    XEvent e;

    e.xclient.type = ClientMessage;
    e.xclient.window = w->w;
    e.xclient.message_type = w->wb->atoms[RUNES_ATOM_WM_PROTOCOLS];
    e.xclient.format = 32;
    e.xclient.data.l[0] = w->wb->atoms[RUNES_ATOM_WM_DELETE_WINDOW];
    e.xclient.data.l[1] = CurrentTime;

    XSendEvent(w->wb->dpy, w->w, False, NoEventMask, &e);
    XFlush(w->wb->dpy);
}

unsigned long runes_window_get_window_id(RunesTerm *t)
{
    RunesWindow *w = t->w;

    return (unsigned long)w->w;
}

void runes_window_get_size(RunesTerm *t, int *xpixel, int *ypixel)
{
    RunesWindow *w = t->w;
    cairo_surface_t *surface;

    surface = cairo_get_target(w->backend_cr);
    *xpixel = cairo_xlib_surface_get_width(surface);
    *ypixel = cairo_xlib_surface_get_height(surface);
}

void runes_window_set_icon_name(RunesTerm *t, char *name, size_t len)
{
    RunesWindow *w = t->w;

    XChangeProperty(
        w->wb->dpy, w->border_w, XA_WM_ICON_NAME,
        w->wb->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->wb->dpy, w->border_w, w->wb->atoms[RUNES_ATOM_NET_WM_ICON_NAME],
        w->wb->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_set_window_title(RunesTerm *t, char *name, size_t len)
{
    RunesWindow *w = t->w;

    XChangeProperty(
        w->wb->dpy, w->border_w, XA_WM_NAME,
        w->wb->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
    XChangeProperty(
        w->wb->dpy, w->border_w, w->wb->atoms[RUNES_ATOM_NET_WM_NAME],
        w->wb->atoms[RUNES_ATOM_UTF8_STRING], 8, PropModeReplace,
        (unsigned char *)name, len);
}

void runes_window_delete(RunesWindow *w)
{
    XIM im;

    cairo_destroy(w->backend_cr);
    im = XIMOfIC(w->ic);
    XDestroyIC(w->ic);
    XCloseIM(im);
    XDestroyWindow(w->wb->dpy, w->w);
    XDestroyWindow(w->wb->dpy, w->border_w);
    XFlush(w->wb->dpy);

    free(w);
}

static int runes_window_event_cb(void *t)
{
    RunesWindow *w = ((RunesTerm *)t)->w;
    XEvent e;
    int should_close = 0;

    while (XCheckIfEvent(w->wb->dpy, &e, runes_window_wants_event, t)) {
        should_close = runes_window_handle_event(((RunesTerm *)t), &e);
        if (should_close) {
            break;
        }
    }

    if (should_close) {
        runes_pty_request_close(t);
        runes_term_refcnt_dec(t);
    }

    return !should_close;
}

static int runes_window_handle_event(RunesTerm *t, XEvent *e)
{
    RunesWindow *w = ((RunesTerm *)t)->w;
    int should_close = 0;

    if (!XFilterEvent(e, None)) {
        switch (e->type) {
        case KeyPress:
        case KeyRelease:
            runes_window_handle_key_event(t, &e->xkey);
            break;
        case ButtonPress:
        case ButtonRelease:
            runes_window_handle_button_event(t, &e->xbutton);
            break;
        case MotionNotify:
            runes_window_handle_motion_event(t, &e->xmotion);
            break;
        case Expose:
            runes_window_handle_expose_event(t, &e->xexpose);
            break;
        case ConfigureNotify:
            runes_window_handle_configure_event(t, &e->xconfigure);
            break;
        case FocusIn:
        case FocusOut:
            runes_window_handle_focus_event(t, &e->xfocus);
            break;
        case SelectionNotify:
            runes_window_handle_selection_notify_event(
                t, &e->xselection);
            break;
        case SelectionRequest:
            runes_window_handle_selection_request_event(
                t, &e->xselectionrequest);
            break;
        case SelectionClear:
            runes_window_handle_selection_clear_event(
                t, &e->xselectionclear);
            break;
        case ClientMessage: {
            Atom a = e->xclient.data.l[0];
            if (a == w->wb->atoms[RUNES_ATOM_WM_DELETE_WINDOW]) {
                should_close = 1;
            }
            else if (a == w->wb->atoms[RUNES_ATOM_NET_WM_PING]) {
                e->xclient.window = DefaultRootWindow(w->wb->dpy);
                XSendEvent(
                    w->wb->dpy, e->xclient.window, False,
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

    return should_close;
}

static Bool runes_window_wants_event(Display *dpy, XEvent *event, XPointer arg)
{
    RunesWindow *w = ((RunesTerm *)arg)->w;
    Window event_window = ((XAnyEvent*)event)->window;

    UNUSED(dpy);

    return event_window == w->w || event_window == w->border_w;
}

static void runes_window_resize_window(
    RunesTerm *t, int width, int height)
{
    RunesWindow *w = t->w;

    /* XXX no idea why shrinking the window dimensions to 0 makes xlib think
     * that the dimension is 65535 */
    if (width < 1 || width >= 65535) {
        width = 1;
    }
    if (height < 1 || height >= 65535) {
        height = 1;
    }

    if (width != t->display->xpixel || height != t->display->ypixel) {
        int dwidth = width - 4, dheight = height - 4;

        XResizeWindow(w->wb->dpy, w->w, dwidth, dheight);
        cairo_xlib_surface_set_size(
            cairo_get_target(w->backend_cr), dwidth, dheight);
        runes_term_set_window_size(t, dwidth, dheight);
        runes_window_clear_selection(t);
    }
}

static void runes_window_write_to_pty(
    RunesTerm *t, char *buf, size_t len)
{
    runes_pty_write(t, buf, len);
    if (t->display->row_visible_offset != 0) {
        t->display->row_visible_offset = 0;
        t->display->dirty = 1;
        runes_window_flush(t);
    }
}

static int runes_window_check_recent(RunesTerm *t)
{
    RunesWindow *w = t->w;
    struct timespec now;
    int rate = t->config->redraw_rate;

    if (rate == 0) {
        return 0;
    }

    if (w->delaying) {
        return 1;
    }

    clock_gettime(CLOCK_REALTIME, &now);
    while (rate >= 1000) {
        now.tv_sec -= 1;
        rate -= 1000;
    }
    now.tv_nsec -= rate * 1000000;
    while (now.tv_nsec < 0) {
        now.tv_sec -= 1;
        now.tv_nsec += 1000000000;
    }
    if (now.tv_sec < w->last_redraw.tv_sec || (now.tv_sec == w->last_redraw.tv_sec && now.tv_nsec < w->last_redraw.tv_nsec)) {
        runes_term_refcnt_inc(t);
        runes_loop_timer_set(
            t->loop, t->config->redraw_rate, t, runes_window_delay_cb);
        w->delaying = 1;
        return 1;
    }
    else {
        return 0;
    }

}

static void runes_window_delay_cb(void *t)
{
    RunesWindow *w = ((RunesTerm *)t)->w;

    w->delaying = 0;
    runes_window_flush(t);
    runes_term_refcnt_dec((RunesTerm*)t);
}

static void runes_window_visible_scroll(RunesTerm *t, int count)
{
    int min = 0, max = t->scr->grid->row_count - t->scr->grid->max.row;
    int old_offset = t->display->row_visible_offset;

    t->display->row_visible_offset += count;
    if (t->display->row_visible_offset < min) {
        t->display->row_visible_offset = min;
    }
    if (t->display->row_visible_offset > max) {
        t->display->row_visible_offset = max;
    }

    if (t->display->row_visible_offset == old_offset) {
        return;
    }

    t->display->dirty = 1;
    runes_window_flush(t);
}

static void runes_window_visual_bell(RunesTerm *t)
{
    RunesWindow *w = t->w;

    if (t->config->bell_is_urgent) {
        runes_window_set_urgent(t);
    }

    if (!w->visual_bell_is_ringing) {
        w->visual_bell_is_ringing = 1;
        cairo_set_source(w->backend_cr, t->config->fgdefault);
        cairo_paint(w->backend_cr);
        cairo_surface_flush(cairo_get_target(w->backend_cr));
        XFlush(w->wb->dpy);

        runes_term_refcnt_inc(t);
        runes_loop_timer_set(t->loop, 20, t, runes_window_reset_visual_bell);
    }
}

static void runes_window_reset_visual_bell(void *t)
{
    RunesWindow *w = ((RunesTerm *)t)->w;

    w->visual_bell_is_ringing = 0;
    cairo_set_source(w->backend_cr, ((RunesTerm *)t)->config->bgdefault);
    cairo_paint(w->backend_cr);
    runes_window_flush(t);
    runes_term_refcnt_dec(t);
}

static void runes_window_audible_bell(RunesTerm *t)
{
    RunesWindow *w = t->w;

    if (t->config->audible_bell) {
        if (t->config->bell_is_urgent) {
            runes_window_set_urgent(t);
        }
        XBell(w->wb->dpy, 0);
    }
    else {
        runes_window_visual_bell(t);
    }
}

static void runes_window_set_urgent(RunesTerm *t)
{
    RunesWindow *w = t->w;
    XWMHints *hints;

    hints = XGetWMHints(w->wb->dpy, w->border_w);
    hints->flags |= XUrgencyHint;
    XSetWMHints(w->wb->dpy, w->border_w, hints);
    XFree(hints);
}

static void runes_window_clear_urgent(RunesTerm *t)
{
    RunesWindow *w = t->w;
    XWMHints *hints;

    hints = XGetWMHints(w->wb->dpy, t->w->border_w);
    hints->flags &= ~XUrgencyHint;
    XSetWMHints(w->wb->dpy, t->w->border_w, hints);
    XFree(hints);
}

static void runes_window_paste(RunesTerm *t, Time time)
{
    RunesWindow *w = t->w;

    XConvertSelection(
        w->wb->dpy, XA_PRIMARY, w->wb->atoms[RUNES_ATOM_UTF8_STRING],
        w->wb->atoms[RUNES_ATOM_RUNES_SELECTION], w->w, time);
}

static void runes_window_start_selection(
    RunesTerm *t, int xpixel, int ypixel)
{
    struct vt100_loc loc;

    loc = runes_window_get_mouse_position(t, xpixel, ypixel);
    runes_window_start_selection_loc(t, &loc);
}

static void runes_window_start_selection_loc(
    RunesTerm *t, struct vt100_loc *start)
{
    runes_display_set_selection(t, start, start);
    runes_window_flush(t);
}

static void runes_window_update_selection(
    RunesTerm *t, int xpixel, int ypixel, Time time)
{
    struct vt100_loc loc;

    loc = runes_window_get_mouse_position(t, xpixel, ypixel);
    runes_window_update_selection_loc(t, &loc, time);
}

static void runes_window_update_selection_loc(
    RunesTerm *t, struct vt100_loc *end, Time time)
{
    if (!t->display->has_selection) {
        return;
    }

    if (t->display->selection_end.row != end->row || t->display->selection_end.col != end->col) {
        runes_window_acquire_selection(t, time);
        runes_display_set_selection(t, &t->display->selection_start, end);
        runes_window_flush(t);
    }
}

static void runes_window_acquire_selection(RunesTerm *t, Time time)
{
    RunesWindow *w = t->w;
    Window old_owner;
    int got_selection;

    if (w->owns_selection) {
        return;
    }

    old_owner = XGetSelectionOwner(w->wb->dpy, XA_PRIMARY);
    XSetSelectionOwner(w->wb->dpy, XA_PRIMARY, w->w, time);
    got_selection = (XGetSelectionOwner(w->wb->dpy, XA_PRIMARY) == w->w);

    if (got_selection) {
        w->owns_selection = 1;
        if (old_owner != w->w) {
            XEvent e;

            e.xselectionclear.type = SelectionClear;
            e.xselectionclear.window = old_owner;
            e.xselectionclear.selection = XA_PRIMARY;

            XSendEvent(w->wb->dpy, old_owner, False, NoEventMask, &e);
        }
    }
}

static void runes_window_clear_selection(RunesTerm *t)
{
    RunesWindow *w = t->w;

    XSetSelectionOwner(w->wb->dpy, XA_PRIMARY, None, CurrentTime);
    t->display->has_selection = 0;
    w->owns_selection = 0;
}

static void runes_window_handle_key_event(RunesTerm *t, XKeyEvent *e)
{
    RunesWindow *w = t->w;
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
    case XLookupBoth:
    case XLookupKeySym:
        if (!runes_window_handle_builtin_keypress(t, sym, e)) {
            struct function_key *key;

            key = runes_window_find_key_sequence(t, sym);
            if (key->sym != XK_VoidSymbol) {
                runes_window_write_to_pty(t, key->str, key->len);
                break;
            }
        }
        if (s == XLookupKeySym) {
            break;
        }
    case XLookupChars:
        if (e->state & Mod1Mask) {
            runes_window_write_to_pty(t, "\033", 1);
        }
        runes_window_write_to_pty(t, buf, chars);
        break;
    default:
        break;
    }
    free(buf);
}

static void runes_window_handle_button_event(RunesTerm *t, XButtonEvent *e)
{
    if (e->state & ShiftMask) {
        if (runes_window_handle_builtin_button_press(t, e)) {
            return;
        }
    }

    if (t->scr->mouse_reporting_press_release) {
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

        loc = runes_window_get_mouse_position(t, e->x, e->y);
        sprintf(
            response, "\033[M%c%c%c",
            ' ' + (status), ' ' + loc.col + 1, ' ' + loc.row + 1);
        runes_window_write_to_pty(t, response, 6);
    }
    else if (t->scr->mouse_reporting_press && e->type == ButtonPress) {
        char response[7];
        struct vt100_loc loc;

        loc = runes_window_get_mouse_position(t, e->x, e->y);
        sprintf(
            response, "\033[M%c%c%c",
            ' ' + (e->button - 1), ' ' + loc.col + 1, ' ' + loc.row + 1);
        runes_window_write_to_pty(t, response, 6);
    }
    else {
        runes_window_handle_builtin_button_press(t, e);
    }
}

static void runes_window_handle_motion_event(RunesTerm *t, XMotionEvent *e)
{
    if (!(e->state & Button1Mask)) {
        return;
    }

    runes_window_update_selection(t, e->x, e->y, e->time);
}

static void runes_window_handle_expose_event(RunesTerm *t, XExposeEvent *e)
{
    UNUSED(e);

    runes_window_flush(t);
}

static void runes_window_handle_configure_event(
    RunesTerm *t, XConfigureEvent *e)
{
    RunesWindow *w = t->w;

    if (e->window != w->border_w) {
        return;
    }

    while (XCheckTypedWindowEvent(
        w->wb->dpy, w->border_w, ConfigureNotify, (XEvent *)e));
    runes_window_resize_window(t, e->width, e->height);
}

static void runes_window_handle_focus_event(RunesTerm *t, XFocusChangeEvent *e)
{
    /* we don't care about focus events that are only sent because the pointer
     * is in the window, if focus is changing between two other unrelated
     * windows */
    if (e->detail == NotifyPointer) {
        return;
    }

    if (e->type == FocusIn && !t->display->unfocused) {
        return;
    }
    if (e->type == FocusOut && t->display->unfocused) {
        return;
    }

    runes_window_clear_urgent(t);
    if (e->type == FocusIn) {
        t->display->unfocused = 0;
    }
    else {
        t->display->unfocused = 1;
    }
    runes_window_flush(t);
}

static void runes_window_handle_selection_notify_event(
    RunesTerm *t, XSelectionEvent *e)
{
    RunesWindow *w = t->w;

    if (e->property == None) {
        if (e->target == w->wb->atoms[RUNES_ATOM_UTF8_STRING]) {
            XConvertSelection(
                w->wb->dpy, XA_PRIMARY, XA_STRING,
                w->wb->atoms[RUNES_ATOM_RUNES_SELECTION], w->w, e->time);
        }
    }
    else {
        Atom type;
        int format;
        unsigned long nitems, left;
        unsigned char *buf;

        XGetWindowProperty(
            w->wb->dpy, e->requestor, e->property, 0, 0x1fffffff, 0,
            AnyPropertyType, &type, &format, &nitems, &left, &buf);
        if (t->scr->bracketed_paste) {
            runes_window_write_to_pty(t, "\033[200~", 6);
        }
        runes_window_write_to_pty(t, (char *)buf, nitems);
        if (t->scr->bracketed_paste) {
            runes_window_write_to_pty(t, "\033[201~", 6);
        }
        XFree(buf);
    }
}

static void runes_window_handle_selection_clear_event(
    RunesTerm *t, XSelectionClearEvent *e)
{
    RunesWindow *w = t->w;
    UNUSED(e);

    t->display->has_selection = 0;
    w->owns_selection = 0;
    t->display->dirty = 1;
    runes_window_flush(t);
}

static void runes_window_handle_selection_request_event(
    RunesTerm *t, XSelectionRequestEvent *e)
{
    RunesWindow *w = t->w;
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

    if (e->target == w->wb->atoms[RUNES_ATOM_TARGETS]) {
        Atom targets[2] = { XA_STRING, w->wb->atoms[RUNES_ATOM_UTF8_STRING] };

        XChangeProperty(
            w->wb->dpy, e->requestor, e->property,
            XA_ATOM, 32, PropModeReplace,
            (unsigned char *)&targets, 2);
    }
    else if (e->target == XA_STRING
             || e->target == w->wb->atoms[RUNES_ATOM_UTF8_STRING]) {
        if (t->display->selection_contents) {
            XChangeProperty(
                w->wb->dpy, e->requestor, e->property,
                e->target, 8, PropModeReplace,
                (unsigned char *)t->display->selection_contents,
                t->display->selection_len);
        }
    }
    else {
        selection.property = None;
    }

    XSendEvent(
        w->wb->dpy, e->requestor, False, NoEventMask, (XEvent *)&selection);
}

static int runes_window_handle_builtin_keypress(
    RunesTerm *t, KeySym sym, XKeyEvent *e)
{
    switch (sym) {
    case XK_Insert:
        if (e->state & ShiftMask) {
            runes_window_paste(t, e->time);
            return 1;
        }
        break;
    case XK_Page_Up:
        if (e->state & ShiftMask) {
            runes_window_visible_scroll(
                t, t->scr->grid->max.row - 1);
            return 1;
        }
        break;
    case XK_Page_Down:
        if (e->state & ShiftMask) {
            runes_window_visible_scroll(
                t, -(t->scr->grid->max.row - 1));
            return 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int runes_window_handle_builtin_button_press(
    RunesTerm *t, XButtonEvent *e)
{
    switch (e->type) {
    case ButtonPress:
        switch (e->button) {
        case Button1:
            runes_window_start_selection(t, e->x, e->y);
            return 1;
            break;
        case Button2:
            runes_window_paste(t, e->time);
            return 1;
            break;
        case Button3:
            runes_window_update_selection(t, e->x, e->y, e->time);
            return 1;
            break;
        case Button4:
            runes_window_visible_scroll(t, t->config->scroll_lines);
            return 1;
            break;
        case Button5:
            runes_window_visible_scroll(t, -t->config->scroll_lines);
            return 1;
            break;
        default:
            break;
        }
        break;
    case ButtonRelease:
        runes_window_handle_multi_click(t, e);
        break;
    default:
        break;
    }

    return 0;
}

static void runes_window_handle_multi_click(RunesTerm *t, XButtonEvent *e)
{
    RunesWindow *w = t->w;

    if (e->button != Button1) {
        return;
    }

    if (w->multi_click_timer_event) {
        runes_loop_timer_clear(t->loop, w->multi_click_timer_event);
        runes_term_refcnt_dec(t);
    }

    runes_term_refcnt_inc(t);
    w->multi_click_timer_event = runes_loop_timer_set(
        t->loop, t->config->double_click_rate, t,
        runes_window_multi_click_cb);

    w->multi_clicks++;
    if (w->multi_clicks > 1) {
        struct vt100_loc loc;

        loc = runes_window_get_mouse_position(t, e->x, e->y);
        if (w->multi_clicks % 2) {
            loc.col = 0;
            runes_window_start_selection_loc(t, &loc);
            loc.col = t->scr->grid->max.col;
            runes_window_update_selection_loc(t, &loc, e->time);
        }
        else {
            struct vt100_loc orig_loc = loc;

            runes_window_beginning_of_word(t, &loc);
            runes_window_start_selection_loc(t, &loc);
            loc = orig_loc;
            runes_window_end_of_word(t, &loc);
            runes_window_update_selection_loc(t, &loc, e->time);
        }
    }
}

static void runes_window_beginning_of_word(RunesTerm *t, struct vt100_loc *loc)
{
    struct vt100_cell *c;
    struct vt100_row *r;
    int row = loc->row - t->scr->grid->row_top, col = loc->col;
    int prev_row = row, prev_col = col;

    /* XXX vt100 should expose a better way to get at row data */
    c = vt100_screen_cell_at(t->scr, row, col);
    r = &t->scr->grid->rows[row + t->scr->grid->row_top];
    while (runes_window_is_word_char(c->contents, c->len)) {
        prev_col = col;
        prev_row = row;
        col--;
        if (col < 0) {
            row--;
            if (row + t->scr->grid->row_top < 0) {
                break;
            }
            r = &t->scr->grid->rows[row + t->scr->grid->row_top];
            if (!r->wrapped) {
                break;
            }
            col = t->scr->grid->max.col - 1;
        }
        c = vt100_screen_cell_at(t->scr, row, col);
    }

    loc->row = prev_row + t->scr->grid->row_top;
    loc->col = prev_col;
}

static void runes_window_end_of_word(RunesTerm *t, struct vt100_loc *loc)
{
    struct vt100_cell *c;
    struct vt100_row *r;
    int row = loc->row - t->scr->grid->row_top, col = loc->col;

    /* XXX vt100 should expose a better way to get at row data */
    c = vt100_screen_cell_at(t->scr, row, col);
    r = &t->scr->grid->rows[row + t->scr->grid->row_top];
    while (runes_window_is_word_char(c->contents, c->len)) {
        col++;
        if (col >= t->scr->grid->max.col) {
            if (!r->wrapped) {
                break;
            }
            row++;
            if (row >= t->scr->grid->max.row) {
                break;
            }
            r = &t->scr->grid->rows[row + t->scr->grid->row_top];
            col = 0;
        }
        c = vt100_screen_cell_at(t->scr, row, col);
    }

    loc->row = row + t->scr->grid->row_top;
    loc->col = col;
}

static int runes_window_is_word_char(char *buf, size_t len)
{
    gunichar uc;

    if (len == 0) {
        return 0;
    }

    if (g_utf8_next_char(buf) < buf + len) {
        return 1;
    }

    uc = g_utf8_get_char(buf);
    if (vt100_char_width(uc) == 0) {
        return 0;
    }
    if (g_unichar_isspace(uc)) {
        return 0;
    }

    return 1;
}

static void runes_window_multi_click_cb(void *t)
{
    RunesWindow *w = ((RunesTerm *)t)->w;

    w->multi_clicks = 0;
    w->multi_click_timer_event = NULL;
    runes_term_refcnt_dec(t);
}

static struct function_key *runes_window_find_key_sequence(
    RunesTerm *t, KeySym sym)
{
    struct function_key *key;

    if (t->scr->application_keypad) {
        if (t->scr->application_cursor) {
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

static struct vt100_loc runes_window_get_mouse_position(
    RunesTerm *t, int xpixel, int ypixel)
{
    struct vt100_loc ret;

    ret.row = ypixel / t->display->fonty;
    ret.col = xpixel / t->display->fontx;

    ret.row = ret.row < 0                         ? 0
            : ret.row > t->scr->grid->max.row - 1 ? t->scr->grid->max.row - 1
            :                                       ret.row;
    ret.col = ret.col < 0                     ? 0
            : ret.col > t->scr->grid->max.col ? t->scr->grid->max.col
            :                                   ret.col;

    ret.row = ret.row - t->display->row_visible_offset + t->scr->grid->row_count - t->scr->grid->max.row;

    return ret;
}
