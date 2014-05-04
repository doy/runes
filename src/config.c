#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runes.h"

static void runes_config_set_defaults(RunesTerm *t);
static FILE *runes_config_get_config_file();
static void runes_config_process_config_file(RunesTerm *t, FILE *config_file);
static void runes_config_process_args(RunesTerm *t, int argc, char *argv[]);
static void runes_config_set(RunesTerm *t, char *key, char *value);
static char runes_config_parse_bool(char *val);
static int runes_config_parse_uint(char *val);
static char *runes_config_parse_string(char *val);
static cairo_pattern_t *runes_config_parse_color(char *val);

void runes_config_init(RunesTerm *t, int argc, char *argv[])
{
    runes_config_set_defaults(t);
    runes_config_process_config_file(t, runes_config_get_config_file());
    runes_config_process_args(t, argc, argv);
}

void runes_config_cleanup(RunesTerm *t)
{
    int i;

    free(t->config.font_name);
    cairo_pattern_destroy(t->config.cursorcolor);
    cairo_pattern_destroy(t->config.mousecursorcolor);
    cairo_pattern_destroy(t->config.fgdefault);
    cairo_pattern_destroy(t->config.bgdefault);
    for (i = 0; i < 256; ++i) {
        cairo_pattern_destroy(t->config.colors[i]);
    }
}

static void runes_config_set_defaults(RunesTerm *t)
{
    RunesConfig *config = &t->config;

    config->font_name      = strdup("monospace 10");
    config->bold_is_bright = 1;
    config->bold_is_bold   = 1;
    config->audible_bell   = 1;
    config->bell_is_urgent = 1;

    config->cursorcolor      = cairo_pattern_create_rgb(0.0, 1.0, 0.0);
    config->mousecursorcolor = cairo_pattern_create_rgb(1.0, 1.0, 1.0);

    config->fgdefault = cairo_pattern_create_rgb(0.827, 0.827, 0.827);
    config->bgdefault = cairo_pattern_create_rgb(0.0,   0.0,   0.0);

    config->colors[0]   = cairo_pattern_create_rgb(0.000, 0.000, 0.000);
    config->colors[1]   = cairo_pattern_create_rgb(0.804, 0.000, 0.000);
    config->colors[2]   = cairo_pattern_create_rgb(0.000, 0.804, 0.000);
    config->colors[3]   = cairo_pattern_create_rgb(0.804, 0.804, 0.000);
    config->colors[4]   = cairo_pattern_create_rgb(0.000, 0.000, 0.804);
    config->colors[5]   = cairo_pattern_create_rgb(0.804, 0.000, 0.804);
    config->colors[6]   = cairo_pattern_create_rgb(0.000, 0.804, 0.804);
    config->colors[7]   = cairo_pattern_create_rgb(0.898, 0.898, 0.898);
    config->colors[8]   = cairo_pattern_create_rgb(0.302, 0.302, 0.302);
    config->colors[9]   = cairo_pattern_create_rgb(1.000, 0.000, 0.000);
    config->colors[10]  = cairo_pattern_create_rgb(0.000, 1.000, 0.000);
    config->colors[11]  = cairo_pattern_create_rgb(1.000, 1.000, 0.000);
    config->colors[12]  = cairo_pattern_create_rgb(0.000, 0.000, 1.000);
    config->colors[13]  = cairo_pattern_create_rgb(1.000, 0.000, 1.000);
    config->colors[14]  = cairo_pattern_create_rgb(0.000, 1.000, 1.000);
    config->colors[15]  = cairo_pattern_create_rgb(1.000, 1.000, 1.000);
    config->colors[16]  = cairo_pattern_create_rgb(0.000, 0.000, 0.000);
    config->colors[17]  = cairo_pattern_create_rgb(0.000, 0.000, 0.373);
    config->colors[18]  = cairo_pattern_create_rgb(0.000, 0.000, 0.529);
    config->colors[19]  = cairo_pattern_create_rgb(0.000, 0.000, 0.686);
    config->colors[20]  = cairo_pattern_create_rgb(0.000, 0.000, 0.843);
    config->colors[21]  = cairo_pattern_create_rgb(0.000, 0.000, 1.000);
    config->colors[22]  = cairo_pattern_create_rgb(0.000, 0.373, 0.000);
    config->colors[23]  = cairo_pattern_create_rgb(0.000, 0.373, 0.373);
    config->colors[24]  = cairo_pattern_create_rgb(0.000, 0.373, 0.529);
    config->colors[25]  = cairo_pattern_create_rgb(0.000, 0.373, 0.686);
    config->colors[26]  = cairo_pattern_create_rgb(0.000, 0.373, 0.843);
    config->colors[27]  = cairo_pattern_create_rgb(0.000, 0.373, 1.000);
    config->colors[28]  = cairo_pattern_create_rgb(0.000, 0.529, 0.000);
    config->colors[29]  = cairo_pattern_create_rgb(0.000, 0.529, 0.373);
    config->colors[30]  = cairo_pattern_create_rgb(0.000, 0.529, 0.529);
    config->colors[31]  = cairo_pattern_create_rgb(0.000, 0.529, 0.686);
    config->colors[32]  = cairo_pattern_create_rgb(0.000, 0.529, 0.843);
    config->colors[33]  = cairo_pattern_create_rgb(0.000, 0.529, 1.000);
    config->colors[34]  = cairo_pattern_create_rgb(0.000, 0.686, 0.000);
    config->colors[35]  = cairo_pattern_create_rgb(0.000, 0.686, 0.373);
    config->colors[36]  = cairo_pattern_create_rgb(0.000, 0.686, 0.529);
    config->colors[37]  = cairo_pattern_create_rgb(0.000, 0.686, 0.686);
    config->colors[38]  = cairo_pattern_create_rgb(0.000, 0.686, 0.843);
    config->colors[39]  = cairo_pattern_create_rgb(0.000, 0.686, 1.000);
    config->colors[40]  = cairo_pattern_create_rgb(0.000, 0.843, 0.000);
    config->colors[41]  = cairo_pattern_create_rgb(0.000, 0.843, 0.373);
    config->colors[42]  = cairo_pattern_create_rgb(0.000, 0.843, 0.529);
    config->colors[43]  = cairo_pattern_create_rgb(0.000, 0.843, 0.686);
    config->colors[44]  = cairo_pattern_create_rgb(0.000, 0.843, 0.843);
    config->colors[45]  = cairo_pattern_create_rgb(0.000, 0.843, 1.000);
    config->colors[46]  = cairo_pattern_create_rgb(0.000, 1.000, 0.000);
    config->colors[47]  = cairo_pattern_create_rgb(0.000, 1.000, 0.373);
    config->colors[48]  = cairo_pattern_create_rgb(0.000, 1.000, 0.529);
    config->colors[49]  = cairo_pattern_create_rgb(0.000, 1.000, 0.686);
    config->colors[50]  = cairo_pattern_create_rgb(0.000, 1.000, 0.843);
    config->colors[51]  = cairo_pattern_create_rgb(0.000, 1.000, 1.000);
    config->colors[52]  = cairo_pattern_create_rgb(0.373, 0.000, 0.000);
    config->colors[53]  = cairo_pattern_create_rgb(0.373, 0.000, 0.373);
    config->colors[54]  = cairo_pattern_create_rgb(0.373, 0.000, 0.529);
    config->colors[55]  = cairo_pattern_create_rgb(0.373, 0.000, 0.686);
    config->colors[56]  = cairo_pattern_create_rgb(0.373, 0.000, 0.843);
    config->colors[57]  = cairo_pattern_create_rgb(0.373, 0.000, 1.000);
    config->colors[58]  = cairo_pattern_create_rgb(0.373, 0.373, 0.000);
    config->colors[59]  = cairo_pattern_create_rgb(0.373, 0.373, 0.373);
    config->colors[60]  = cairo_pattern_create_rgb(0.373, 0.373, 0.529);
    config->colors[61]  = cairo_pattern_create_rgb(0.373, 0.373, 0.686);
    config->colors[62]  = cairo_pattern_create_rgb(0.373, 0.373, 0.843);
    config->colors[63]  = cairo_pattern_create_rgb(0.373, 0.373, 1.000);
    config->colors[64]  = cairo_pattern_create_rgb(0.373, 0.529, 0.000);
    config->colors[65]  = cairo_pattern_create_rgb(0.373, 0.529, 0.373);
    config->colors[66]  = cairo_pattern_create_rgb(0.373, 0.529, 0.529);
    config->colors[67]  = cairo_pattern_create_rgb(0.373, 0.529, 0.686);
    config->colors[68]  = cairo_pattern_create_rgb(0.373, 0.529, 0.843);
    config->colors[69]  = cairo_pattern_create_rgb(0.373, 0.529, 1.000);
    config->colors[70]  = cairo_pattern_create_rgb(0.373, 0.686, 0.000);
    config->colors[71]  = cairo_pattern_create_rgb(0.373, 0.686, 0.373);
    config->colors[72]  = cairo_pattern_create_rgb(0.373, 0.686, 0.529);
    config->colors[73]  = cairo_pattern_create_rgb(0.373, 0.686, 0.686);
    config->colors[74]  = cairo_pattern_create_rgb(0.373, 0.686, 0.843);
    config->colors[75]  = cairo_pattern_create_rgb(0.373, 0.686, 1.000);
    config->colors[76]  = cairo_pattern_create_rgb(0.373, 0.843, 0.000);
    config->colors[77]  = cairo_pattern_create_rgb(0.373, 0.843, 0.373);
    config->colors[78]  = cairo_pattern_create_rgb(0.373, 0.843, 0.529);
    config->colors[79]  = cairo_pattern_create_rgb(0.373, 0.843, 0.686);
    config->colors[80]  = cairo_pattern_create_rgb(0.373, 0.843, 0.843);
    config->colors[81]  = cairo_pattern_create_rgb(0.373, 0.843, 1.000);
    config->colors[82]  = cairo_pattern_create_rgb(0.373, 1.000, 0.000);
    config->colors[83]  = cairo_pattern_create_rgb(0.373, 1.000, 0.373);
    config->colors[84]  = cairo_pattern_create_rgb(0.373, 1.000, 0.529);
    config->colors[85]  = cairo_pattern_create_rgb(0.373, 1.000, 0.686);
    config->colors[86]  = cairo_pattern_create_rgb(0.373, 1.000, 0.843);
    config->colors[87]  = cairo_pattern_create_rgb(0.373, 1.000, 1.000);
    config->colors[88]  = cairo_pattern_create_rgb(0.529, 0.000, 0.000);
    config->colors[89]  = cairo_pattern_create_rgb(0.529, 0.000, 0.373);
    config->colors[90]  = cairo_pattern_create_rgb(0.529, 0.000, 0.529);
    config->colors[91]  = cairo_pattern_create_rgb(0.529, 0.000, 0.686);
    config->colors[92]  = cairo_pattern_create_rgb(0.529, 0.000, 0.843);
    config->colors[93]  = cairo_pattern_create_rgb(0.529, 0.000, 1.000);
    config->colors[94]  = cairo_pattern_create_rgb(0.529, 0.373, 0.000);
    config->colors[95]  = cairo_pattern_create_rgb(0.529, 0.373, 0.373);
    config->colors[96]  = cairo_pattern_create_rgb(0.529, 0.373, 0.529);
    config->colors[97]  = cairo_pattern_create_rgb(0.529, 0.373, 0.686);
    config->colors[98]  = cairo_pattern_create_rgb(0.529, 0.373, 0.843);
    config->colors[99]  = cairo_pattern_create_rgb(0.529, 0.373, 1.000);
    config->colors[100] = cairo_pattern_create_rgb(0.529, 0.529, 0.000);
    config->colors[101] = cairo_pattern_create_rgb(0.529, 0.529, 0.373);
    config->colors[102] = cairo_pattern_create_rgb(0.529, 0.529, 0.529);
    config->colors[103] = cairo_pattern_create_rgb(0.529, 0.529, 0.686);
    config->colors[104] = cairo_pattern_create_rgb(0.529, 0.529, 0.843);
    config->colors[105] = cairo_pattern_create_rgb(0.529, 0.529, 1.000);
    config->colors[106] = cairo_pattern_create_rgb(0.529, 0.686, 0.000);
    config->colors[107] = cairo_pattern_create_rgb(0.529, 0.686, 0.373);
    config->colors[108] = cairo_pattern_create_rgb(0.529, 0.686, 0.529);
    config->colors[109] = cairo_pattern_create_rgb(0.529, 0.686, 0.686);
    config->colors[110] = cairo_pattern_create_rgb(0.529, 0.686, 0.843);
    config->colors[111] = cairo_pattern_create_rgb(0.529, 0.686, 1.000);
    config->colors[112] = cairo_pattern_create_rgb(0.529, 0.843, 0.000);
    config->colors[113] = cairo_pattern_create_rgb(0.529, 0.843, 0.373);
    config->colors[114] = cairo_pattern_create_rgb(0.529, 0.843, 0.529);
    config->colors[115] = cairo_pattern_create_rgb(0.529, 0.843, 0.686);
    config->colors[116] = cairo_pattern_create_rgb(0.529, 0.843, 0.843);
    config->colors[117] = cairo_pattern_create_rgb(0.529, 0.843, 1.000);
    config->colors[118] = cairo_pattern_create_rgb(0.529, 1.000, 0.000);
    config->colors[119] = cairo_pattern_create_rgb(0.529, 1.000, 0.373);
    config->colors[120] = cairo_pattern_create_rgb(0.529, 1.000, 0.529);
    config->colors[121] = cairo_pattern_create_rgb(0.529, 1.000, 0.686);
    config->colors[122] = cairo_pattern_create_rgb(0.529, 1.000, 0.843);
    config->colors[123] = cairo_pattern_create_rgb(0.529, 1.000, 1.000);
    config->colors[124] = cairo_pattern_create_rgb(0.686, 0.000, 0.000);
    config->colors[125] = cairo_pattern_create_rgb(0.686, 0.000, 0.373);
    config->colors[126] = cairo_pattern_create_rgb(0.686, 0.000, 0.529);
    config->colors[127] = cairo_pattern_create_rgb(0.686, 0.000, 0.686);
    config->colors[128] = cairo_pattern_create_rgb(0.686, 0.000, 0.843);
    config->colors[129] = cairo_pattern_create_rgb(0.686, 0.000, 1.000);
    config->colors[130] = cairo_pattern_create_rgb(0.686, 0.373, 0.000);
    config->colors[131] = cairo_pattern_create_rgb(0.686, 0.373, 0.373);
    config->colors[132] = cairo_pattern_create_rgb(0.686, 0.373, 0.529);
    config->colors[133] = cairo_pattern_create_rgb(0.686, 0.373, 0.686);
    config->colors[134] = cairo_pattern_create_rgb(0.686, 0.373, 0.843);
    config->colors[135] = cairo_pattern_create_rgb(0.686, 0.373, 1.000);
    config->colors[136] = cairo_pattern_create_rgb(0.686, 0.529, 0.000);
    config->colors[137] = cairo_pattern_create_rgb(0.686, 0.529, 0.373);
    config->colors[138] = cairo_pattern_create_rgb(0.686, 0.529, 0.529);
    config->colors[139] = cairo_pattern_create_rgb(0.686, 0.529, 0.686);
    config->colors[140] = cairo_pattern_create_rgb(0.686, 0.529, 0.843);
    config->colors[141] = cairo_pattern_create_rgb(0.686, 0.529, 1.000);
    config->colors[142] = cairo_pattern_create_rgb(0.686, 0.686, 0.000);
    config->colors[143] = cairo_pattern_create_rgb(0.686, 0.686, 0.373);
    config->colors[144] = cairo_pattern_create_rgb(0.686, 0.686, 0.529);
    config->colors[145] = cairo_pattern_create_rgb(0.686, 0.686, 0.686);
    config->colors[146] = cairo_pattern_create_rgb(0.686, 0.686, 0.843);
    config->colors[147] = cairo_pattern_create_rgb(0.686, 0.686, 1.000);
    config->colors[148] = cairo_pattern_create_rgb(0.686, 0.843, 0.000);
    config->colors[149] = cairo_pattern_create_rgb(0.686, 0.843, 0.373);
    config->colors[150] = cairo_pattern_create_rgb(0.686, 0.843, 0.529);
    config->colors[151] = cairo_pattern_create_rgb(0.686, 0.843, 0.686);
    config->colors[152] = cairo_pattern_create_rgb(0.686, 0.843, 0.843);
    config->colors[153] = cairo_pattern_create_rgb(0.686, 0.843, 1.000);
    config->colors[154] = cairo_pattern_create_rgb(0.686, 1.000, 0.000);
    config->colors[155] = cairo_pattern_create_rgb(0.686, 1.000, 0.373);
    config->colors[156] = cairo_pattern_create_rgb(0.686, 1.000, 0.529);
    config->colors[157] = cairo_pattern_create_rgb(0.686, 1.000, 0.686);
    config->colors[158] = cairo_pattern_create_rgb(0.686, 1.000, 0.843);
    config->colors[159] = cairo_pattern_create_rgb(0.686, 1.000, 1.000);
    config->colors[160] = cairo_pattern_create_rgb(0.843, 0.000, 0.000);
    config->colors[161] = cairo_pattern_create_rgb(0.843, 0.000, 0.373);
    config->colors[162] = cairo_pattern_create_rgb(0.843, 0.000, 0.529);
    config->colors[163] = cairo_pattern_create_rgb(0.843, 0.000, 0.686);
    config->colors[164] = cairo_pattern_create_rgb(0.843, 0.000, 0.843);
    config->colors[165] = cairo_pattern_create_rgb(0.843, 0.000, 1.000);
    config->colors[166] = cairo_pattern_create_rgb(0.843, 0.373, 0.000);
    config->colors[167] = cairo_pattern_create_rgb(0.843, 0.373, 0.373);
    config->colors[168] = cairo_pattern_create_rgb(0.843, 0.373, 0.529);
    config->colors[169] = cairo_pattern_create_rgb(0.843, 0.373, 0.686);
    config->colors[170] = cairo_pattern_create_rgb(0.843, 0.373, 0.843);
    config->colors[171] = cairo_pattern_create_rgb(0.843, 0.373, 1.000);
    config->colors[172] = cairo_pattern_create_rgb(0.843, 0.529, 0.000);
    config->colors[173] = cairo_pattern_create_rgb(0.843, 0.529, 0.373);
    config->colors[174] = cairo_pattern_create_rgb(0.843, 0.529, 0.529);
    config->colors[175] = cairo_pattern_create_rgb(0.843, 0.529, 0.686);
    config->colors[176] = cairo_pattern_create_rgb(0.843, 0.529, 0.843);
    config->colors[177] = cairo_pattern_create_rgb(0.843, 0.529, 1.000);
    config->colors[178] = cairo_pattern_create_rgb(0.843, 0.686, 0.000);
    config->colors[179] = cairo_pattern_create_rgb(0.843, 0.686, 0.373);
    config->colors[180] = cairo_pattern_create_rgb(0.843, 0.686, 0.529);
    config->colors[181] = cairo_pattern_create_rgb(0.843, 0.686, 0.686);
    config->colors[182] = cairo_pattern_create_rgb(0.843, 0.686, 0.843);
    config->colors[183] = cairo_pattern_create_rgb(0.843, 0.686, 1.000);
    config->colors[184] = cairo_pattern_create_rgb(0.843, 0.843, 0.000);
    config->colors[185] = cairo_pattern_create_rgb(0.843, 0.843, 0.373);
    config->colors[186] = cairo_pattern_create_rgb(0.843, 0.843, 0.529);
    config->colors[187] = cairo_pattern_create_rgb(0.843, 0.843, 0.686);
    config->colors[188] = cairo_pattern_create_rgb(0.843, 0.843, 0.843);
    config->colors[189] = cairo_pattern_create_rgb(0.843, 0.843, 1.000);
    config->colors[190] = cairo_pattern_create_rgb(0.843, 1.000, 0.000);
    config->colors[191] = cairo_pattern_create_rgb(0.843, 1.000, 0.373);
    config->colors[192] = cairo_pattern_create_rgb(0.843, 1.000, 0.529);
    config->colors[193] = cairo_pattern_create_rgb(0.843, 1.000, 0.686);
    config->colors[194] = cairo_pattern_create_rgb(0.843, 1.000, 0.843);
    config->colors[195] = cairo_pattern_create_rgb(0.843, 1.000, 1.000);
    config->colors[196] = cairo_pattern_create_rgb(1.000, 0.000, 0.000);
    config->colors[197] = cairo_pattern_create_rgb(1.000, 0.000, 0.373);
    config->colors[198] = cairo_pattern_create_rgb(1.000, 0.000, 0.529);
    config->colors[199] = cairo_pattern_create_rgb(1.000, 0.000, 0.686);
    config->colors[200] = cairo_pattern_create_rgb(1.000, 0.000, 0.843);
    config->colors[201] = cairo_pattern_create_rgb(1.000, 0.000, 1.000);
    config->colors[202] = cairo_pattern_create_rgb(1.000, 0.373, 0.000);
    config->colors[203] = cairo_pattern_create_rgb(1.000, 0.373, 0.373);
    config->colors[204] = cairo_pattern_create_rgb(1.000, 0.373, 0.529);
    config->colors[205] = cairo_pattern_create_rgb(1.000, 0.373, 0.686);
    config->colors[206] = cairo_pattern_create_rgb(1.000, 0.373, 0.843);
    config->colors[207] = cairo_pattern_create_rgb(1.000, 0.373, 1.000);
    config->colors[208] = cairo_pattern_create_rgb(1.000, 0.529, 0.000);
    config->colors[209] = cairo_pattern_create_rgb(1.000, 0.529, 0.373);
    config->colors[210] = cairo_pattern_create_rgb(1.000, 0.529, 0.529);
    config->colors[211] = cairo_pattern_create_rgb(1.000, 0.529, 0.686);
    config->colors[212] = cairo_pattern_create_rgb(1.000, 0.529, 0.843);
    config->colors[213] = cairo_pattern_create_rgb(1.000, 0.529, 1.000);
    config->colors[214] = cairo_pattern_create_rgb(1.000, 0.686, 0.000);
    config->colors[215] = cairo_pattern_create_rgb(1.000, 0.686, 0.373);
    config->colors[216] = cairo_pattern_create_rgb(1.000, 0.686, 0.529);
    config->colors[217] = cairo_pattern_create_rgb(1.000, 0.686, 0.686);
    config->colors[218] = cairo_pattern_create_rgb(1.000, 0.686, 0.843);
    config->colors[219] = cairo_pattern_create_rgb(1.000, 0.686, 1.000);
    config->colors[220] = cairo_pattern_create_rgb(1.000, 0.843, 0.000);
    config->colors[221] = cairo_pattern_create_rgb(1.000, 0.843, 0.373);
    config->colors[222] = cairo_pattern_create_rgb(1.000, 0.843, 0.529);
    config->colors[223] = cairo_pattern_create_rgb(1.000, 0.843, 0.686);
    config->colors[224] = cairo_pattern_create_rgb(1.000, 0.843, 0.843);
    config->colors[225] = cairo_pattern_create_rgb(1.000, 0.843, 1.000);
    config->colors[226] = cairo_pattern_create_rgb(1.000, 1.000, 0.000);
    config->colors[227] = cairo_pattern_create_rgb(1.000, 1.000, 0.373);
    config->colors[228] = cairo_pattern_create_rgb(1.000, 1.000, 0.529);
    config->colors[229] = cairo_pattern_create_rgb(1.000, 1.000, 0.686);
    config->colors[230] = cairo_pattern_create_rgb(1.000, 1.000, 0.843);
    config->colors[231] = cairo_pattern_create_rgb(1.000, 1.000, 1.000);
    config->colors[232] = cairo_pattern_create_rgb(0.031, 0.031, 0.031);
    config->colors[233] = cairo_pattern_create_rgb(0.071, 0.071, 0.071);
    config->colors[234] = cairo_pattern_create_rgb(0.110, 0.110, 0.110);
    config->colors[235] = cairo_pattern_create_rgb(0.149, 0.149, 0.149);
    config->colors[236] = cairo_pattern_create_rgb(0.188, 0.188, 0.188);
    config->colors[237] = cairo_pattern_create_rgb(0.227, 0.227, 0.227);
    config->colors[238] = cairo_pattern_create_rgb(0.267, 0.267, 0.267);
    config->colors[239] = cairo_pattern_create_rgb(0.306, 0.306, 0.306);
    config->colors[240] = cairo_pattern_create_rgb(0.345, 0.345, 0.345);
    config->colors[241] = cairo_pattern_create_rgb(0.376, 0.376, 0.376);
    config->colors[242] = cairo_pattern_create_rgb(0.400, 0.400, 0.400);
    config->colors[243] = cairo_pattern_create_rgb(0.463, 0.463, 0.463);
    config->colors[244] = cairo_pattern_create_rgb(0.502, 0.502, 0.502);
    config->colors[245] = cairo_pattern_create_rgb(0.541, 0.541, 0.541);
    config->colors[246] = cairo_pattern_create_rgb(0.580, 0.580, 0.580);
    config->colors[247] = cairo_pattern_create_rgb(0.620, 0.620, 0.620);
    config->colors[248] = cairo_pattern_create_rgb(0.659, 0.659, 0.659);
    config->colors[249] = cairo_pattern_create_rgb(0.698, 0.698, 0.698);
    config->colors[250] = cairo_pattern_create_rgb(0.737, 0.737, 0.737);
    config->colors[251] = cairo_pattern_create_rgb(0.776, 0.776, 0.776);
    config->colors[252] = cairo_pattern_create_rgb(0.816, 0.816, 0.816);
    config->colors[253] = cairo_pattern_create_rgb(0.855, 0.855, 0.855);
    config->colors[254] = cairo_pattern_create_rgb(0.894, 0.894, 0.894);
    config->colors[255] = cairo_pattern_create_rgb(0.933, 0.933, 0.933);

    config->default_rows = 24;
    config->default_cols = 80;

    config->scroll_lines = 3;
}

static FILE *runes_config_get_config_file()
{
    char *home, *config_dir, *path;
    size_t home_len, config_dir_len;
    FILE *file;

    home = getenv("HOME");
    home_len = strlen(home);

    config_dir = getenv("XDG_CONFIG_HOME");
    if (config_dir) {
        config_dir = strdup(config_dir);
    }
    else {
        config_dir = malloc(home_len + sizeof("/.config") + 1);
        strcpy(config_dir, home);
        strcpy(config_dir + home_len, "/.config");
    }
    config_dir_len = strlen(config_dir);

    path = malloc(config_dir_len + sizeof("/runes/runes.conf") + 1);
    strcpy(path, config_dir);
    strcpy(path + config_dir_len, "/runes/runes.conf");
    free(config_dir);

    if ((file = fopen(path, "r"))) {
        free(path);
        return file;
    }

    free(path);
    path = malloc(home_len + sizeof("/.runesrc") + 1);
    strcpy(path, home);
    strcpy(path + home_len, "/.runesrc");

    if ((file = fopen(path, "r"))) {
        free(path);
        return file;
    }

    free(path);

    if ((file = fopen("/etc/runesrc", "r"))) {
        return file;
    }

    return NULL;
}

static void runes_config_process_config_file(RunesTerm *t, FILE *config_file)
{
    char line[1024];

    if (!config_file) {
        return;
    }

    while (fgets(line, 1024, config_file)) {
        char *kbegin, *kend, *vbegin, *vend;
        size_t len;

        len = strlen(line);
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        if (!len || line[strspn(line, " \t")] == '#') {
            continue;
        }

        kbegin = line + strspn(line, " \t");
        kend = kbegin + strcspn(kbegin, " \t=");
        vbegin = kend + strspn(kend, " \t");
        if (*vbegin != '=') {
            fprintf(stderr, "couldn't parse line: '%s'\n", line);
        }
        vbegin++;
        vbegin = vbegin + strspn(vbegin, " \t");
        vend = line + len;

        *kend = '\0';
        *vend = '\0';

        runes_config_set(t, kbegin, vbegin);
    }
}

static void runes_config_process_args(RunesTerm *t, int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "--", 2)) {
            if (i + 1 < argc) {
                runes_config_set(t, &argv[i][2], argv[i + 1]);
                i++;
            }
            else {
                fprintf(
                    stderr, "option found with no argument: '%s'\n", argv[i]);
            }
        }
        else {
            fprintf(stderr, "unknown argument: '%s'\n", argv[i]);
        }
    }
}

static void runes_config_set(RunesTerm *t, char *key, char *val)
{
    RunesConfig *config = &t->config;

    if (!strcmp(key, "font")) {
        free(config->font_name);
        config->font_name = runes_config_parse_string(val);
    }
    else if (!strcmp(key, "bold_is_bright")) {
        config->bold_is_bright = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bold_is_bold")) {
        config->bold_is_bold = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "audible_bell")) {
        config->audible_bell = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bell_is_urgent")) {
        config->bell_is_urgent = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bgcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(config->bgdefault);
            config->bgdefault = newcolor;
        }
    }
    else if (!strcmp(key, "fgcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(config->fgdefault);
            config->fgdefault = newcolor;
        }
    }
    else if (!strcmp(key, "cursorcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(config->cursorcolor);
            config->cursorcolor = newcolor;
        }
    }
    else if (!strcmp(key, "mousecursorcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(config->mousecursorcolor);
            config->mousecursorcolor = newcolor;
        }
    }
    else if (!strncmp(key, "color", 5)) {
        cairo_pattern_t *newcolor;
        int i;

        i = atoi(&key[5]);
        if (key[5] < '0' || key[5] > '9' || i < 0 || i > 255) {
            fprintf(stderr, "unknown option: '%s'\n", key);
            return;
        }
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(config->colors[i]);
            config->colors[i] = newcolor;
        }
    }
    else if (!strcmp(key, "rows")) {
        config->default_rows = runes_config_parse_uint(val);
    }
    else if (!strcmp(key, "cols")) {
        config->default_cols = runes_config_parse_uint(val);
    }
    else if (!strcmp(key, "scroll_lines")) {
        config->scroll_lines = runes_config_parse_uint(val);
    }
    else if (!strcmp(key, "command")) {
        config->cmd = runes_config_parse_string(val);
    }
    else {
        fprintf(stderr, "unknown option: '%s'\n", key);
    }
}

static char runes_config_parse_bool(char *val)
{
    if (!strcmp(val, "true")) {
        return 1;
    }
    else if (!strcmp(val, "false")) {
        return 0;
    }
    else {
        fprintf(stderr, "unknown boolean value: '%s'\n", val);
        return 0;
    }
}

static int runes_config_parse_uint(char *val)
{
    if (strspn(val, "0123456789") != strlen(val)) {
        fprintf(stderr, "unknown unsigned integer value: '%s'\n", val);
    }

    return atoi(val);
}

static char *runes_config_parse_string(char *val)
{
    return strdup(val);
}

static cairo_pattern_t *runes_config_parse_color(char *val)
{
    int r, g, b;

    if (strlen(val) != 7 || sscanf(val, "#%2x%2x%2x", &r, &g, &b) != 3) {
        fprintf(stderr, "unknown color value: '%s'\n", val);
        return NULL;
    }

    return cairo_pattern_create_rgb(
        (double)r / 255.0, (double)g / 255.0, (double)b / 255.0);
}
