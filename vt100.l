%{
#include <string.h>

#include "runes.h"

#define RUNES_VT100_CSI_MAX_PARAMS 256
%}

%option reentrant noyywrap nodefault batch
%option prefix="runes_vt100_yy"
%option extra-type="RunesTerm *"

CTRL [\000-\037\177]
TEXT [^\000-\037\177]

ST  \007
BEL \007
BS  \010
TAB \011
LF  \012
VT  \013
FF  \014
CR  \015
ESC \033

CSI {ESC}\[
OSC {ESC}\]
RI  {ESC}M

DECCSI    {CSI}\?
CSIPARAM1 ([0-9]+)?
CSIPARAM2 ([0-9]+(;[0-9]+)?)?
CSIPARAMS ([0-9]+(;[0-9]+)*)?
SMPARAMS  ([<=?]?[0-9]+(;[<=?]?[0-9]+)*)?

CUU {CSI}{CSIPARAM1}A
CUD {CSI}{CSIPARAM1}B
CUF {CSI}{CSIPARAM1}C
CUB {CSI}{CSIPARAM1}D
CUP {CSI}{CSIPARAM2}H
ED  {CSI}{CSIPARAM1}J
EL  {CSI}{CSIPARAM1}K
SM  {CSI}{SMPARAMS}h
RM  {CSI}{SMPARAMS}l
SGR {CSI}{CSIPARAMS}m

DECSED {DECCSI}{CSIPARAM1}J
DECSEL {DECCSI}{CSIPARAM1}K

OSC0 {OSC}0;{TEXT}*{ST}
OSC1 {OSC}1;{TEXT}*{ST}
OSC2 {OSC}2;{TEXT}*{ST}

%{
static void runes_vt100_handle_text(RunesTerm *t, char *text, size_t len);
static void runes_vt100_handle_bel(RunesTerm *t);
static void runes_vt100_handle_bs(RunesTerm *t);
static void runes_vt100_handle_tab(RunesTerm *t);
static void runes_vt100_handle_lf(RunesTerm *t);
static void runes_vt100_handle_cr(RunesTerm *t);
static void runes_vt100_handle_ri(RunesTerm *t);
static void runes_vt100_extract_csi_params(
    char *buf, size_t len, int *params, int *nparams);
static void runes_vt100_extract_sm_params(
    char *buf, size_t len, char *modes, int *params, int *nparams);
static void runes_vt100_handle_cuu(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_cud(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_cuf(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_cub(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_cup(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_ed(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_el(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_sm(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_rm(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_sgr(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_decsed(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_decsel(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_osc0(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_osc1(RunesTerm *t, char *buf, size_t len);
static void runes_vt100_handle_osc2(RunesTerm *t, char *buf, size_t len);
%}

%%

{TEXT}+ runes_vt100_handle_text(yyextra, yytext, yyleng);

{BEL} runes_vt100_handle_bel(yyextra);
{BS}  runes_vt100_handle_bs(yyextra);
{TAB} runes_vt100_handle_tab(yyextra);
{LF}  |
{VT}  |
{FF}  runes_vt100_handle_lf(yyextra);
{CR}  runes_vt100_handle_cr(yyextra);

{RI} runes_vt100_handle_ri(yyextra);

{CUU} runes_vt100_handle_cuu(yyextra, yytext, yyleng);
{CUD} runes_vt100_handle_cud(yyextra, yytext, yyleng);
{CUF} runes_vt100_handle_cuf(yyextra, yytext, yyleng);
{CUB} runes_vt100_handle_cub(yyextra, yytext, yyleng);
{CUP} runes_vt100_handle_cup(yyextra, yytext, yyleng);
{ED}  runes_vt100_handle_ed(yyextra, yytext, yyleng);
{EL}  runes_vt100_handle_el(yyextra, yytext, yyleng);
{SM}  runes_vt100_handle_sm(yyextra, yytext, yyleng);
{RM}  runes_vt100_handle_rm(yyextra, yytext, yyleng);
{SGR} runes_vt100_handle_sgr(yyextra, yytext, yyleng);

{DECSED} runes_vt100_handle_decsed(yyextra, yytext, yyleng);
{DECSEL} runes_vt100_handle_decsel(yyextra, yytext, yyleng);

{OSC0} runes_vt100_handle_osc0(yyextra, yytext, yyleng);
{OSC1} runes_vt100_handle_osc1(yyextra, yytext, yyleng);
{OSC2} runes_vt100_handle_osc2(yyextra, yytext, yyleng);

{CSI}[<=?]?{CSIPARAMS}(?s:.) {
    fprintf(
        stderr, "unhandled CSI sequence: \\033%*s\n",
        (int)yyleng - 1, yytext + 1);
}

{OSC}(?s:.) {
    fprintf(
        stderr, "unhandled OSC sequence: \\033%*s\n",
        (int)yyleng - 1, yytext + 1);
}

{ESC}{CTRL} {
    fprintf(stderr, "unhandled escape sequence: \\%hho\n", yytext[1]);
    yyless(1);
}

{ESC}{TEXT} {
    fprintf(stderr, "unhandled escape sequence: %c\n", yytext[1]);
    yyless(1);
}

{CTRL} {
    fprintf(stderr, "unhandled control character: \\%hho\n", yytext[0]);
}

%%

void runes_vt100_process_string(RunesTerm *t, char *buf, size_t len)
{
    YY_BUFFER_STATE state;
    yyscan_t scanner;

    yylex_init_extra(t, &scanner);
    state = runes_vt100_yy_scan_bytes(buf, len, scanner);
    runes_vt100_yylex(scanner);
    runes_vt100_yy_delete_buffer(state, scanner);
    yylex_destroy(scanner);
}

static void runes_vt100_handle_text(RunesTerm *t, char *text, size_t len)
{
    runes_display_show_string(t, text, len);
}

static void runes_vt100_handle_bel(RunesTerm *t)
{
    /* XXX */
}

static void runes_vt100_handle_bs(RunesTerm *t)
{
    runes_display_move_to(t, t->row, t->col - 1);
}

static void runes_vt100_handle_tab(RunesTerm *t)
{
    runes_display_move_to(t, t->row, t->col - (t->col % 8) + 8);
}

static void runes_vt100_handle_lf(RunesTerm *t)
{
    runes_display_move_to(t, t->row + 1, t->col);
}

static void runes_vt100_handle_cr(RunesTerm *t)
{
    runes_display_move_to(t, t->row, 0);
}

static void runes_vt100_handle_ri(RunesTerm *t)
{
    runes_display_move_to(t, t->row - 1, t->col);
}

static void runes_vt100_extract_csi_params(
    char *buf, size_t len, int *params, int *nparams)
{
    runes_vt100_extract_sm_params(buf, len, NULL, params, nparams);
}

static void runes_vt100_extract_sm_params(
    char *buf, size_t len, char *modes, int *params, int *nparams)
{
    char tmp = buf[len];
    char *pos = buf;

    /* this assumes that it will only ever be called on a fully matched CSI
     * sequence: accessing one character beyond the end is safe because CSI
     * sequences always have one character after the parameters (to determine
     * the type of sequence), and the parameters can only ever be digits,
     * separated by semicolons. */
    buf[len] = '\0';
    *nparams = 0;
    while ((size_t)(pos - buf) < len) {
        if (*nparams >= RUNES_VT100_CSI_MAX_PARAMS) {
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

static void runes_vt100_handle_cuu(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row - params[0], t->col);
}

static void runes_vt100_handle_cud(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row + params[0], t->col);
}

static void runes_vt100_handle_cuf(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row, t->col + params[0]);
}

static void runes_vt100_handle_cub(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_display_move_to(t, t->row, t->col - params[0]);
}

static void runes_vt100_handle_cup(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 0, 0 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
    if (params[0] == 0) {
        params[0] = 1;
    }
    if (params[1] == 0) {
        params[1] = 1;
    }
    runes_display_move_to(t, params[0] - 1, params[1] - 1);
}

static void runes_vt100_handle_ed(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
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

static void runes_vt100_handle_el(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
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

static void runes_vt100_handle_sm(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS], nparams, i;
    char modes[RUNES_VT100_CSI_MAX_PARAMS] = { 0 };

    runes_vt100_extract_sm_params(buf + 2, len - 3, modes, params, &nparams);
    for (i = 0; i < nparams; ++i) {
        switch (modes[i]) {
        case '?':
            switch (params[i]) {
            case 25:
                runes_display_show_cursor(t);
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

static void runes_vt100_handle_rm(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS], nparams, i;
    char modes[RUNES_VT100_CSI_MAX_PARAMS] = { 0 };

    runes_vt100_extract_sm_params(buf + 2, len - 3, modes, params, &nparams);
    for (i = 0; i < nparams; ++i) {
        switch (modes[i]) {
        case '?':
            switch (params[i]) {
            case 25:
                runes_display_hide_cursor(t);
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

static void runes_vt100_handle_sgr(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 0 }, nparams, i;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
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
        case 22:
            runes_display_reset_bold(t);
            break;
        case 23:
            runes_display_reset_italic(t);
            break;
        case 24:
            runes_display_reset_underline(t);
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            runes_display_set_fg_color(t, t->colors[params[i] - 30]);
            break;
        case 39:
            runes_display_reset_fg_color(t);
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            runes_display_set_bg_color(t, t->colors[params[i] - 40]);
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

static void runes_vt100_handle_decsed(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
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

static void runes_vt100_handle_decsel(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_VT100_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_vt100_extract_csi_params(buf + 2, len - 3, params, &nparams);
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

static void runes_vt100_handle_osc0(RunesTerm *t, char *buf, size_t len)
{
    runes_window_backend_set_icon_name(t, buf + 4, len - 5);
    runes_window_backend_set_window_title(t, buf + 4, len - 5);
}

static void runes_vt100_handle_osc1(RunesTerm *t, char *buf, size_t len)
{
    runes_window_backend_set_icon_name(t, buf + 4, len - 5);
}

static void runes_vt100_handle_osc2(RunesTerm *t, char *buf, size_t len)
{
    runes_window_backend_set_window_title(t, buf + 4, len - 5);
}
