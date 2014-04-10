#include "runes.h"

void runes_handle_keyboard_event(RunesTerm *t, char *buf, size_t len)
{
    runes_pty_backend_write(t, buf, len);
}

void runes_handle_pty_read(RunesTerm *t, char *buf, ssize_t len)
{
    runes_vt100_process_string(t, buf, len);
}

void runes_handle_pty_close(RunesTerm *t)
{
    runes_window_backend_request_close(t);
}

void runes_handle_close_window(RunesTerm *t)
{
    runes_pty_backend_request_close(t);
}
