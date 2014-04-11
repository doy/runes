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

static void runes_vt100_unhandled_escape_sequence(RunesTerm *t, int *p, char type)
{
    UNUSED(t);

    fprintf(stderr, "unhandled escape sequence: \\033");
    if (p) {
        fprintf(stderr, "[");
        if (p[0] != -1) {
            fprintf(stderr, "%d", p[0]);
        }
        if (p[1] != -1) {
            fprintf(stderr, ";%d", p[1]);
        }
        if (p[2] != -1) {
            fprintf(stderr, ";%d", p[2]);
        }
    }
    fprintf(stderr, "%c\n", type);
}

static char *runes_vt100_handle_escape_sequence(RunesTerm *t, char *buf, size_t len)
{
    UNUSED(len);

    if (buf[1] == '[') { /* CSI */
        int p[3] = { -1, -1, -1 };
        int paramlen;
        char type;

        buf += 2;

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
        case 'D': { /* CUB */
            int row, col;

            runes_display_get_position(t, &row, &col);
            runes_display_move_to(t, row, col - 1);
            break;
        }
        case 'B': { /* CUD */
            int row, col;

            runes_display_get_position(t, &row, &col);
            runes_display_move_to(t, row + 1, col);
            break;
        }
        case 'C': { /* CUF */
            int row, col;

            runes_display_get_position(t, &row, &col);
            runes_display_move_to(t, row, col + 1);
            break;
        }
        case 'A': { /* CUU */
            int row, col;

            runes_display_get_position(t, &row, &col);
            runes_display_move_to(t, row - 1, col);
            break;
        }
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
            switch (p[0]) {
            case -1:
            case 0:
                runes_display_clear_screen_forward(t);
                break;
            case 1:
                /* XXX */
                runes_vt100_unhandled_escape_sequence(t, p, type);
                break;
            case 2:
                runes_display_clear_screen(t);
                break;
            default:
                runes_vt100_unhandled_escape_sequence(t, p, type);
                break;
            }
            break;
        case 'K':   /* EL */
            switch (p[0]) {
            case -1:
            case 0:
                runes_display_kill_line_forward(t);
                break;
            case 1:
                /* XXX */
                runes_vt100_unhandled_escape_sequence(t, p, type);
                break;
            case 2:
                /* XXX */
                runes_vt100_unhandled_escape_sequence(t, p, type);
                break;
            default:
                runes_vt100_unhandled_escape_sequence(t, p, type);
                break;
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
                    runes_vt100_unhandled_escape_sequence(t, p, type);
                    break;
                }
            }
            break;
        }
        default:
            runes_vt100_unhandled_escape_sequence(t, p, type);
            break;
        }

        buf += paramlen;
    }
    else {
        runes_vt100_unhandled_escape_sequence(t, NULL, buf[1]);
        /* do nothing, for now */
        buf++;
    }

    return buf;
}

static char *runes_vt100_handle_ctrl_char(RunesTerm *t, char *buf, size_t len)
{
    switch (buf[0]) {
    case '\010':   /* BS */
        runes_display_backspace(t);
        buf++;
        break;
    case '\011': { /* TAB */
        int row, col;

        runes_display_get_position(t, &row, &col);
        runes_display_move_to(t, row, col - (col % 8) + 8);
        buf++;
        break;
    }
    case '\012':   /* LF */
    case '\013':   /* VT */
    case '\014': { /* FF */
        int row, col;

        runes_display_get_position(t, &row, &col);
        runes_display_move_to(t, row + 1, col);
        buf++;
        break;
    }
    case '\015': { /* CR */
        int row, col;

        runes_display_get_position(t, &row, &col);
        runes_display_move_to(t, row, 0);
        buf++;
        break;
    }
    case '\033':
        buf = runes_vt100_handle_escape_sequence(t, buf, len);
        break;
    default: {
        buf++;
        break;
    }
    }

    return buf;
}

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
