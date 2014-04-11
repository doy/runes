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

static char *runes_vt100_handle_ctrl_char(RunesTerm *t, char *buf, size_t len)
{
    UNUSED(len);

    switch (buf[0]) {
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
            char tmp = *end;

            *end = '\0';
            while (buf < end) {
                buf = runes_vt100_handle_ctrl_char(t, buf, strlen(buf));
            }
            *end = tmp;
            found = 1;
        }
    } while (found);
}
