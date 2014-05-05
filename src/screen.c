#include <stdlib.h>
#include <string.h>

#include "runes.h"
#include "parser.h"

static void runes_screen_ensure_capacity(RunesTerm *t, int size);
static struct runes_row *runes_screen_row_at(RunesTerm *t, int row);
static struct runes_cell *runes_screen_cell_at(RunesTerm *t, int row, int col);
static void runes_screen_scroll_down(RunesTerm *t, int count);
static void runes_screen_scroll_up(RunesTerm *t, int count);
static int runes_screen_scroll_region_is_active(RunesTerm *t);

void runes_screen_init(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->grid = calloc(1, sizeof(struct runes_grid));
}

void runes_screen_set_window_size(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    struct runes_loc old_size;
    int i;

    old_size.row = scr->grid->max.row;
    old_size.col = scr->grid->max.col;

    scr->grid->max.row = t->display.ypixel / t->display.fonty;
    scr->grid->max.col = t->display.xpixel / t->display.fontx;

    if (scr->grid->max.row == 0) {
        scr->grid->max.row = 1;
    }
    if (scr->grid->max.col == 0) {
        scr->grid->max.col = 1;
    }

    if (scr->grid->max.row == old_size.row && scr->grid->max.col == old_size.col) {
        return;
    }

    if (scr->grid->cur.row >= scr->grid->max.row) {
        scr->grid->cur.row = scr->grid->max.row - 1;
    }
    if (scr->grid->cur.col > scr->grid->max.col) {
        scr->grid->cur.col = scr->grid->max.col;
    }

    runes_screen_ensure_capacity(t, scr->grid->max.row);

    for (i = 0; i < scr->grid->row_count; ++i) {
        scr->grid->rows[i].cells = realloc(
            scr->grid->rows[i].cells,
            scr->grid->max.col * sizeof(struct runes_cell));
        if (old_size.col > scr->grid->max.col) {
            memset(
                &scr->grid->rows[i].cells[scr->grid->max.col], 0,
                (old_size.col - scr->grid->max.col) * sizeof(struct runes_cell));
        }
    }

    for (i = scr->grid->row_count; i < scr->grid->max.row; ++i) {
        scr->grid->rows[i].cells = calloc(
            scr->grid->max.col, sizeof(struct runes_cell));
    }

    if (scr->grid->row_count < scr->grid->max.row) {
        scr->grid->row_count = scr->grid->max.row;
        scr->grid->row_top = 0;
    }
    else {
        scr->grid->row_top = scr->grid->row_count - scr->grid->max.row;
    }

    scr->grid->scroll_top    = 0;
    scr->grid->scroll_bottom = scr->grid->max.row - 1;
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
    remaining = runes_parser_yylex(scanner);
    t->pty.remaininglen = remaining;
    if (t->pty.remaininglen) {
        memmove(
            t->pty.readbuf, &buf[len - t->pty.remaininglen],
            t->pty.remaininglen);
    }
    runes_parser_yy_delete_buffer(state, scanner);
    runes_parser_yylex_destroy(scanner);
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

        if (scr->grid->cur.col >= scr->grid->max.col) {
            runes_screen_row_at(t, scr->grid->cur.row)->wrapped = 1;
            runes_screen_move_to(t, scr->grid->cur.row + 1, 0);
        }
        cell = runes_screen_cell_at(t, scr->grid->cur.row, scr->grid->cur.col);

        cell->len = 1;
        cell->contents[0] = buf[i];
        cell->attrs = scr->attrs;
        cell->is_wide = 0;

        runes_screen_move_to(t, scr->grid->cur.row, scr->grid->cur.col + 1);
    }

    scr->dirty = 1;
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
            if (scr->grid->cur.col > 0) {
                cell = runes_screen_cell_at(
                    t, scr->grid->cur.row, scr->grid->cur.col - 1);
            }
            else if (scr->grid->cur.row > 0 && runes_screen_row_at(t, scr->grid->cur.row - 1)->wrapped) {
                cell = runes_screen_cell_at(
                    t, scr->grid->cur.row - 1, scr->grid->max.col - 1);
            }

            if (cell) {
                char *normal;

                memcpy(cell->contents + cell->len, c, next - c);
                cell->len += next - c;
                /* some fonts have combined characters but can't handle
                 * combining characters, so try to fix that here */
                /* XXX it'd be nice if there was a way to do this that didn't
                 * require an allocation */
                normal = g_utf8_normalize(
                    cell->contents, cell->len, G_NORMALIZE_NFC);
                memcpy(cell->contents, normal, cell->len);
                free(normal);
            }
        }
        else {
            if (scr->grid->cur.col + (is_wide ? 2 : 1) > scr->grid->max.col) {
                runes_screen_row_at(t, scr->grid->cur.row)->wrapped = 1;
                runes_screen_move_to(t, scr->grid->cur.row + 1, 0);
            }
            cell = runes_screen_cell_at(
                t, scr->grid->cur.row, scr->grid->cur.col);
            cell->is_wide = is_wide;

            cell->len = next - c;
            memcpy(cell->contents, c, cell->len);
            cell->attrs = scr->attrs;

            runes_screen_move_to(
                t, scr->grid->cur.row, scr->grid->cur.col + (is_wide ? 2 : 1));
        }

        c = next;
        if ((size_t)(c - buf) >= len) {
            break;
        }
    }

    scr->dirty = 1;
}

void runes_screen_move_to(RunesTerm *t, int row, int col)
{
    RunesScreen *scr = &t->scr;
    int top = scr->grid->scroll_top, bottom = scr->grid->scroll_bottom;

    if (row > bottom) {
        runes_screen_scroll_down(t, row - bottom);
        row = bottom;
    }
    else if (row < top) {
        runes_screen_scroll_up(t, top - row);
        row = top;
    }

    if (col < 0) {
        col = 0;
    }

    if (col > scr->grid->max.col) {
        col = scr->grid->max.col;
    }

    scr->grid->cur.row = row;
    scr->grid->cur.col = col;
}

void runes_screen_clear_screen(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int r;

    for (r = 0; r < scr->grid->max.row; ++r) {
        struct runes_row *row;

        row = runes_screen_row_at(t, r);
        memset(row->cells, 0, scr->grid->max.col * sizeof(struct runes_cell));
        row->wrapped = 0;
    }

    scr->dirty = 1;
}

void runes_screen_clear_screen_forward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;
    int r;

    row = runes_screen_row_at(t, scr->grid->cur.row);
    memset(
        &row->cells[scr->grid->cur.col], 0,
        (scr->grid->max.col - scr->grid->cur.col) * sizeof(struct runes_cell));
    row->wrapped = 0;
    for (r = scr->grid->cur.row + 1; r < scr->grid->max.row; ++r) {
        row = runes_screen_row_at(t, r);
        memset(row->cells, 0, scr->grid->max.col * sizeof(struct runes_cell));
        row->wrapped = 0;
    }

    scr->dirty = 1;
}

void runes_screen_clear_screen_backward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;
    int r;

    for (r = 0; r < scr->grid->cur.row - 1; ++r) {
        row = runes_screen_row_at(t, r);
        memset(row->cells, 0, scr->grid->max.col * sizeof(struct runes_cell));
        row->wrapped = 0;
    }
    row = runes_screen_row_at(t, scr->grid->cur.row);
    memset(row->cells, 0, scr->grid->cur.col * sizeof(struct runes_cell));

    scr->dirty = 1;
}

void runes_screen_kill_line(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;

    row = runes_screen_row_at(t, scr->grid->cur.row);
    memset(row->cells, 0, scr->grid->max.col * sizeof(struct runes_cell));
    row->wrapped = 0;

    scr->dirty = 1;
}

void runes_screen_kill_line_forward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;

    row = runes_screen_row_at(t, scr->grid->cur.row);
    memset(
        &row->cells[scr->grid->cur.col], 0,
        (scr->grid->max.col - scr->grid->cur.col) * sizeof(struct runes_cell));
    row->wrapped = 0;

    scr->dirty = 1;
}

void runes_screen_kill_line_backward(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;

    row = runes_screen_row_at(t, scr->grid->cur.row);
    memset(row->cells, 0, scr->grid->cur.col * sizeof(struct runes_cell));
    if (scr->grid->cur.row > 0) {
        row = runes_screen_row_at(t, scr->grid->cur.row - 1);
        row->wrapped = 0;
    }

    scr->dirty = 1;
}

void runes_screen_insert_characters(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;

    row = runes_screen_row_at(t, scr->grid->cur.row);
    if (count >= scr->grid->max.col - scr->grid->cur.col) {
        runes_screen_kill_line_forward(t);
    }
    else {
        memmove(
            &row->cells[scr->grid->cur.col + count],
            &row->cells[scr->grid->cur.col],
            (scr->grid->max.col - scr->grid->cur.col - count) * sizeof(struct runes_cell));
        memset(
            &row->cells[scr->grid->cur.col], 0,
            count * sizeof(struct runes_cell));
        row->wrapped = 0;
    }

    scr->dirty = 1;
}

void runes_screen_insert_lines(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;

    if (count >= scr->grid->max.row - scr->grid->cur.row) {
        runes_screen_clear_screen_forward(t);
        runes_screen_kill_line(t);
    }
    else {
        struct runes_row *row;
        int i;

        for (i = scr->grid->max.row - count; i < scr->grid->max.row; ++i) {
            row = runes_screen_row_at(t, i);
            free(row->cells);
        }
        row = runes_screen_row_at(t, scr->grid->cur.row);
        memmove(
            row + count, row,
            (scr->grid->max.row - scr->grid->cur.row - count) * sizeof(struct runes_row));
        memset(row, 0, count * sizeof(struct runes_row));
        for (i = scr->grid->cur.row; i < scr->grid->cur.row + count; ++i) {
            row = runes_screen_row_at(t, i);
            row->cells = calloc(scr->grid->max.col, sizeof(struct runes_cell));
            row->wrapped = 0;
        }
    }

    scr->dirty = 1;
}

void runes_screen_delete_characters(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;

    if (count >= scr->grid->max.col - scr->grid->cur.col) {
        runes_screen_kill_line_forward(t);
    }
    else {
        struct runes_row *row;

        row = runes_screen_row_at(t, scr->grid->cur.row);
        memmove(
            &row->cells[scr->grid->cur.col],
            &row->cells[scr->grid->cur.col + count],
            (scr->grid->max.col - scr->grid->cur.col - count) * sizeof(struct runes_cell));
        memset(
            &row->cells[scr->grid->max.col - count], 0,
            count * sizeof(struct runes_cell));
        row->wrapped = 0;
    }

    scr->dirty = 1;
}

void runes_screen_delete_lines(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;

    if (count >= scr->grid->max.row - scr->grid->cur.row) {
        runes_screen_clear_screen_forward(t);
        runes_screen_kill_line(t);
    }
    else {
        struct runes_row *row;
        int i;

        for (i = scr->grid->cur.row; i < scr->grid->cur.row + count; ++i) {
            row = runes_screen_row_at(t, i);
            free(row->cells);
        }
        row = runes_screen_row_at(t, scr->grid->cur.row);
        memmove(
            row, row + count,
            (scr->grid->max.row - scr->grid->cur.row - count) * sizeof(struct runes_row));
        row = runes_screen_row_at(t, scr->grid->max.row - count);
        memset(row, 0, count * sizeof(struct runes_row));
        for (i = scr->grid->max.row - count; i < scr->grid->max.row; ++i) {
            row = runes_screen_row_at(t, i);
            row->cells = calloc(scr->grid->max.col, sizeof(struct runes_cell));
            row->wrapped = 0;
        }
    }

    scr->dirty = 1;
}

void runes_screen_set_scroll_region(
    RunesTerm *t, int top, int bottom, int left, int right)
{
    RunesScreen *scr = &t->scr;

    if (left > 0 || right < scr->grid->max.col - 1) {
        fprintf(stderr, "vertical scroll regions not yet implemented\n");
    }

    if (top > bottom) {
        return;
    }

    scr->grid->scroll_top = top < 0
        ? 0
        : top;
    scr->grid->scroll_bottom = bottom >= scr->grid->max.row
        ? scr->grid->max.row - 1
        : bottom;

    runes_screen_move_to(t, scr->grid->scroll_top, 0);
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

    if (scr->alternate) {
        return;
    }

    scr->alternate = scr->grid;
    scr->grid = calloc(1, sizeof(struct runes_grid));
    runes_screen_set_window_size(t);

    scr->dirty = 1;
}

void runes_screen_use_normal_buffer(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;
    int i;

    if (!scr->alternate) {
        return;
    }

    for (i = 0; i < scr->grid->row_count; ++i) {
        free(scr->grid->rows[i].cells);
    }
    free(scr->grid->rows);
    free(scr->grid);

    scr->grid = scr->alternate;
    scr->alternate = NULL;

    runes_screen_set_window_size(t);

    scr->dirty = 1;
}

void runes_screen_save_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->grid->saved = scr->grid->cur;
}

void runes_screen_restore_cursor(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    scr->grid->cur = scr->grid->saved;
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

    for (i = 0; i < scr->grid->row_count; ++i) {
        free(scr->grid->rows[i].cells);
    }
    free(scr->grid->rows);
    free(scr->grid);

    free(scr->title);
    free(scr->icon_name);
}

static void runes_screen_ensure_capacity(RunesTerm *t, int size)
{
    RunesScreen *scr = &t->scr;
    int old_capacity = scr->grid->row_capacity;

    if (scr->grid->row_capacity >= size) {
        return;
    }

    if (scr->grid->row_capacity == 0) {
        scr->grid->row_capacity = scr->grid->max.row;
    }

    while (scr->grid->row_capacity < size) {
        scr->grid->row_capacity *= 1.5;
    }

    scr->grid->rows = realloc(
        scr->grid->rows, scr->grid->row_capacity * sizeof(struct runes_row));
    memset(
        &scr->grid->rows[old_capacity], 0,
        (scr->grid->row_capacity - old_capacity) * sizeof(struct runes_row));
}

static struct runes_row *runes_screen_row_at(RunesTerm *t, int row)
{
    RunesScreen *scr = &t->scr;

    return &scr->grid->rows[row + scr->grid->row_top];
}

static struct runes_cell *runes_screen_cell_at(RunesTerm *t, int row, int col)
{
    RunesScreen *scr = &t->scr;

    return &scr->grid->rows[row + scr->grid->row_top].cells[col];
}

static void runes_screen_scroll_down(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;
    int i;

    if (runes_screen_scroll_region_is_active(t) || scr->alternate) {
        int bottom = scr->grid->scroll_bottom, top = scr->grid->scroll_top;

        if (bottom - top + 1 > count) {
            for (i = 0; i < count; ++i) {
                row = runes_screen_row_at(t, top + i);
                free(row->cells);
            }
            row = runes_screen_row_at(t, top);
            memmove(
                row, row + count,
                (bottom - top + 1 - count) * sizeof(struct runes_row));
            for (i = 0; i < count; ++i) {
                row = runes_screen_row_at(t, bottom - i);
                row->cells = calloc(
                    scr->grid->max.col, sizeof(struct runes_cell));
                row->wrapped = 0;
            }
        }
        else {
            for (i = 0; i < bottom - top + 1; ++i) {
                row = runes_screen_row_at(t, top + i);
                memset(
                    row->cells, 0,
                    scr->grid->max.col * sizeof(struct runes_cell));
                row->wrapped = 0;
            }
        }
    }
    else {
        int scrollback = t->config.scrollback_length;

        if (scr->grid->row_count + count > scrollback) {
            int overflow = scr->grid->row_count + count - scrollback;

            runes_screen_ensure_capacity(t, scrollback);
            for (i = 0; i < overflow; ++i) {
                free(scr->grid->rows[i].cells);
            }
            memmove(
                &scr->grid->rows[0], &scr->grid->rows[overflow],
                (scrollback - overflow) * sizeof(struct runes_row));
            for (i = scrollback - count; i < scrollback; ++i) {
                scr->grid->rows[i].cells = calloc(
                    scr->grid->max.col, sizeof(struct runes_cell));
            }
            scr->grid->row_count = scrollback;
            scr->grid->row_top = scrollback - scr->grid->max.row;
        }
        else {
            runes_screen_ensure_capacity(t, scr->grid->row_count + count);
            for (i = 0; i < count; ++i) {
                row = runes_screen_row_at(t, i + scr->grid->max.row);
                row->cells = calloc(
                    scr->grid->max.col, sizeof(struct runes_cell));
            }
            scr->grid->row_count += count;
            scr->grid->row_top += count;
        }
    }
}

static void runes_screen_scroll_up(RunesTerm *t, int count)
{
    RunesScreen *scr = &t->scr;
    struct runes_row *row;
    int bottom = scr->grid->scroll_bottom, top = scr->grid->scroll_top;
    int i;

    if (bottom - top + 1 > count) {
        for (i = 0; i < count; ++i) {
            row = runes_screen_row_at(t, bottom - i);
            free(row->cells);
        }
        row = runes_screen_row_at(t, top);
        memmove(
            row + count, row,
            (bottom - top + 1 - count) * sizeof(struct runes_row));
        for (i = 0; i < count; ++i) {
            row = runes_screen_row_at(t, top + i);
            row->cells = calloc(scr->grid->max.col, sizeof(struct runes_cell));
            row->wrapped = 0;
        }
    }
    else {
        for (i = 0; i < bottom - top + 1; ++i) {
            row = runes_screen_row_at(t, top + i);
            memset(
                row->cells, 0, scr->grid->max.col * sizeof(struct runes_cell));
            row->wrapped = 0;
        }
    }
}

static int runes_screen_scroll_region_is_active(RunesTerm *t)
{
    RunesScreen *scr = &t->scr;

    return scr->grid->scroll_top != 0
        || scr->grid->scroll_bottom != scr->grid->max.row - 1;
}
