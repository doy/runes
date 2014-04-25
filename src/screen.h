#ifndef _RUNES_SCREEN_H
#define _RUNES_SCREEN_H

#include <stdint.h>

enum RunesColorType {
    RUNES_COLOR_DEFAULT,
    RUNES_COLOR_IDX,
    RUNES_COLOR_RGB
};

struct runes_loc {
    int row;
    int col;
};

struct runes_color {
    union {
        struct {
            union {
                struct {
                    unsigned char r;
                    unsigned char g;
                    unsigned char b;
                };
                unsigned char idx;
            };
            unsigned char type;
        };
        uint32_t id;
    };
};

struct runes_cell_attrs {
    struct runes_color fgcolor;
    struct runes_color bgcolor;
    union {
        struct {
            unsigned char bold: 1;
            unsigned char italic: 1;
            unsigned char underline: 1;
            unsigned char inverse: 1;
        };
        unsigned char attrs;
    };
};

struct runes_cell {
    char contents[8];
    size_t len;
    struct runes_cell_attrs attrs;
};

struct runes_row {
    struct runes_cell *cells;
    unsigned char dirty: 1;
};

struct runes_screen {
    struct runes_loc cur;
    struct runes_loc max;
    struct runes_loc saved;

    int scroll_top;
    int scroll_bottom;

    char *title;
    size_t title_len;
    char *icon_name;
    size_t icon_name_len;

    struct runes_row *rows;
    struct runes_row *alternate;

    struct runes_cell_attrs attrs;

    unsigned char hide_cursor: 1;
    unsigned char application_keypad: 1;
    unsigned char application_cursor: 1;
    unsigned char mouse_reporting_press: 1;
    unsigned char mouse_reporting_press_release: 1;

    unsigned char visual_bell: 1;
    unsigned char audible_bell: 1;
    unsigned char update_title: 1;
    unsigned char update_icon_name: 1;
};

void runes_screen_init(RunesTerm *t);
void runes_screen_process_string(RunesTerm *t, char *buf, size_t len);
void runes_screen_audible_bell(RunesTerm *t);
void runes_screen_visual_bell(RunesTerm *t);
void runes_screen_show_string_ascii(RunesTerm *t, char *buf, size_t len);
void runes_screen_show_string_utf8(RunesTerm *t, char *buf, size_t len);
void runes_screen_move_to(RunesTerm *t, int row, int col);
void runes_screen_clear_screen(RunesTerm *t);
void runes_screen_clear_screen_forward(RunesTerm *t);
void runes_screen_clear_screen_backward(RunesTerm *t);
void runes_screen_kill_line(RunesTerm *t);
void runes_screen_kill_line_forward(RunesTerm *t);
void runes_screen_kill_line_backward(RunesTerm *t);
void runes_screen_insert_characters(RunesTerm *t, int count);
void runes_screen_insert_lines(RunesTerm *t, int count);
void runes_screen_delete_characters(RunesTerm *t, int count);
void runes_screen_delete_lines(RunesTerm *t, int count);
void runes_screen_set_scroll_region(
    RunesTerm *t, int top, int bottom, int left, int right);
void runes_screen_reset_text_attributes(RunesTerm *t);
void runes_screen_set_fg_color(RunesTerm *t, int idx);
void runes_screen_set_fg_color_rgb(RunesTerm *t, int r, int g, int b);
void runes_screen_reset_fg_color(RunesTerm *t);
void runes_screen_set_bg_color(RunesTerm *t, int idx);
void runes_screen_set_bg_color_rgb(RunesTerm *t, int r, int g, int b);
void runes_screen_reset_bg_color(RunesTerm *t);
void runes_screen_set_bold(RunesTerm *t);
void runes_screen_set_italic(RunesTerm *t);
void runes_screen_set_underline(RunesTerm *t);
void runes_screen_set_inverse(RunesTerm *t);
void runes_screen_reset_bold(RunesTerm *t);
void runes_screen_reset_italic(RunesTerm *t);
void runes_screen_reset_underline(RunesTerm *t);
void runes_screen_reset_inverse(RunesTerm *t);
void runes_screen_use_alternate_buffer(RunesTerm *t);
void runes_screen_use_normal_buffer(RunesTerm *t);
void runes_screen_save_cursor(RunesTerm *t);
void runes_screen_restore_cursor(RunesTerm *t);
void runes_screen_show_cursor(RunesTerm *t);
void runes_screen_hide_cursor(RunesTerm *t);
void runes_screen_set_application_keypad(RunesTerm *t);
void runes_screen_reset_application_keypad(RunesTerm *t);
void runes_screen_set_application_cursor(RunesTerm *t);
void runes_screen_reset_application_cursor(RunesTerm *t);
void runes_screen_set_mouse_reporting_press(RunesTerm *t);
void runes_screen_reset_mouse_reporting_press(RunesTerm *t);
void runes_screen_set_mouse_reporting_press_release(RunesTerm *t);
void runes_screen_reset_mouse_reporting_press_release(RunesTerm *t);
void runes_screen_set_window_title(RunesTerm *t, char *buf, size_t len);
void runes_screen_set_icon_name(RunesTerm *t, char *buf, size_t len);
void runes_screen_cleanup(RunesTerm *t);

#endif
