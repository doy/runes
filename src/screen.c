#include <stdlib.h>
#include <string.h>

#include "runes.h"
#include "parser.h"

void runes_screen_init(RunesTerm *t)
{
    int i;

    t->scr.rows = calloc(t->scr.max.row, sizeof(struct runes_row));
    for (i = 0; i < t->scr.max.row; ++i) {
        t->scr.rows[i].cells = calloc(
            t->scr.max.col, sizeof(struct runes_cell));
    }
}

void runes_screen_process_string(RunesTerm *t, char *buf, size_t len)
{
    YY_BUFFER_STATE state;
    yyscan_t scanner;
    int remaining;

    /* XXX this will break if buf ends with a partial escape sequence or utf8
     * character. we need to detect that and not consume the entire input in
     * that case */
    runes_parser_yylex_init_extra(t, &scanner);
    state = runes_parser_yy_scan_bytes(buf, len, scanner);
    while ((remaining = runes_parser_yylex(scanner)) == -1);
    t->remaininglen = remaining;
    if (t->remaininglen) {
        memmove(t->readbuf, &buf[len - t->remaininglen], t->remaininglen);
    }
    runes_parser_yy_delete_buffer(state, scanner);
    runes_parser_yylex_destroy(scanner);

    runes_display_draw_screen(t);
}

void runes_screen_audible_bell(RunesTerm *t)
{
    t->scr.audible_bell = 1;
}

void runes_screen_visual_bell(RunesTerm *t)
{
    t->scr.visual_bell = 1;
}

void runes_screen_show_string_ascii(RunesTerm *t, char *buf, size_t len)
{
    RunesScreen *scr = &t->scr;
    size_t i;

    for (i = 0; i < len; ++i) {
        struct runes_cell *cell;

        if (scr->cur.col >= scr->max.col) {
            scr->cur.col = 0;
            scr->cur.row++;
        }
        cell = &scr->rows[scr->cur.row].cells[scr->cur.col];

        cell->contents[0] = buf[i];
        cell->contents[1] = '\0';
        cell->len = 1;

        scr->cur.col++;
    }
}

void runes_screen_show_string_utf8(RunesTerm *t, char *buf, size_t len)
{
    UNUSED(t);
    UNUSED(buf);
    UNUSED(len);
    fprintf(stderr, "show_string_utf8 nyi\n");
}

void runes_screen_move_to(RunesTerm *t, int row, int col)
{
    RunesScreen *scr = &t->scr;

    scr->cur.row = row;
    scr->cur.col = col;
}

void runes_screen_clear_screen(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int r;

    for (r = 0; r < scr->max.row; ++r) {
        memset(
            scr->rows[r].cells, 0, scr->max.col * sizeof(struct runes_cell));
    }
}

void runes_screen_clear_screen_forward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int r;

    memset(
        &scr->rows[scr->cur.row].cells[scr->cur.col], 0,
        (scr->max.col - scr->cur.col) * sizeof(struct runes_cell));
    for (r = scr->cur.row + 1; r < scr->max.row; ++r) {
        memset(
            scr->rows[r].cells, 0, scr->max.col * sizeof(struct runes_cell));
    }
}

void runes_screen_clear_screen_backward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int r;

    for (r = 0; r < scr->cur.row - 1; ++r) {
        memset(
            scr->rows[r].cells, 0, scr->max.col * sizeof(struct runes_cell));
    }
    memset(
        scr->rows[scr->cur.row].cells, 0,
        scr->cur.col * sizeof(struct runes_cell));
}

void runes_screen_kill_line(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    memset(
        scr->rows[scr->cur.row].cells, 0,
        scr->max.col * sizeof(struct runes_cell));
}

void runes_screen_kill_line_forward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    memset(
        &scr->rows[scr->cur.row].cells[scr->cur.col], 0,
        (scr->max.col - scr->cur.col) * sizeof(struct runes_cell));
}

void runes_screen_kill_line_backward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    memset(
        scr->rows[scr->cur.row].cells, 0,
        scr->cur.col * sizeof(struct runes_cell));
}

void runes_screen_insert_characters(RunesTerm *t, int count)
{
    UNUSED(t);
    UNUSED(count);
    fprintf(stderr, "insert_characters nyi\n");
}

void runes_screen_insert_lines(RunesTerm *t, int count)
{
    UNUSED(t);
    UNUSED(count);
    fprintf(stderr, "insert_lines nyi\n");
}

void runes_screen_delete_characters(RunesTerm *t, int count)
{
    UNUSED(t);
    UNUSED(count);
    fprintf(stderr, "delete_characters nyi\n");
}

void runes_screen_delete_lines(RunesTerm *t, int count)
{
    UNUSED(t);
    UNUSED(count);
    fprintf(stderr, "delete_lines nyi\n");
}

void runes_screen_set_scroll_region(
    RunesTerm *t, int top, int bottom, int left, int right)
{
    UNUSED(t);
    UNUSED(top);
    UNUSED(bottom);
    UNUSED(left);
    UNUSED(right);
    fprintf(stderr, "set_scroll_region nyi\n");
}

void runes_screen_reset_text_attributes(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_text_attributes nyi\n");
}

void runes_screen_set_fg_color(RunesTerm *t, int idx)
{
    UNUSED(t);
    UNUSED(idx);
    fprintf(stderr, "set_fg_color nyi\n");
}

void runes_screen_set_fg_color_rgb(RunesTerm *t, int r, int g, int b)
{
    UNUSED(t);
    UNUSED(r);
    UNUSED(g);
    UNUSED(b);
    fprintf(stderr, "set_fg_color_rgb nyi\n");
}

void runes_screen_reset_fg_color(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_fg_color nyi\n");
}

void runes_screen_set_bg_color(RunesTerm *t, int idx)
{
    UNUSED(t);
    UNUSED(idx);
    fprintf(stderr, "set_bg_color nyi\n");
}

void runes_screen_set_bg_color_rgb(RunesTerm *t, int r, int g, int b)
{
    UNUSED(t);
    UNUSED(r);
    UNUSED(g);
    UNUSED(b);
    fprintf(stderr, "set_bg_color_rgb nyi\n");
}

void runes_screen_reset_bg_color(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_bg_color nyi\n");
}

void runes_screen_set_bold(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_bold nyi\n");
}

void runes_screen_set_italic(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_italic nyi\n");
}

void runes_screen_set_underline(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_underline nyi\n");
}

void runes_screen_set_inverse(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_inverse nyi\n");
}

void runes_screen_reset_bold(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_bold nyi\n");
}

void runes_screen_reset_italic(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_italic nyi\n");
}

void runes_screen_reset_underline(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_underline nyi\n");
}

void runes_screen_reset_inverse(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_inverse nyi\n");
}

void runes_screen_use_alternate_buffer(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "use_alternate_buffer nyi\n");
}

void runes_screen_use_normal_buffer(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "use_normal_buffer nyi\n");
}

void runes_screen_save_cursor(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "save_cursor nyi\n");
}

void runes_screen_restore_cursor(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "restore_cursor nyi\n");
}

void runes_screen_show_cursor(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "show_cursor nyi\n");
}

void runes_screen_hide_cursor(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "hide_cursor nyi\n");
}

void runes_screen_set_application_keypad(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_application_keypad nyi\n");
}

void runes_screen_reset_application_keypad(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_application_keypad nyi\n");
}

void runes_screen_set_application_cursor(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_application_cursor nyi\n");
}

void runes_screen_reset_application_cursor(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_application_cursor nyi\n");
}

void runes_screen_set_mouse_reporting_press(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_mouse_reporting_press nyi\n");
}

void runes_screen_reset_mouse_reporting_press(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_mouse_reporting_press nyi\n");
}

void runes_screen_set_mouse_reporting_press_release(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "set_mouse_reporting_press_release nyi\n");
}

void runes_screen_reset_mouse_reporting_press_release(RunesTerm *t)
{
    UNUSED(t);
    fprintf(stderr, "reset_mouse_reporting_press_release nyi\n");
}

void runes_screen_set_window_title(RunesTerm *t, char *buf, size_t len)
{
    UNUSED(t);
    UNUSED(buf);
    UNUSED(len);
    fprintf(stderr, "set_window_title nyi\n");
}

void runes_screen_set_icon_name(RunesTerm *t, char *buf, size_t len)
{
    UNUSED(t);
    UNUSED(buf);
    UNUSED(len);
    fprintf(stderr, "set_icon_name nyi\n");
}

void runes_screen_cleanup(RunesTerm *t)
{
    int i;

    for (i = 0; i < t->scr.max.row; ++i) {
        free(t->scr.rows[i].cells);
    }
    free(t->scr.rows);
}
