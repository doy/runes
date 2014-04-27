#include <stdlib.h>
#include <string.h>

#include "runes.h"
#include "parser.h"

void runes_screen_init(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int i;

    scr->rows = calloc(scr->max.row, sizeof(struct runes_row));
    for (i = 0; i < scr->max.row; ++i) {
        scr->rows[i].cells = calloc(
            scr->max.col, sizeof(struct runes_cell));
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
    RunesScreen *scr = &t->scr;

    scr->audible_bell = 1;
}

void runes_screen_visual_bell(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->visual_bell = 1;
}

void runes_screen_show_string_ascii(RunesTerm *t, char *buf, size_t len)
{
    RunesScreen *scr = &t->scr;
    size_t i;

    for (i = 0; i < len; ++i) {
        struct runes_cell *cell;

        if (scr->cur.col >= scr->max.col) {
            scr->rows[scr->cur.row].wrapped = 1;
            runes_screen_move_to(t, scr->cur.row + 1, 0);
        }
        cell = &scr->rows[scr->cur.row].cells[scr->cur.col];

        cell->len = 1;
        cell->contents[0] = buf[i];
        cell->attrs = scr->attrs;
        cell->is_wide = 0;

        runes_screen_move_to(t, scr->cur.row, scr->cur.col + 1);
    }
}

void runes_screen_show_string_utf8(RunesTerm *t, char *buf, size_t len)
{
    RunesScreen *scr = &t->scr;
    char *c = buf, *next;

    /* XXX need to detect combining characters and append them to the previous
     * cell */
    while ((next = g_utf8_next_char(c))) {
        gunichar uc;
        struct runes_cell *cell = NULL;
        int is_wide, is_combining;
        GUnicodeType ctype;

        uc = g_utf8_get_char(c);
        /* XXX handle zero width characters */
        is_wide = g_unichar_iswide(uc);
        ctype = g_unichar_type(uc);
        /* XXX should this also include spacing marks? */
        is_combining = ctype == G_UNICODE_ENCLOSING_MARK
                    || ctype == G_UNICODE_NON_SPACING_MARK;

        if (is_combining) {
            if (scr->cur.col > 0) {
                cell = &scr->rows[scr->cur.row].cells[scr->cur.col - 1];
            }
            else if (scr->cur.row > 0 && scr->rows[scr->cur.row - 1].wrapped) {
                cell = &scr->rows[scr->cur.row - 1].cells[scr->max.col - 1];
            }

            if (cell) {
                memcpy(cell->contents + cell->len, c, next - c);
                cell->len += next - c;
            }
        }
        else {
            if (scr->cur.col + (is_wide ? 2 : 1) > scr->max.col) {
                scr->rows[scr->cur.row].wrapped = 1;
                runes_screen_move_to(t, scr->cur.row + 1, 0);
            }
            cell = &scr->rows[scr->cur.row].cells[scr->cur.col];
            cell->is_wide = is_wide;

            cell->len = next - c;
            memcpy(cell->contents, c, cell->len);
            cell->attrs = scr->attrs;

            runes_screen_move_to(
                t, scr->cur.row, scr->cur.col + (is_wide ? 2 : 1));
        }

        c = next;
        if ((size_t)(c - buf) >= len) {
            break;
        }
    }
}

void runes_screen_move_to(RunesTerm *t, int row, int col)
{
    RunesScreen *scr = &t->scr;
    int top = scr->scroll_top, bottom = scr->scroll_bottom;

    /* XXX should be able to do this all in one operation */
    while (row > bottom) {
        free(scr->rows[top].cells);
        memmove(
            &scr->rows[top], &scr->rows[top + 1],
            (bottom - top) * sizeof(struct runes_row));
        scr->rows[bottom].cells = calloc(
            scr->max.col, sizeof(struct runes_cell));
        scr->rows[bottom].wrapped = 0;
        row--;
    }
    while (row < top) {
        free(scr->rows[bottom].cells);
        memmove(
            &scr->rows[top + 1], &scr->rows[top],
            (bottom - top) * sizeof(struct runes_row));
        scr->rows[top].cells = calloc(
            scr->max.col, sizeof(struct runes_cell));
        scr->rows[top].wrapped = 0;
        row++;
    }

    if (col < 0) {
        col = 0;
    }

    if (col > scr->max.col) {
        col = scr->max.col;
    }

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
        scr->rows[r].wrapped = 0;
    }
}

void runes_screen_clear_screen_forward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int r;

    memset(
        &scr->rows[scr->cur.row].cells[scr->cur.col], 0,
        (scr->max.col - scr->cur.col) * sizeof(struct runes_cell));
    scr->rows[scr->cur.row].wrapped = 0;
    for (r = scr->cur.row + 1; r < scr->max.row; ++r) {
        memset(
            scr->rows[r].cells, 0, scr->max.col * sizeof(struct runes_cell));
        scr->rows[r].wrapped = 0;
    }
}

void runes_screen_clear_screen_backward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int r;

    for (r = 0; r < scr->cur.row - 1; ++r) {
        memset(
            scr->rows[r].cells, 0, scr->max.col * sizeof(struct runes_cell));
        scr->rows[r].wrapped = 0;
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
    scr->rows[scr->cur.row].wrapped = 0;
}

void runes_screen_kill_line_forward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    memset(
        &scr->rows[scr->cur.row].cells[scr->cur.col], 0,
        (scr->max.col - scr->cur.col) * sizeof(struct runes_cell));
    scr->rows[scr->cur.row].wrapped = 0;
}

void runes_screen_kill_line_backward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    memset(
        scr->rows[scr->cur.row].cells, 0,
        scr->cur.col * sizeof(struct runes_cell));
    if (scr->cur.row > 0) {
        scr->rows[scr->cur.row - 1].wrapped = 0;
    }
}

void runes_screen_insert_characters(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row = &scr->rows[scr->cur.row];

    if (count >= scr->max.col - scr->cur.col) {
        runes_screen_kill_line_forward(t);
    }
    else {
        memmove(
            &row->cells[scr->cur.col + count], &row->cells[scr->cur.col],
            (scr->max.col - scr->cur.col - count) * sizeof(struct runes_cell));
        memset(
            &row->cells[scr->cur.col], 0,
            count * sizeof(struct runes_cell));
        scr->rows[scr->cur.row].wrapped = 0;
    }
}

void runes_screen_insert_lines(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;

    if (count >= scr->max.row - scr->cur.row) {
        runes_screen_clear_screen_forward(t);
        runes_screen_kill_line(t);
    }
    else {
        int i;

        for (i = scr->max.row - count; i < scr->max.row; ++i) {
            free(scr->rows[i].cells);
        }
        memmove(
            &scr->rows[scr->cur.row + count], &scr->rows[scr->cur.row],
            (scr->max.row - scr->cur.row - count) * sizeof(struct runes_row));
        memset(
            &scr->rows[scr->cur.row], 0,
            count * sizeof(struct runes_row));
        for (i = scr->cur.row; i < scr->cur.row + count; ++i) {
            scr->rows[i].cells = calloc(
                scr->max.col, sizeof(struct runes_cell));
            scr->rows[i].wrapped = 0;
        }
    }
}

void runes_screen_delete_characters(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row = &scr->rows[scr->cur.row];

    if (count >= scr->max.col - scr->cur.col) {
        runes_screen_kill_line_forward(t);
    }
    else {
        memmove(
            &row->cells[scr->cur.col], &row->cells[scr->cur.col + count],
            (scr->max.col - scr->cur.col - count) * sizeof(struct runes_cell));
        memset(
            &row->cells[scr->max.col - count], 0,
            count * sizeof(struct runes_cell));
        scr->rows[scr->cur.row].wrapped = 0;
    }
}

void runes_screen_delete_lines(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;

    if (count >= scr->max.row - scr->cur.row) {
        runes_screen_clear_screen_forward(t);
        runes_screen_kill_line(t);
    }
    else {
        int i;

        for (i = scr->cur.row; i < scr->cur.row + count; ++i) {
            free(scr->rows[i].cells);
        }
        memmove(
            &scr->rows[scr->cur.row], &scr->rows[scr->cur.row + count],
            (scr->max.row - scr->cur.row - count) * sizeof(struct runes_row));
        memset(
            &scr->rows[scr->max.row - count], 0,
            count * sizeof(struct runes_row));
        for (i = scr->max.row - count; i < scr->max.row; ++i) {
            scr->rows[i].cells = calloc(
                scr->max.col, sizeof(struct runes_cell));
            scr->rows[i].wrapped = 0;
        }
    }
}

void runes_screen_set_scroll_region(
    RunesTerm *t, int top, int bottom, int left, int right)
{
    RunesScreen *scr = &t->scr;

    if (left > 0 || right < scr->max.col - 1) {
        fprintf(stderr, "vertical scroll regions not yet implemented\n");
    }

    if (top > bottom) {
        return;
    }

    scr->scroll_top    = top    <  0            ? 0                : top;
    scr->scroll_bottom = bottom >= scr->max.row ? scr->max.row - 1 : bottom;

    runes_screen_move_to(t, scr->scroll_top, 0);
}

void runes_screen_reset_text_attributes(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    memset(&scr->attrs, 0, sizeof(struct runes_cell_attrs));
}

void runes_screen_set_fg_color(RunesTerm *t, int idx)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.fgcolor.type = RUNES_COLOR_IDX;
    scr->attrs.fgcolor.idx = idx;
}

void runes_screen_set_fg_color_rgb(
    RunesTerm *t, unsigned char r, unsigned char g, unsigned char b)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.fgcolor.type = RUNES_COLOR_RGB;
    scr->attrs.fgcolor.r = r;
    scr->attrs.fgcolor.g = g;
    scr->attrs.fgcolor.b = b;
}

void runes_screen_reset_fg_color(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.fgcolor.type = RUNES_COLOR_DEFAULT;
}

void runes_screen_set_bg_color(RunesTerm *t, int idx)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.bgcolor.type = RUNES_COLOR_IDX;
    scr->attrs.bgcolor.idx = idx;
}

void runes_screen_set_bg_color_rgb(
    RunesTerm *t, unsigned char r, unsigned char g, unsigned char b)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.bgcolor.type = RUNES_COLOR_RGB;
    scr->attrs.bgcolor.r = r;
    scr->attrs.bgcolor.g = g;
    scr->attrs.bgcolor.b = b;
}

void runes_screen_reset_bg_color(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.bgcolor.type = RUNES_COLOR_DEFAULT;
}

void runes_screen_set_bold(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.bold = 1;
}

void runes_screen_set_italic(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.italic = 1;
}

void runes_screen_set_underline(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.underline = 1;
}

void runes_screen_set_inverse(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.inverse = 1;
}

void runes_screen_reset_bold(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.bold = 0;
}

void runes_screen_reset_italic(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.italic = 0;
}

void runes_screen_reset_underline(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.underline = 0;
}

void runes_screen_reset_inverse(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->attrs.inverse = 0;
}

void runes_screen_use_alternate_buffer(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int i;

    if (scr->alternate) {
        return;
    }

    runes_screen_save_cursor(t);

    scr->alternate = scr->rows;

    scr->rows = calloc(scr->max.row, sizeof(struct runes_row));
    for (i = 0; i < scr->max.row; ++i) {
        scr->rows[i].cells = calloc(
            scr->max.col, sizeof(struct runes_cell));
    }
}

void runes_screen_use_normal_buffer(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int i;

    if (!scr->alternate) {
        return;
    }

    for (i = 0; i < scr->max.row; ++i) {
        free(scr->rows[i].cells);
    }
    free(scr->rows);

    scr->rows = scr->alternate;
    scr->alternate = NULL;

    runes_screen_restore_cursor(t);
}

void runes_screen_save_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->saved = scr->cur;
}

void runes_screen_restore_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->cur = scr->saved;
}

void runes_screen_show_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->hide_cursor = 0;
}

void runes_screen_hide_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->hide_cursor = 1;
}

void runes_screen_set_application_keypad(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->application_keypad = 1;
}

void runes_screen_reset_application_keypad(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->application_keypad = 0;
}

void runes_screen_set_application_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->application_cursor = 1;
}

void runes_screen_reset_application_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->application_cursor = 0;
}

void runes_screen_set_mouse_reporting_press(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->mouse_reporting_press = 1;
}

void runes_screen_reset_mouse_reporting_press(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->mouse_reporting_press = 0;
}

void runes_screen_set_mouse_reporting_press_release(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->mouse_reporting_press_release = 1;
}

void runes_screen_reset_mouse_reporting_press_release(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->mouse_reporting_press_release = 0;
}

void runes_screen_set_window_title(RunesTerm *t, char *buf, size_t len)
{
    RunesScreen *scr = &t->scr;

    free(scr->title);
    scr->title_len = len;
    scr->title = malloc(scr->title_len);
    memcpy(scr->title, buf, scr->title_len);
    scr->update_title = 1;
}

void runes_screen_set_icon_name(RunesTerm *t, char *buf, size_t len)
{
    RunesScreen *scr = &t->scr;

    free(scr->icon_name);
    scr->icon_name_len = len;
    scr->icon_name = malloc(scr->icon_name_len);
    memcpy(scr->icon_name, buf, scr->icon_name_len);
    scr->update_icon_name = 1;
}

void runes_screen_cleanup(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int i;

    for (i = 0; i < scr->max.row; ++i) {
        free(scr->rows[i].cells);
    }
    free(scr->rows);

    free(scr->title);
    free(scr->icon_name);
}
