#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

void runes_display_init(RunesTerm *t);
void runes_display_get_term_size(RunesTerm *t, int *row, int *col, int *xpixel, int *ypixel);
void runes_display_get_position(RunesTerm *t, int *row, int *col);
void runes_display_move_to(RunesTerm *t, int row, int col);
void runes_display_show_string(RunesTerm *t, char *buf, size_t len);
void runes_display_backspace(RunesTerm *t);
void runes_display_kill_line_forward(RunesTerm *t);

#endif
