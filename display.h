#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

void runes_display_init(RunesTerm *t);
void runes_display_set_window_size(RunesTerm *t);
void runes_display_draw_cursor(RunesTerm *t);
void runes_display_focus_in(RunesTerm *t);
void runes_display_focus_out(RunesTerm *t);
void runes_display_move_to(RunesTerm *t, int row, int col);
void runes_display_show_string(RunesTerm *t, char *buf, size_t len);
void runes_display_clear_screen(RunesTerm *t);
void runes_display_clear_screen_forward(RunesTerm *t);
void runes_display_kill_line_forward(RunesTerm *t);
void runes_display_delete_characters(RunesTerm *t, int count);
void runes_display_reset_text_attributes(RunesTerm *t);
void runes_display_set_bold(RunesTerm *t);
void runes_display_reset_bold(RunesTerm *t);
void runes_display_set_italic(RunesTerm *t);
void runes_display_reset_italic(RunesTerm *t);
void runes_display_set_underline(RunesTerm *t);
void runes_display_reset_underline(RunesTerm *t);
void runes_display_set_fg_color(RunesTerm *t, cairo_pattern_t *color);
void runes_display_reset_fg_color(RunesTerm *t);
void runes_display_set_bg_color(RunesTerm *t, cairo_pattern_t *color);
void runes_display_reset_bg_color(RunesTerm *t);
void runes_display_show_cursor(RunesTerm *t);
void runes_display_hide_cursor(RunesTerm *t);
void runes_display_visual_bell(RunesTerm *t);
void runes_display_save_cursor(RunesTerm *t);
void runes_display_restore_cursor(RunesTerm *t);
void runes_display_use_alternate_buffer(RunesTerm *t);
void runes_display_use_normal_buffer(RunesTerm *t);
void runes_display_set_scroll_region(
    RunesTerm *t, int top, int bottom, int left, int right);
void runes_display_scroll_up(RunesTerm *t, int rows);

#endif
