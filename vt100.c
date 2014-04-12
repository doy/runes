#include <stdio.h>
#include <string.h>

#include "runes.h"

static const char *ctrl_chars =
    /* XXX working with nul terminated strings for the moment... */
    /* "\000\001\002\003\004\005\006\007" */
        "\001\002\003\004\005\006\007"
    "\010\011\012\013\014\015\016\017"
    "\020\021\022\023\024\025\026\027"
    "\030\031\032\033\034\035\036\037"
    "\177";

static char *runes_vt100_handle_ctrl_char(RunesTerm *t, char *buf, size_t len);
static char *runes_vt100_handle_escape_sequence(
    RunesTerm *t, char *buf, size_t len);
static char *runes_vt100_handle_csi(RunesTerm *t, char *buf, size_t len);
static char *runes_vt100_handle_osc(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_unhandled_escape_sequence(
    RunesTerm *t, char type);
static void runes_vt100_unhandled_csi(
    RunesTerm *t, char dec, int p[3], char type);
static void runes_vt100_unhandled_osc(
    RunesTerm *t, int type, char *arg, char terminator);

void runes_vt100_process_string(RunesTerm *t, char *buf, size_t len)
{
    int found;
    size_t prefix;

    buf[len] = '\0';
    do {
        found = 0;

        prefix = strcspn(buf, ctrl_chars);
        if (prefix) {
            char tmp = buf[prefix];

            buf[prefix] = '\0';
            runes_display_show_string(t, buf, strlen(buf));
            buf[prefix] = tmp;
            buf += prefix;
            found = 1;
        }

        prefix = strspn(buf, ctrl_chars);
        if (prefix) {
            char *end = buf + prefix;

            while (buf < end) {
                buf = runes_vt100_handle_ctrl_char(t, buf, strlen(buf));
            }
            found = 1;
        }
    } while (found);
}

static char *runes_vt100_handle_ctrl_char(RunesTerm *t, char *buf, size_t len)
{
    switch (buf[0]) {
    case '\010':   /* BS */
        runes_display_backspace(t);
        buf++;
        break;
    case '\011':   /* TAB */
        runes_display_move_to(t, t->row, t->col - (t->col % 8) + 8);
        buf++;
        break;
    case '\012':   /* LF */
    case '\013':   /* VT */
    case '\014':   /* FF */
        runes_display_move_to(t, t->row + 1, t->col);
        buf++;
        break;
    case '\015':   /* CR */
        runes_display_move_to(t, t->row, 0);
        buf++;
        break;
    case '\033':
        buf++;
        buf = runes_vt100_handle_escape_sequence(t, buf, len);
        break;
    default:
        buf++;
        break;
    }

    return buf;
}

static char *runes_vt100_handle_escape_sequence(
    RunesTerm *t, char *buf, size_t len)
{
    switch (buf[0]) {
    case '[': /* CSI */
        buf++;
        buf = runes_vt100_handle_csi(t, buf, len);
        break;
    case ']': /* OSC */
        buf++;
        buf = runes_vt100_handle_osc(t, buf, len);
        break;
    default:
        runes_vt100_unhandled_escape_sequence(t, buf[0]);
        buf++;
        break;
    }

    return buf;
}

static char *runes_vt100_handle_csi(RunesTerm *t, char *buf, size_t len)
{
    int p[3] = { -1, -1, -1 };
    int paramlen;
    char type, dec = 0;

    UNUSED(len);

    if (buf[0] == '?') {
        dec = 1;
        buf++;
    }

    /* XXX stop hardcoding the max number of parameters */
    if (sscanf(buf, "%d;%d;%d%c%n", &p[0], &p[1], &p[2], &type, &paramlen) == 4) {
        /* nothing */
    }
    else if (sscanf(buf, "%d;%d%c%n", &p[0], &p[1], &type, &paramlen) == 3) {
        /* nothing */
    }
    else if (sscanf(buf, "%d%c%n", &p[0], &type, &paramlen) == 2) {
        /* nothing */
    }
    else if (sscanf(buf, "%c%n", &type, &paramlen) == 1) {
        /* nothing */
    }

    switch (type) {
    case 'D':   /* CUB */
        runes_display_move_to(t, t->row, t->col - 1);
        break;
    case 'B':   /* CUD */
        runes_display_move_to(t, t->row + 1, t->col);
        break;
    case 'C':   /* CUF */
        runes_display_move_to(t, t->row, t->col + 1);
        break;
    case 'A':   /* CUU */
        runes_display_move_to(t, t->row - 1, t->col);
        break;
    case 'H':   /* CUP */
        if (p[0] == -1) {
            p[0] = 0;
        }
        if (p[1] == -1) {
            p[1] = 0;
        }
        runes_display_move_to(t, p[0], p[1]);
        break;
    case 'J':   /* ED */
        /* XXX need to do something special when dec is set */
        switch (p[0]) {
        case -1:
        case 0:
            runes_display_clear_screen_forward(t);
            break;
        case 1:
            /* XXX */
            runes_vt100_unhandled_csi(t, dec, p, type);
            break;
        case 2:
            runes_display_clear_screen(t);
            break;
        default:
            runes_vt100_unhandled_csi(t, dec, p, type);
            break;
        }
        break;
    case 'K':   /* EL */
        /* XXX need to do something special when dec is set */
        switch (p[0]) {
        case -1:
        case 0:
            runes_display_kill_line_forward(t);
            break;
        case 1:
            /* XXX */
            runes_vt100_unhandled_csi(t, dec, p, type);
            break;
        case 2:
            /* XXX */
            runes_vt100_unhandled_csi(t, dec, p, type);
            break;
        default:
            runes_vt100_unhandled_csi(t, dec, p, type);
            break;
        }
        break;
    case 'h':
        if (dec) {
            switch (p[0]) {
            case 25:
                runes_display_show_cursor(t);
                break;
            default:
                runes_vt100_unhandled_csi(t, dec, p, type);
                break;
            }
        }
        else {
            runes_vt100_unhandled_csi(t, dec, p, type);
        }
        break;
    case 'l':
        if (dec) {
            switch (p[0]) {
            case 25:
                runes_display_hide_cursor(t);
                break;
            default:
                runes_vt100_unhandled_csi(t, dec, p, type);
                break;
            }
        }
        else {
            runes_vt100_unhandled_csi(t, dec, p, type);
        }
        break;
    case 'm': { /* SGR */
        int i;

        if (p[0] == -1) {
            p[0] = 0;
        }

        for (i = 0; i < 3; ++i) {
            switch (p[i]) {
            case -1:
                break;
            case 0:
                runes_display_reset_text_attributes(t);
                break;
            case 1:
                runes_display_set_bold(t);
                break;
            case 3:
                runes_display_set_italic(t);
                break;
            case 4:
                runes_display_set_underline(t);
                break;
            case 22:
                runes_display_reset_bold(t);
                break;
            case 23:
                runes_display_reset_italic(t);
                break;
            case 24:
                runes_display_reset_underline(t);
                break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                runes_display_set_fg_color(t, t->colors[p[i] - 30]);
                break;
            case 39:
                runes_display_reset_fg_color(t);
                break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                runes_display_set_bg_color(t, t->colors[p[i] - 40]);
                break;
            case 49:
                runes_display_reset_bg_color(t);
                break;
            /* XXX ... */
            default:
                runes_vt100_unhandled_csi(t, dec, p, type);
                break;
            }
        }
        break;
    }
    default:
        runes_vt100_unhandled_csi(t, dec, p, type);
        break;
    }

    return buf + paramlen;
}

static char *runes_vt100_handle_osc(RunesTerm *t, char *buf, size_t len)
{
    int type, prefix;

    UNUSED(len);

    if (sscanf(buf, "%d%n", &type, &prefix) != 1) {
        runes_vt100_unhandled_osc(t, -1, "", buf[0]);
        return buf + 1;
    }

    switch (type) {
    case 0:
        if (buf[prefix] == ';') {
            size_t namelen;

            namelen = strcspn(&buf[prefix + 1], "\007");
            runes_window_backend_set_icon_name(t, &buf[prefix + 1], namelen);
            runes_window_backend_set_window_title(
                t, &buf[prefix + 1], namelen);
            prefix += namelen + 2;
        }
        else {
            runes_vt100_unhandled_osc(t, type, "", -1);
            prefix++;
        }
        break;
    case 1:
        if (buf[prefix] == ';') {
            size_t namelen;

            namelen = strcspn(&buf[prefix + 1], "\007");
            runes_window_backend_set_icon_name(t, &buf[prefix + 1], namelen);
            prefix += namelen + 2;
        }
        else {
            runes_vt100_unhandled_osc(t, type, "", -1);
            prefix++;
        }
        break;
    case 2:
        if (buf[prefix] == ';') {
            size_t namelen;

            namelen = strcspn(&buf[prefix + 1], "\007");
            runes_window_backend_set_window_title(
                t, &buf[prefix + 1], namelen);
            prefix += namelen + 2;
        }
        else {
            runes_vt100_unhandled_osc(t, type, "", -1);
            prefix++;
        }
        break;
    default:
        runes_vt100_unhandled_osc(t, type, "", -1);
        prefix++;
        break;
    }

    return buf + prefix;
}

static void runes_vt100_unhandled_escape_sequence(
    RunesTerm *t, char type)
{
    UNUSED(t);

    fprintf(stderr, "unhandled escape sequence: \\033%c\n", type);
}

static void runes_vt100_unhandled_csi(
    RunesTerm *t, char dec, int p[3], char type)
{
    UNUSED(t);

    fprintf(stderr, "unhandled escape sequence: \\033[");
    if (dec) {
        fprintf(stderr, "?");
    }
    if (p[0] != -1) {
        fprintf(stderr, "%d", p[0]);
    }
    if (p[1] != -1) {
        fprintf(stderr, ";%d", p[1]);
    }
    if (p[2] != -1) {
        fprintf(stderr, ";%d", p[2]);
    }
    fprintf(stderr, "%c\n", type);
}

static void runes_vt100_unhandled_osc(
    RunesTerm *t, int type, char *arg, char terminator)
{
    UNUSED(t);

    fprintf(stderr, "unhandled escape sequence: \\033]");
    if (type == -1) {
        fprintf(stderr, "\\%hho\n", terminator);
    }
    else if (terminator == -1) {
        fprintf(stderr, "%d;<unknown>\n", type);
    }
    else {
        fprintf(stderr, "%d;%s\\%hho\n", type, arg, terminator);
    }
}
