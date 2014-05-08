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
SI  \017
ESC \033

DECKPAM {ESC}=
DECKPNM {ESC}>
CSI     {ESC}\[
OSC     {ESC}\]
RI      {ESC}M
RIS     {ESC}c
VB      {ESC}g
DECSC   {ESC}7
DECRC   {ESC}8

DECCSI     {CSI}\?
CSIPARAM1  ([0-9]+)?
CSIPARAM2  ([0-9]+(;[0-9]+)?)?
CSIPARAM24 ([0-9]+(;[0-9]+){1,3})?
CSIPARAMS  ([0-9]+(;[0-9]+)*)?
SMPARAMS   ([<=?]?[0-9]+(;[<=?]?[0-9]+)*)?

ICH {CSI}{CSIPARAM1}@
CUU {CSI}{CSIPARAM1}A
CUD {CSI}{CSIPARAM1}B
CUF {CSI}{CSIPARAM1}C
CUB {CSI}{CSIPARAM1}D
CUP {CSI}{CSIPARAM2}H
ED  {CSI}{CSIPARAM1}J
EL  {CSI}{CSIPARAM1}K
IL  {CSI}{CSIPARAM1}L
DL  {CSI}{CSIPARAM1}M
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

GZD4 {ESC}\([\040-\057]*[\060-\176]
G1D4 {ESC}\)[\040-\057]*[\060-\176]
G2D4 {ESC}\*[\040-\057]*[\060-\176]
G3D4 {ESC}\+[\040-\057]*[\060-\176]

%{
static void runes_parser_handle_bel(RunesTerm *t);
static void runes_parser_handle_bs(RunesTerm *t);
static void runes_parser_handle_tab(RunesTerm *t);
static void runes_parser_handle_lf(RunesTerm *t);
static void runes_parser_handle_cr(RunesTerm *t);
static void runes_parser_handle_deckpam(RunesTerm *t);
static void runes_parser_handle_deckpnm(RunesTerm *t);
static void runes_parser_handle_ri(RunesTerm *t);
static void runes_parser_handle_ris(RunesTerm *t);
static void runes_parser_handle_vb(RunesTerm *t);
static void runes_parser_handle_decsc(RunesTerm *t);
static void runes_parser_handle_decrc(RunesTerm *t);
static void runes_parser_extract_csi_params(
    char *buf, size_t len, int *params, int *nparams);
static void runes_parser_extract_sm_params(
    char *buf, size_t len, char *modes, int *params, int *nparams);
static void runes_parser_handle_ich(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cuu(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cud(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cuf(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cub(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_cup(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_ed(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_el(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_il(RunesTerm *t, char *buf, size_t len);
static void runes_parser_handle_dl(RunesTerm *t, char *buf, size_t len);
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

{BEL}     runes_parser_handle_bel(yyextra);
{BS}      runes_parser_handle_bs(yyextra);
{TAB}     runes_parser_handle_tab(yyextra);
{LF}      |
{VT}      |
{FF}      runes_parser_handle_lf(yyextra);
{CR}      runes_parser_handle_cr(yyextra);
{SI}

{DECKPAM} runes_parser_handle_deckpam(yyextra);
{DECKPNM} runes_parser_handle_deckpnm(yyextra);
{RI}      runes_parser_handle_ri(yyextra);
{RIS}     runes_parser_handle_ris(yyextra);
{VB}      runes_parser_handle_vb(yyextra);
{DECSC}   runes_parser_handle_decsc(yyextra);
{DECRC}   runes_parser_handle_decrc(yyextra);

{ICH}     runes_parser_handle_ich(yyextra, yytext, yyleng);
{CUU}     runes_parser_handle_cuu(yyextra, yytext, yyleng);
{CUD}     runes_parser_handle_cud(yyextra, yytext, yyleng);
{CUF}     runes_parser_handle_cuf(yyextra, yytext, yyleng);
{CUB}     runes_parser_handle_cub(yyextra, yytext, yyleng);
{CUP}     runes_parser_handle_cup(yyextra, yytext, yyleng);
{ED}      runes_parser_handle_ed(yyextra, yytext, yyleng);
{EL}      runes_parser_handle_el(yyextra, yytext, yyleng);
{IL}      runes_parser_handle_il(yyextra, yytext, yyleng);
{DL}      runes_parser_handle_dl(yyextra, yytext, yyleng);
{DCH}     runes_parser_handle_dch(yyextra, yytext, yyleng);
{SM}      runes_parser_handle_sm(yyextra, yytext, yyleng);
{RM}      runes_parser_handle_rm(yyextra, yytext, yyleng);
{SGR}     runes_parser_handle_sgr(yyextra, yytext, yyleng);
{CSR}     runes_parser_handle_csr(yyextra, yytext, yyleng);

{DECSED}  runes_parser_handle_decsed(yyextra, yytext, yyleng);
{DECSEL}  runes_parser_handle_decsel(yyextra, yytext, yyleng);

{OSC0}    runes_parser_handle_osc0(yyextra, yytext, yyleng);
{OSC1}    runes_parser_handle_osc1(yyextra, yytext, yyleng);
{OSC2}    runes_parser_handle_osc2(yyextra, yytext, yyleng);

{GZD4}    |
{G1D4}    |
{G2D4}    |
{G3D4}

{ASCII}+  runes_parser_handle_ascii(yyextra, yytext, yyleng);
{CHAR}+   runes_parser_handle_text(yyextra, yytext, yyleng);

{LEAD2}                      |
{LEAD3}{CONT}?               |
{LEAD4}{CONT}?{CONT}?        |
{CSI}[<=?]?{CSIPARAMS}[0-9;] |
{CSI}                        |
{OSC}                        |
{ESC}                        return yyleng;

<<EOF>> return 0;

{CSI}[<=?]?{CSIPARAMS}{CTRL} {
    runes_warn(
        "unhandled CSI sequence: \\033%*s\\%hho\n",
        (int)yyleng - 2, yytext + 1, yytext[yyleng - 1]);
}

{CSI}[<=?]?{CSIPARAMS}{CHAR} {
    runes_warn(
        "unhandled CSI sequence: \\033%*s\n",
        (int)yyleng - 1, yytext + 1);
}

{OSC}{CTRL} {
    runes_warn(
        "unhandled OSC sequence: \\033%*s\\%hho\n",
        (int)yyleng - 2, yytext + 1, yytext[yyleng - 1]);
}

{OSC}{CHAR} {
    runes_warn(
        "unhandled OSC sequence: \\033%*s\n",
        (int)yyleng - 1, yytext + 1);
}

{ESC}{CTRL} {
    runes_warn("unhandled escape sequence: \\%hho\n", yytext[1]);
}

{ESC}{CHAR} {
    runes_warn(
        "unhandled escape sequence: %*s\n",
        (int)yyleng - 1, yytext + 1);
}

{CTRL} {
    runes_warn("unhandled control character: \\%hho\n", yytext[0]);
}

(?s:.) {
    runes_warn("invalid utf8 byte: \\%hho\n", yytext[0]);
}

%%

static void runes_parser_handle_bel(RunesTerm *t)
{
    if (t->config.audible_bell) {
        runes_screen_audible_bell(t);
    }
    else {
        runes_screen_visual_bell(t);
    }
}

static void runes_parser_handle_bs(RunesTerm *t)
{
    runes_screen_move_to(t, t->scr.grid->cur.row, t->scr.grid->cur.col - 1);
}

static void runes_parser_handle_tab(RunesTerm *t)
{
    runes_screen_move_to(
        t, t->scr.grid->cur.row,
        t->scr.grid->cur.col - (t->scr.grid->cur.col % 8) + 8);
}

static void runes_parser_handle_lf(RunesTerm *t)
{
    runes_screen_move_to(t, t->scr.grid->cur.row + 1, t->scr.grid->cur.col);
}

static void runes_parser_handle_cr(RunesTerm *t)
{
    runes_screen_move_to(t, t->scr.grid->cur.row, 0);
}

static void runes_parser_handle_deckpam(RunesTerm *t)
{
    runes_screen_set_application_keypad(t);
}

static void runes_parser_handle_deckpnm(RunesTerm *t)
{
    runes_screen_reset_application_keypad(t);
}

static void runes_parser_handle_ri(RunesTerm *t)
{
    runes_screen_move_to(t, t->scr.grid->cur.row - 1, t->scr.grid->cur.col);
}

static void runes_parser_handle_ris(RunesTerm *t)
{
    runes_screen_use_normal_buffer(t);
    runes_screen_set_scroll_region(
        t, 0, t->scr.grid->max.row - 1, 0, t->scr.grid->max.col - 1);
    runes_screen_clear_screen(t);
    runes_screen_save_cursor(t);
    runes_screen_reset_text_attributes(t);
    runes_screen_show_cursor(t);
    runes_screen_reset_application_keypad(t);
    runes_screen_reset_application_cursor(t);
    runes_screen_reset_mouse_reporting_press(t);
    runes_screen_reset_mouse_reporting_press_release(t);
}

static void runes_parser_handle_vb(RunesTerm *t)
{
    runes_screen_visual_bell(t);
}

static void runes_parser_handle_decsc(RunesTerm *t)
{
    runes_screen_save_cursor(t);
}

static void runes_parser_handle_decrc(RunesTerm *t)
{
    runes_screen_restore_cursor(t);
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
            runes_warn("max CSI parameter length exceeded\n");
            break;
        }

        if (modes && (size_t)(pos - buf) < len) {
            if (strspn(pos, "0123456789")) {
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

static void runes_parser_handle_ich(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_insert_characters(t, params[0]);
}

static void runes_parser_handle_cuu(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_move_to(
        t, t->scr.grid->cur.row - params[0], t->scr.grid->cur.col);
}

static void runes_parser_handle_cud(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_move_to(
        t, t->scr.grid->cur.row + params[0], t->scr.grid->cur.col);
}

static void runes_parser_handle_cuf(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_move_to(
        t, t->scr.grid->cur.row, t->scr.grid->cur.col + params[0]);
}

static void runes_parser_handle_cub(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_move_to(
        t, t->scr.grid->cur.row, t->scr.grid->cur.col - params[0]);
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
    runes_screen_move_to(t, params[0] - 1, params[1] - 1);
}

static void runes_parser_handle_ed(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    switch (params[0]) {
    case 0:
        runes_screen_clear_screen_forward(t);
        break;
    case 1:
        runes_screen_clear_screen_backward(t);
        break;
    case 2:
        runes_screen_clear_screen(t);
        break;
    default:
        runes_warn("unknown ED parameter %d\n", params[0]);
        break;
    }
}

static void runes_parser_handle_el(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    switch (params[0]) {
    case 0:
        runes_screen_kill_line_forward(t);
        break;
    case 1:
        runes_screen_kill_line_backward(t);
        break;
    case 2:
        runes_screen_kill_line(t);
        break;
    default:
        runes_warn("unknown EL parameter %d\n", params[0]);
        break;
    }
}

static void runes_parser_handle_il(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_insert_lines(t, params[0]);
    runes_screen_move_to(t, t->scr.grid->cur.row, 0);
}

static void runes_parser_handle_dl(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_delete_lines(t, params[0]);
    runes_screen_move_to(t, t->scr.grid->cur.row, 0);
}

static void runes_parser_handle_dch(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = { 1 }, nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);
    runes_screen_delete_characters(t, params[0]);
}

static void runes_parser_handle_sm(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS], nparams, i;
    char modes[RUNES_PARSER_CSI_MAX_PARAMS] = { 0 };

    runes_parser_extract_sm_params(buf + 2, len - 3, modes, params, &nparams);
    for (i = 0; i < nparams; ++i) {
        switch (modes[i]) {
        case 0:
            switch (params[i]) {
                case 34:
                    /* do nothing, no idea what this is even for */
                    break;
                default:
                    runes_warn("unknown SM parameter: %d\n", params[i]);
                    break;
            }
            break;
        case '?':
            switch (params[i]) {
            case 1:
                runes_screen_set_application_cursor(t);
                break;
            case 9:
                runes_screen_set_mouse_reporting_press(t);
                break;
            case 25:
                runes_screen_show_cursor(t);
                break;
            case 1000:
                runes_screen_set_mouse_reporting_press_release(t);
                break;
            case 47:
            case 1049:
                runes_screen_use_alternate_buffer(t);
                break;
            default:
                runes_warn(
                    "unknown SM parameter: %c%d\n", modes[i], params[i]);
                break;
            }
            break;
        default:
            runes_warn("unknown SM parameter: %c%d\n", modes[i], params[i]);
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
        case 0:
            switch (params[i]) {
                case 34:
                    /* do nothing, no idea what this is even for */
                    break;
                default:
                    runes_warn("unknown RM parameter: %d\n", params[i]);
                    break;
            }
            break;
        case '?':
            switch (params[i]) {
            case 1:
                runes_screen_set_application_cursor(t);
                break;
            case 9:
                runes_screen_set_mouse_reporting_press(t);
                break;
            case 25:
                runes_screen_hide_cursor(t);
                break;
            case 1000:
                runes_screen_set_mouse_reporting_press_release(t);
                break;
            case 47:
            case 1049:
                runes_screen_use_normal_buffer(t);
                break;
            default:
                runes_warn(
                    "unknown RM parameter: %c%d\n", modes[i], params[i]);
                break;
            }
            break;
        default:
            runes_warn("unknown RM parameter: %c%d\n", modes[i], params[i]);
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
            runes_screen_reset_text_attributes(t);
            break;
        case 1:
            runes_screen_set_bold(t);
            break;
        case 3:
            runes_screen_set_italic(t);
            break;
        case 4:
            runes_screen_set_underline(t);
            break;
        case 7:
            runes_screen_set_inverse(t);
            break;
        case 22:
            runes_screen_reset_bold(t);
            break;
        case 23:
            runes_screen_reset_italic(t);
            break;
        case 24:
            runes_screen_reset_underline(t);
            break;
        case 27:
            runes_screen_reset_inverse(t);
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            runes_screen_set_fg_color(t, params[i] - 30);
            break;
        case 38: {
            i++;
            if (i >= nparams) {
                runes_warn(
                    "unknown SGR parameter: %d (too few parameters)\n",
                    params[i - 1]);
                break;
            }

            switch (params[i]) {
            case 2:
                i += 3;
                if (i >= nparams) {
                    runes_warn(
                        "unknown SGR parameter: %d;%d (too few parameters)\n",
                        params[i - 4], params[i - 3]);
                    break;
                }
                runes_screen_set_fg_color_rgb(
                    t, params[i - 2], params[i - 1], params[i]);
                break;
            case 5:
                i++;
                if (i >= nparams) {
                    runes_warn(
                        "unknown SGR parameter: %d;%d (too few parameters)\n",
                        params[i - 2], params[i - 1]);
                    break;
                }
                runes_screen_set_fg_color(t, params[i]);
                break;
            default:
                i++;
                runes_warn(
                    "unknown SGR parameter: %d;%d\n",
                    params[i - 2], params[i - 1]);
                break;
            }
            break;
        }
        case 39:
            runes_screen_reset_fg_color(t);
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            runes_screen_set_bg_color(t, params[i] - 40);
            break;
        case 48: {
            i++;
            if (i >= nparams) {
                runes_warn(
                    "unknown SGR parameter: %d (too few parameters)\n",
                    params[i - 1]);
                break;
            }

            switch (params[i]) {
            case 2:
                i += 3;
                if (i >= nparams) {
                    runes_warn(
                        "unknown SGR parameter: %d;%d (too few parameters)\n",
                        params[i - 4], params[i - 3]);
                    break;
                }
                runes_screen_set_bg_color_rgb(
                    t, params[i - 2], params[i - 1], params[i]);
                break;
            case 5:
                i++;
                if (i >= nparams) {
                    runes_warn(
                        "unknown SGR parameter: %d;%d (too few parameters)\n",
                        params[i - 2], params[i - 1]);
                    break;
                }
                runes_screen_set_bg_color(t, params[i]);
                break;
            default:
                i++;
                runes_warn(
                    "unknown SGR parameter: %d;%d\n",
                    params[i - 2], params[i - 1]);
                break;
            }
            break;
        }
        case 49:
            runes_screen_reset_bg_color(t);
            break;
        case 90: case 91: case 92: case 93:
        case 94: case 95: case 96: case 97:
            runes_screen_set_fg_color(t, params[i] - 82);
            break;
        case 100: case 101: case 102: case 103:
        case 104: case 105: case 106: case 107:
            runes_screen_set_bg_color(t, params[i] - 92);
            break;
        default:
            runes_warn("unknown SGR parameter: %d\n", params[i]);
            break;
        }
    }
}

static void runes_parser_handle_csr(RunesTerm *t, char *buf, size_t len)
{
    int params[RUNES_PARSER_CSI_MAX_PARAMS] = {
        1, t->scr.grid->max.row, 1, t->scr.grid->max.col };
    int nparams;

    runes_parser_extract_csi_params(buf + 2, len - 3, params, &nparams);

    runes_screen_set_scroll_region(
        t, params[0] - 1, params[1] - 1, params[2] - 1, params[3] - 1);
}

static void runes_parser_handle_decsed(RunesTerm *t, char *buf, size_t len)
{
    /* XXX not quite correct, but i don't think programs really use anything
     * that would show a difference */
    runes_parser_handle_ed(t, buf, len);
}

static void runes_parser_handle_decsel(RunesTerm *t, char *buf, size_t len)
{
    /* XXX not quite correct, but i don't think programs really use anything
     * that would show a difference */
    runes_parser_handle_el(t, buf, len);
}

static void runes_parser_handle_osc0(RunesTerm *t, char *buf, size_t len)
{
    runes_screen_set_icon_name(t, buf + 4, len - 5);
    runes_screen_set_window_title(t, buf + 4, len - 5);
}

static void runes_parser_handle_osc1(RunesTerm *t, char *buf, size_t len)
{
    runes_screen_set_icon_name(t, buf + 4, len - 5);
}

static void runes_parser_handle_osc2(RunesTerm *t, char *buf, size_t len)
{
    runes_screen_set_window_title(t, buf + 4, len - 5);
}

static void runes_parser_handle_ascii(RunesTerm *t, char *text, size_t len)
{
    runes_screen_show_string_ascii(t, text, len);
}

static void runes_parser_handle_text(RunesTerm *t, char *text, size_t len)
{
    runes_screen_show_string_utf8(t, text, len);
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
