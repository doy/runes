%{
#include <string.h>

#include "runes.h"

#define RUNES_PARSER_CSI_MAX_PARAMS 256

#define YY_EXIT_FAILURE (UNUSED(yyscanner), 2)
%}

%option reentrant nodefault batch
%option noyywrap nounput noinput noyyalloc noyyrealloc noyyfree
%option prefix="runes_parser_yy"
%option extra-type="RunesTerm *"

CTRL    [\000-\037\177]
ASCII   [\040-\176]
LEAD2   [\300-\337]
LEAD3   [\340-\357]
LEAD4   [\360-\367]
CONT    [\200-\277]
UNICHAR ({LEAD2}{CONT}|{LEAD3}{CONT}{CONT}|{LEAD4}{CONT}{CONT}{CONT})
CHAR    ({ASCII}|{UNICHAR})

ST  \007
BEL \007
BS  \010
TAB \011
LF  \012
VT  \013
FF  \014
CR  \015
ESC \033

DECKPAM {ESC}=
DECKPNM {ESC}>
CSI     {ESC}\[
OSC     {ESC}\]
RI      {ESC}M
VB      {ESC}g
DECSC   {ESC}7
DECRC   {ESC}8

DECCSI     {CSI}\?
CSIPARAM1  ([0-9]+)?
CSIPARAM2  ([0-9]+(;[0-9]+)?)?
CSIPARAM24 ([0-9]+(;[0-9]+){1,3})?
CSIPARAMS  ([0-9]+(;[0-9]+)*)?
SMPARAMS   ([<=?]?[0-9]+(;[<=?]?[0-9]+)*)?

CUU {CSI}{CSIPARAM1}A
CUD {CSI}{CSIPARAM1}B
CUF {CSI}{CSIPARAM1}C
CUB {CSI}{CSIPARAM1}D
CUP {CSI}{CSIPARAM2}H
ED  {CSI}{CSIPARAM1}J
EL  {CSI}{CSIPARAM1}K
IL  {CSI}{CSIPARAM1}L
DCH {CSI}{CSIPARAM1}P
SM  {CSI}{SMPARAMS}h
RM  {CSI}{SMPARAMS}l
SGR {CSI}{CSIPARAMS}m
CSR {CSI}{CSIPARAM24}r

DECSED {DECCSI}{CSIPARAM1}J
DECSEL {DECCSI}{CSIPARAM1}K

OSC0 {OSC}0;{CHAR}*{ST}
OSC1 {OSC}1;{CHAR}*{ST}
OSC2 {OSC}2;{CHAR}*{ST}

%{
static void runes_parser_handle_bel(RunesTerm *t);
static void runes_parser_handle_bs(RunesTerm *t);
static void runes_parser_handle_tab(RunesTerm *t);
static void runes_parser_handle_lf(RunesTerm *t);
static void runes_parser_handle_cr(RunesTerm *t);
static void runes_parser_handle_deckpam(RunesTerm *t);
static void runes_parser_handle_deckpnm(RunesTerm *t);
static void runes_parser_handle_ri(RunesTerm *t);
static void runes_parser_handle_vb(RunesTerm *t);
static void runes_parser_handle_decsc(RunesTerm *t);
static void runes_parser_handle_decrc(RunesTerm *t);
static void runes_parser_extract_csi_params(
    char *buf, size_t len, int *params, int *nparams);
static void runes_parser_extract_sm_params(
    char *buf, size_t len, char *modes, int *params, int *nparams);
static void runes_parser_handle_cuu(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cud(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cuf(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cub(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cup(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_ed(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_el(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_il(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_dch(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_sm(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_rm(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_sgr(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_csr(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_decsed(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_decsel(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_osc0(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_osc1(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_osc2(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_ascii(RunesTerm *t, char *text, size_t len);
static void runes_parser_handle_text(RunesTerm *t, char *text, size_t len);
%}

%%

{BEL} runes_parser_handle_bel(yyextra); return -1;
{BS}  runes_parser_handle_bs(yyextra); return -1;
{TAB} runes_parser_handle_tab(yyextra); return -1;
{LF}  |
{VT}  |
{FF}  runes_parser_handle_lf(yyextra); return -1;
{CR}  runes_parser_handle_cr(yyextra); return -1;

{DECKPAM} runes_parser_handle_deckpam(yyextra); return -1;
{DECKPNM} runes_parser_handle_deckpnm(yyextra); return -1;
{RI}      runes_parser_handle_ri(yyextra); return -1;
{VB}      runes_parser_handle_vb(yyextra); return -1;
{DECSC}   runes_parser_handle_decsc(yyextra); return -1;
{DECRC}   runes_parser_handle_decrc(yyextra); return -1;

{CUU} runes_parser_handle_cuu(yyextra, yytext, yyleng); return -1;
{CUD} runes_parser_handle_cud(yyextra, yytext, yyleng); return -1;
{CUF} runes_parser_handle_cuf(yyextra, yytext, yyleng); return -1;
{CUB} runes_parser_handle_cub(yyextra, yytext, yyleng); return -1;
{CUP} runes_parser_handle_cup(yyextra, yytext, yyleng); return -1;
{ED}  runes_parser_handle_ed(yyextra, yytext, yyleng); return -1;
{EL}  runes_parser_handle_el(yyextra, yytext, yyleng); return -1;
{IL}  runes_parser_handle_il(yyextra, yytext, yyleng); return -1;
{DCH} runes_parser_handle_dch(yyextra, yytext, yyleng); return -1;
{SM}  runes_parser_handle_sm(yyextra, yytext, yyleng); return -1;
{RM}  runes_parser_handle_rm(yyextra, yytext, yyleng); return -1;
{SGR} runes_parser_handle_sgr(yyextra, yytext, yyleng); return -1;
{CSR} runes_parser_handle_csr(yyextra, yytext, yyleng); return -1;

{DECSED} runes_parser_handle_decsed(yyextra, yytext, yyleng); return -1;
{DECSEL} runes_parser_handle_decsel(yyextra, yytext, yyleng); return -1;

{OSC0} runes_parser_handle_osc0(yyextra, yytext, yyleng); return -1;
{OSC1} runes_parser_handle_osc1(yyextra, yytext, yyleng); return -1;
{OSC2} runes_parser_handle_osc2(yyextra, yytext, yyleng); return -1;

{ASCII}+ runes_parser_handle_ascii(yyextra, yytext, yyleng); return -1;
{CHAR}+  runes_parser_handle_text(yyextra, yytext, yyleng); return -1;

{CSI}[<=?]?{CSIPARAMS}[0-9;] |
{CSI}                        |
{OSC}                        |
{ESC}                        return yyleng;

<<EOF>> return 0;

{CSI}[<=?]?{CSIPARAMS}{CTRL} {
    fprintf(
        stderr, "unhandled CSI sequence: \\033%*s\\%hho\n",
        (int)yyleng - 2, yytext + 1, yytext[yyleng - 1]);
    return -1;
}

{CSI}[<=?]?{CSIPARAMS}{CHAR} {
    fprintf(
        stderr, "unhandled CSI sequence: \\033%*s\n",
        (int)yyleng - 1, yytext + 1);
    return -1;
}

{OSC}{CTRL} {
    fprintf(
        stderr, "unhandled OSC sequence: \\033%*s\\%hho\n",
        (int)yyleng - 2, yytext + 1, yytext[yyleng - 1]);
    return -1;
}

{OSC}{CHAR} {
    fprintf(
        stderr, "unhandled OSC sequence: \\033%*s\n",
        (int)yyleng - 1, yytext + 1);
    return -1;
}

{ESC}{CTRL} {
    fprintf(stderr, "unhandled escape sequence: \\%hho\n", yytext[1]);
    return -1;
}

{ESC}{CHAR} {
    fprintf(
        stderr, "unhandled escape sequence: %*s\n",
        (int)yyleng - 1, yytext + 1);
    return -1;
}

{CTRL} {
    fprintf(stderr, "unhandled control character: \\%hho\n", yytext[0]);
    return -1;
}

(?s:.) {
    fprintf(stderr, "invalid utf8 byte: \\%hho\n", yytext[0]);
}

%%

void runes_parser_process_string(RunesTerm *t, char *buf, size_t len)
{
    YY_BUFFER_STATE state;
    yyscan_t scanner;
    int remaining;

    /* XXX this will break if buf ends with a partial escape sequence or utf8
     * character. we need to detect that and not consume the entire input in
     * that case */
    yylex_init_extra(t, &scanner);
    state = runes_parser_yy_scan_bytes(buf, len, scanner);
    while ((remaining = runes_parser_yylex(scanner)) == -1);
    t->remaininglen = remaining;
    if (t->remaininglen) {
        memmove(t->readbuf, &buf[len - t->remaininglen], t->remaininglen);
    }
    runes_parser_yy_delete_buffer(state, scanner);
    yylex_destroy(scanner);
}

static void runes_parser_handle_bel(RunesTerm *t)
{
    runes_window_backend_request_visual_bell(t);
}

static void runes_parser_handle_bs(RunesTerm *t)
{
    runes_display_move_to(t, t->row, t->col - 1);
}

static void runes_parser_handle_tab(RunesTerm *t)
{
    runes_display_move_to(t, t->row, t->col - (t->col % 8) + 8);
}

static void runes_parser_handle_lf(RunesTerm *t)
{
    runes_display_move_to(t, t->row + 1, t->col);
}

static void runes_parser_handle_cr(RunesTerm *t)
{
    runes_display_move_to(t, t->row, 0);
}

static void runes_parser_handle_deckpam(RunesTerm *t)
{
    t->application_keypad = 1;
}

static void runes_parser_handle_deckpnm(RunesTerm *t)
{
    t->application_keypad = 0;
}

static void runes_parser_handle_ri(RunesTerm *t)
{
    runes_display_move_to(t, t->row - 1, t->col);
}

static void runes_parser_handle_vb(RunesTerm *t)
{
    runes_window_backend_request_visual_bell(t);
}

static void runes_parser_handle_decsc(RunesTerm *t)
{
    runes_display_save_cursor(t);
}

static void runes_parser_handle_decrc(RunesTerm *t)
{
    runes_display_restore_cursor(t);
}

static void runes_parser_extract_csi_params(
    char *buf, size_t len, int *params, int *nparams)
{
    runes_parser_extract_sm_params(buf, len, NULL, params, nparams);
}

static void runes_parser_extract_sm_params(
    char *buf, size_t len, char *modes, int *params, int *nparams)
{
    char *pos = buf;

    /* this assumes that it will only ever be called on a fully matched CSI
     * sequence: accessing one character beyond the end is safe because CSI
     * sequences always have one character after the parameters (to determine
     * the type of sequence), and the parameters can only ever be digits,
     * separated by semicolons. */
    buf[len] = '\0';
    *nparams = 0;
    while ((size_t)(pos - buf) < len) {
        if (*nparams >= RUNES_PARSER_CSI_MAX_PARAMS) {
            fprintf(stderr, "max CSI parameter length exceeded\n");
            break;
        }

        if (modes && (size_t)(pos - buf) < len) {
            if (strstr(pos, "0123456789")) {
                modes[*nparams] = '\0';
            }
            else {
                modes[*nparams] = *pos++;
            }
        }

        params[(*nparams)++] = atoi(pos);

        pos = strchr(pos, ';');
        if (pos) {
            pos++;
        }
        else {
            break;
        }
    }
}

static void runes_parser_handle_cuu(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row - params[0], t->col);
}

static void runes_parser_handle_cud(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row + params[0], t->col);
}

static void runes_parser_handle_cuf(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row, t->col + params[0]);
}

static void runes_parser_handle_cub(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row, t->col - params[0]);
}

static void runes_parser_handle_cup(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0, 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    if (params[0] == 0) {
        params[0] = 1;
    }
    if (params[1] == 0) {
        params[1] = 1;
    }
    runes_display_move_to(t, params[0] - 1, params[1] - 1);
}

static void runes_parser_handle_ed(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    switch (params[0]) {
    case 0:
        runes_display_clear_screen_forward(t);
        break;
    case 1:
        /* XXX */
        fprintf(stderr, "unhandled ED parameter 1\n");
        break;
    case 2:
        runes_display_clear_screen(t);
        break;
    default:
        fprintf(stderr, "unknown ED parameter %d\n", params[0]);
        break;
    }
}

static void runes_parser_handle_el(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    switch (params[0]) {
    case 0:
        runes_display_kill_line_forward(t);
        break;
    case 1:
        /* XXX */
        fprintf(stderr, "unhandled EL parameter 1\n");
        break;
    case 2:
        /* XXX */
        fprintf(stderr, "unhandled EL parameter 2\n");
        break;
    default:
        fprintf(stderr, "unknown EL parameter %d\n", params[0]);
        break;
    }
}

static void runes_parser_handle_il(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_scroll_up(t, params[0]);
}

static void runes_parser_handle_dch(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_delete_characters(t, params[0]);
}

static void runes_parser_handle_sm(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS], nparams, i;
    char modes[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 };

    runes_parser_extract_sm_params(buf + 2, len - 3, modes, params, &nparams);
    for (i = 0; i < nparams; ++i) {
        switch (modes[i]) {
        case '?':
            switch (params[i]) {
            case 1:
                t->application_cursor = 1;
                break;
            case 25:
                runes_display_show_cursor(t);
                break;
            case 1049:
                runes_display_use_alternate_buffer(t);
                break;
            default:
                fprintf(
                    stderr, "unknown SM parameter: %c%d\n",
                    modes[i], params[i]);
                break;
            }
            break;
        default:
            fprintf(
                stderr, "unknown SM parameter: %c%d\n",
                modes[i], params[i]);
            break;
        }
    }
}

static void runes_parser_handle_rm(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS], nparams, i;
    char modes[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 };

    runes_parser_extract_sm_params(buf + 2, len - 3, modes, params, &nparams);
    for (i = 0; i < nparams; ++i) {
        switch (modes[i]) {
        case '?':
            switch (params[i]) {
            case 1:
                t->application_cursor = 0;
                break;
            case 25:
                runes_display_hide_cursor(t);
                break;
            case 1049:
                runes_display_use_normal_buffer(t);
                break;
            default:
                fprintf(
                    stderr, "unknown RM parameter: %c%d\n",
                    modes[i], params[i]);
                break;
            }
            break;
        default:
            fprintf(
                stderr, "unknown RM parameter: %c%d\n",
                modes[i], params[i]);
            break;
        }
    }
}

static void runes_parser_handle_sgr(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams, i;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    if (nparams < 1) {
        nparams = 1;
    }
    for (i = 0; i < nparams; ++i) {
        switch (params[i]) {
        case 0:
            runes_display_reset_text_attributes(t);
            break;
        case 1:
            runes_display_set_bold(t);
            break;
        case 3:
            runes_display_set_italic(t);
            break;
        case 4:
            runes_display_set_underline(t);
            break;
        case 7:
            runes_display_set_inverse(t);
            break;
        case 22:
            runes_display_reset_bold(t);
            break;
        case 23:
            runes_display_reset_italic(t);
            break;
        case 24:
            runes_display_reset_underline(t);
            break;
        case 27:
            runes_display_reset_inverse(t);
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            runes_display_set_fg_color(t, params[i] - 30);
            break;
        case 39:
            runes_display_reset_fg_color(t);
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            runes_display_set_bg_color(t, params[i] - 40);
            break;
        case 49:
            runes_display_reset_bg_color(t);
            break;
        default:
            fprintf(stderr, "unknown SGR parameter: %d\n", params[i]);
            break;
        }
    }
}

static void runes_parser_handle_csr(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1, t->rows, 1, t->cols };
    int nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);

    runes_display_set_scroll_region(
        t, params[0], params[1], params[2], params[3]);
}

static void runes_parser_handle_decsed(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    switch (params[0]) {
    case 0:
        /* XXX not quite correct */
        runes_display_clear_screen_forward(t);
        break;
    case 1:
        /* XXX */
        fprintf(stderr, "unhandled DECSED parameter 1\n");
        break;
    case 2:
        /* XXX not quite correct */
        runes_display_clear_screen(t);
        break;
    default:
        fprintf(stderr, "unknown DECSED parameter %d\n", params[0]);
        break;
    }
}

static void runes_parser_handle_decsel(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    switch (params[0]) {
    case 0:
        /* XXX not quite correct */
        runes_display_kill_line_forward(t);
        break;
    case 1:
        /* XXX */
        fprintf(stderr, "unhandled DECSEL parameter 1\n");
        break;
    case 2:
        /* XXX */
        fprintf(stderr, "unhandled DECSEL parameter 2\n");
        break;
    default:
        fprintf(stderr, "unknown DECSEL parameter %d\n", params[0]);
        break;
    }
}

static void runes_parser_handle_osc0(RunesTerm *t, char *buf, size_t len)
{
    runes_window_backend_set_icon_name(t, buf + 4, len - 5);
    runes_window_backend_set_window_title(t, buf + 4, len - 5);
}

static void runes_parser_handle_osc1(RunesTerm *t, char *buf, size_t len)
{
    runes_window_backend_set_icon_name(t, buf + 4, len - 5);
}

static void runes_parser_handle_osc2(RunesTerm *t, char *buf, size_t len)
{
    runes_window_backend_set_window_title(t, buf + 4, len - 5);
}

static void runes_parser_handle_ascii(RunesTerm *t, char *text, size_t len)
{
    runes_display_show_string_ascii(t, text, len);
}

static void runes_parser_handle_text(RunesTerm *t, char *text, size_t len)
{
    runes_display_show_string_utf8(t, text, len);
}

/* XXX these are copied from the generated file so that I can add the UNUSED
 * declarations, otherwise we get compilation errors */
void *runes_parser_yyalloc(yy_size_t size, yyscan_t yyscanner)
{
    UNUSED(yyscanner);
    return (void *)malloc(size);
}

void *runes_parser_yyrealloc(void *ptr, yy_size_t size, yyscan_t yyscanner)
{
    UNUSED(yyscanner);
    return (void *)realloc((char *)ptr, size);
}

void runes_parser_yyfree(void *ptr, yyscan_t yyscanner)
{
    UNUSED(yyscanner);
    free((char *) ptr);
}
