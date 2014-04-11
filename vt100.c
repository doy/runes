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

static char *runes_vt100_handle_escape_sequence(RunesTerm *t, char *buf, size_t len)
{
    UNUSED(len);

    if (buf[1] == '[') { /* CSI */
        /* ignoring parameters, for now */
        switch (buf[2]) {
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
        }
        case 'C': { /* CUF */
            int row, col;

            runes_display_get_position(t, &row, &col);
            runes_display_move_to(t, row, col + 1);
        }
        case 'A': { /* CUU */
            int row, col;

            runes_display_get_position(t, &row, &col);
            runes_display_move_to(t, row - 1, col);
        }
        case 'K':   /* EL */
            runes_display_kill_line_forward(t);
        default:
            break;
        }

        buf += 3;
    }
    else {
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
