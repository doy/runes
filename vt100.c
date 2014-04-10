#include <string.h>

#include "runes.h"

void runes_vt100_process_string(RunesTerm *t, char *buf, size_t len)
{
    int row, col;
    char *nl;

    runes_display_get_position(t, &row, &col);

    buf[len] = '\0';
    while ((nl = strchr(buf, '\n'))) {
        *nl = '\0';
        runes_display_show_string(t, buf, strlen(buf));
        runes_display_move_to(t, ++row, 0);
        buf = nl + 1;
    }
    runes_display_show_string(t, buf, strlen(buf));
}
