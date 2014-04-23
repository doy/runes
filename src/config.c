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

static void runes_config_set_defaults(RunesTerm *t)
{
    memset((void *)t, 0, sizeof(*t));

    t->font_name      = strdup("monospace 10");
    t->bold_is_bright = 1;
    t->bold_is_bold   = 1;
    t->audible_bell   = 1;
    t->bell_is_urgent = 1;

    t->mousecursorcolor = cairo_pattern_create_rgb(1.0, 1.0, 1.0);

    t->fgdefault = cairo_pattern_create_rgb(0.827, 0.827, 0.827);
    t->bgdefault = cairo_pattern_create_rgb(0.0,   0.0,   0.0);

    t->colors[0]   = cairo_pattern_create_rgb(0.000, 0.000, 0.000);
    t->colors[1]   = cairo_pattern_create_rgb(0.804, 0.000, 0.000);
    t->colors[2]   = cairo_pattern_create_rgb(0.000, 0.804, 0.000);
    t->colors[3]   = cairo_pattern_create_rgb(0.804, 0.804, 0.000);
    t->colors[4]   = cairo_pattern_create_rgb(0.000, 0.000, 0.804);
    t->colors[5]   = cairo_pattern_create_rgb(0.804, 0.000, 0.804);
    t->colors[6]   = cairo_pattern_create_rgb(0.000, 0.804, 0.804);
    t->colors[7]   = cairo_pattern_create_rgb(0.898, 0.898, 0.898);
    t->colors[8]   = cairo_pattern_create_rgb(0.302, 0.302, 0.302);
    t->colors[9]   = cairo_pattern_create_rgb(1.000, 0.000, 0.000);
    t->colors[10]  = cairo_pattern_create_rgb(0.000, 1.000, 0.000);
    t->colors[11]  = cairo_pattern_create_rgb(1.000, 1.000, 0.000);
    t->colors[12]  = cairo_pattern_create_rgb(0.000, 0.000, 1.000);
    t->colors[13]  = cairo_pattern_create_rgb(1.000, 0.000, 1.000);
    t->colors[14]  = cairo_pattern_create_rgb(0.000, 1.000, 1.000);
    t->colors[15]  = cairo_pattern_create_rgb(1.000, 1.000, 1.000);
    t->colors[16]  = cairo_pattern_create_rgb(0.000, 0.000, 0.000);
    t->colors[17]  = cairo_pattern_create_rgb(0.000, 0.000, 0.373);
    t->colors[18]  = cairo_pattern_create_rgb(0.000, 0.000, 0.529);
    t->colors[19]  = cairo_pattern_create_rgb(0.000, 0.000, 0.686);
    t->colors[20]  = cairo_pattern_create_rgb(0.000, 0.000, 0.843);
    t->colors[21]  = cairo_pattern_create_rgb(0.000, 0.000, 1.000);
    t->colors[22]  = cairo_pattern_create_rgb(0.000, 0.373, 0.000);
    t->colors[23]  = cairo_pattern_create_rgb(0.000, 0.373, 0.373);
    t->colors[24]  = cairo_pattern_create_rgb(0.000, 0.373, 0.529);
    t->colors[25]  = cairo_pattern_create_rgb(0.000, 0.373, 0.686);
    t->colors[26]  = cairo_pattern_create_rgb(0.000, 0.373, 0.843);
    t->colors[27]  = cairo_pattern_create_rgb(0.000, 0.373, 1.000);
    t->colors[28]  = cairo_pattern_create_rgb(0.000, 0.529, 0.000);
    t->colors[29]  = cairo_pattern_create_rgb(0.000, 0.529, 0.373);
    t->colors[30]  = cairo_pattern_create_rgb(0.000, 0.529, 0.529);
    t->colors[31]  = cairo_pattern_create_rgb(0.000, 0.529, 0.686);
    t->colors[32]  = cairo_pattern_create_rgb(0.000, 0.529, 0.843);
    t->colors[33]  = cairo_pattern_create_rgb(0.000, 0.529, 1.000);
    t->colors[34]  = cairo_pattern_create_rgb(0.000, 0.686, 0.000);
    t->colors[35]  = cairo_pattern_create_rgb(0.000, 0.686, 0.373);
    t->colors[36]  = cairo_pattern_create_rgb(0.000, 0.686, 0.529);
    t->colors[37]  = cairo_pattern_create_rgb(0.000, 0.686, 0.686);
    t->colors[38]  = cairo_pattern_create_rgb(0.000, 0.686, 0.843);
    t->colors[39]  = cairo_pattern_create_rgb(0.000, 0.686, 1.000);
    t->colors[40]  = cairo_pattern_create_rgb(0.000, 0.843, 0.000);
    t->colors[41]  = cairo_pattern_create_rgb(0.000, 0.843, 0.373);
    t->colors[42]  = cairo_pattern_create_rgb(0.000, 0.843, 0.529);
    t->colors[43]  = cairo_pattern_create_rgb(0.000, 0.843, 0.686);
    t->colors[44]  = cairo_pattern_create_rgb(0.000, 0.843, 0.843);
    t->colors[45]  = cairo_pattern_create_rgb(0.000, 0.843, 1.000);
    t->colors[46]  = cairo_pattern_create_rgb(0.000, 1.000, 0.000);
    t->colors[47]  = cairo_pattern_create_rgb(0.000, 1.000, 0.373);
    t->colors[48]  = cairo_pattern_create_rgb(0.000, 1.000, 0.529);
    t->colors[49]  = cairo_pattern_create_rgb(0.000, 1.000, 0.686);
    t->colors[50]  = cairo_pattern_create_rgb(0.000, 1.000, 0.843);
    t->colors[51]  = cairo_pattern_create_rgb(0.000, 1.000, 1.000);
    t->colors[52]  = cairo_pattern_create_rgb(0.373, 0.000, 0.000);
    t->colors[53]  = cairo_pattern_create_rgb(0.373, 0.000, 0.373);
    t->colors[54]  = cairo_pattern_create_rgb(0.373, 0.000, 0.529);
    t->colors[55]  = cairo_pattern_create_rgb(0.373, 0.000, 0.686);
    t->colors[56]  = cairo_pattern_create_rgb(0.373, 0.000, 0.843);
    t->colors[57]  = cairo_pattern_create_rgb(0.373, 0.000, 1.000);
    t->colors[58]  = cairo_pattern_create_rgb(0.373, 0.373, 0.000);
    t->colors[59]  = cairo_pattern_create_rgb(0.373, 0.373, 0.373);
    t->colors[60]  = cairo_pattern_create_rgb(0.373, 0.373, 0.529);
    t->colors[61]  = cairo_pattern_create_rgb(0.373, 0.373, 0.686);
    t->colors[62]  = cairo_pattern_create_rgb(0.373, 0.373, 0.843);
    t->colors[63]  = cairo_pattern_create_rgb(0.373, 0.373, 1.000);
    t->colors[64]  = cairo_pattern_create_rgb(0.373, 0.529, 0.000);
    t->colors[65]  = cairo_pattern_create_rgb(0.373, 0.529, 0.373);
    t->colors[66]  = cairo_pattern_create_rgb(0.373, 0.529, 0.529);
    t->colors[67]  = cairo_pattern_create_rgb(0.373, 0.529, 0.686);
    t->colors[68]  = cairo_pattern_create_rgb(0.373, 0.529, 0.843);
    t->colors[69]  = cairo_pattern_create_rgb(0.373, 0.529, 1.000);
    t->colors[70]  = cairo_pattern_create_rgb(0.373, 0.686, 0.000);
    t->colors[71]  = cairo_pattern_create_rgb(0.373, 0.686, 0.373);
    t->colors[72]  = cairo_pattern_create_rgb(0.373, 0.686, 0.529);
    t->colors[73]  = cairo_pattern_create_rgb(0.373, 0.686, 0.686);
    t->colors[74]  = cairo_pattern_create_rgb(0.373, 0.686, 0.843);
    t->colors[75]  = cairo_pattern_create_rgb(0.373, 0.686, 1.000);
    t->colors[76]  = cairo_pattern_create_rgb(0.373, 0.843, 0.000);
    t->colors[77]  = cairo_pattern_create_rgb(0.373, 0.843, 0.373);
    t->colors[78]  = cairo_pattern_create_rgb(0.373, 0.843, 0.529);
    t->colors[79]  = cairo_pattern_create_rgb(0.373, 0.843, 0.686);
    t->colors[80]  = cairo_pattern_create_rgb(0.373, 0.843, 0.843);
    t->colors[81]  = cairo_pattern_create_rgb(0.373, 0.843, 1.000);
    t->colors[82]  = cairo_pattern_create_rgb(0.373, 1.000, 0.000);
    t->colors[83]  = cairo_pattern_create_rgb(0.373, 1.000, 0.373);
    t->colors[84]  = cairo_pattern_create_rgb(0.373, 1.000, 0.529);
    t->colors[85]  = cairo_pattern_create_rgb(0.373, 1.000, 0.686);
    t->colors[86]  = cairo_pattern_create_rgb(0.373, 1.000, 0.843);
    t->colors[87]  = cairo_pattern_create_rgb(0.373, 1.000, 1.000);
    t->colors[88]  = cairo_pattern_create_rgb(0.529, 0.000, 0.000);
    t->colors[89]  = cairo_pattern_create_rgb(0.529, 0.000, 0.373);
    t->colors[90]  = cairo_pattern_create_rgb(0.529, 0.000, 0.529);
    t->colors[91]  = cairo_pattern_create_rgb(0.529, 0.000, 0.686);
    t->colors[92]  = cairo_pattern_create_rgb(0.529, 0.000, 0.843);
    t->colors[93]  = cairo_pattern_create_rgb(0.529, 0.000, 1.000);
    t->colors[94]  = cairo_pattern_create_rgb(0.529, 0.373, 0.000);
    t->colors[95]  = cairo_pattern_create_rgb(0.529, 0.373, 0.373);
    t->colors[96]  = cairo_pattern_create_rgb(0.529, 0.373, 0.529);
    t->colors[97]  = cairo_pattern_create_rgb(0.529, 0.373, 0.686);
    t->colors[98]  = cairo_pattern_create_rgb(0.529, 0.373, 0.843);
    t->colors[99]  = cairo_pattern_create_rgb(0.529, 0.373, 1.000);
    t->colors[100] = cairo_pattern_create_rgb(0.529, 0.529, 0.000);
    t->colors[101] = cairo_pattern_create_rgb(0.529, 0.529, 0.373);
    t->colors[102] = cairo_pattern_create_rgb(0.529, 0.529, 0.529);
    t->colors[103] = cairo_pattern_create_rgb(0.529, 0.529, 0.686);
    t->colors[104] = cairo_pattern_create_rgb(0.529, 0.529, 0.843);
    t->colors[105] = cairo_pattern_create_rgb(0.529, 0.529, 1.000);
    t->colors[106] = cairo_pattern_create_rgb(0.529, 0.686, 0.000);
    t->colors[107] = cairo_pattern_create_rgb(0.529, 0.686, 0.373);
    t->colors[108] = cairo_pattern_create_rgb(0.529, 0.686, 0.529);
    t->colors[109] = cairo_pattern_create_rgb(0.529, 0.686, 0.686);
    t->colors[110] = cairo_pattern_create_rgb(0.529, 0.686, 0.843);
    t->colors[111] = cairo_pattern_create_rgb(0.529, 0.686, 1.000);
    t->colors[112] = cairo_pattern_create_rgb(0.529, 0.843, 0.000);
    t->colors[113] = cairo_pattern_create_rgb(0.529, 0.843, 0.373);
    t->colors[114] = cairo_pattern_create_rgb(0.529, 0.843, 0.529);
    t->colors[115] = cairo_pattern_create_rgb(0.529, 0.843, 0.686);
    t->colors[116] = cairo_pattern_create_rgb(0.529, 0.843, 0.843);
    t->colors[117] = cairo_pattern_create_rgb(0.529, 0.843, 1.000);
    t->colors[118] = cairo_pattern_create_rgb(0.529, 1.000, 0.000);
    t->colors[119] = cairo_pattern_create_rgb(0.529, 1.000, 0.373);
    t->colors[120] = cairo_pattern_create_rgb(0.529, 1.000, 0.529);
    t->colors[121] = cairo_pattern_create_rgb(0.529, 1.000, 0.686);
    t->colors[122] = cairo_pattern_create_rgb(0.529, 1.000, 0.843);
    t->colors[123] = cairo_pattern_create_rgb(0.529, 1.000, 1.000);
    t->colors[124] = cairo_pattern_create_rgb(0.686, 0.000, 0.000);
    t->colors[125] = cairo_pattern_create_rgb(0.686, 0.000, 0.373);
    t->colors[126] = cairo_pattern_create_rgb(0.686, 0.000, 0.529);
    t->colors[127] = cairo_pattern_create_rgb(0.686, 0.000, 0.686);
    t->colors[128] = cairo_pattern_create_rgb(0.686, 0.000, 0.843);
    t->colors[129] = cairo_pattern_create_rgb(0.686, 0.000, 1.000);
    t->colors[130] = cairo_pattern_create_rgb(0.686, 0.373, 0.000);
    t->colors[131] = cairo_pattern_create_rgb(0.686, 0.373, 0.373);
    t->colors[132] = cairo_pattern_create_rgb(0.686, 0.373, 0.529);
    t->colors[133] = cairo_pattern_create_rgb(0.686, 0.373, 0.686);
    t->colors[134] = cairo_pattern_create_rgb(0.686, 0.373, 0.843);
    t->colors[135] = cairo_pattern_create_rgb(0.686, 0.373, 1.000);
    t->colors[136] = cairo_pattern_create_rgb(0.686, 0.529, 0.000);
    t->colors[137] = cairo_pattern_create_rgb(0.686, 0.529, 0.373);
    t->colors[138] = cairo_pattern_create_rgb(0.686, 0.529, 0.529);
    t->colors[139] = cairo_pattern_create_rgb(0.686, 0.529, 0.686);
    t->colors[140] = cairo_pattern_create_rgb(0.686, 0.529, 0.843);
    t->colors[141] = cairo_pattern_create_rgb(0.686, 0.529, 1.000);
    t->colors[142] = cairo_pattern_create_rgb(0.686, 0.686, 0.000);
    t->colors[143] = cairo_pattern_create_rgb(0.686, 0.686, 0.373);
    t->colors[144] = cairo_pattern_create_rgb(0.686, 0.686, 0.529);
    t->colors[145] = cairo_pattern_create_rgb(0.686, 0.686, 0.686);
    t->colors[146] = cairo_pattern_create_rgb(0.686, 0.686, 0.843);
    t->colors[147] = cairo_pattern_create_rgb(0.686, 0.686, 1.000);
    t->colors[148] = cairo_pattern_create_rgb(0.686, 0.843, 0.000);
    t->colors[149] = cairo_pattern_create_rgb(0.686, 0.843, 0.373);
    t->colors[150] = cairo_pattern_create_rgb(0.686, 0.843, 0.529);
    t->colors[151] = cairo_pattern_create_rgb(0.686, 0.843, 0.686);
    t->colors[152] = cairo_pattern_create_rgb(0.686, 0.843, 0.843);
    t->colors[153] = cairo_pattern_create_rgb(0.686, 0.843, 1.000);
    t->colors[154] = cairo_pattern_create_rgb(0.686, 1.000, 0.000);
    t->colors[155] = cairo_pattern_create_rgb(0.686, 1.000, 0.373);
    t->colors[156] = cairo_pattern_create_rgb(0.686, 1.000, 0.529);
    t->colors[157] = cairo_pattern_create_rgb(0.686, 1.000, 0.686);
    t->colors[158] = cairo_pattern_create_rgb(0.686, 1.000, 0.843);
    t->colors[159] = cairo_pattern_create_rgb(0.686, 1.000, 1.000);
    t->colors[160] = cairo_pattern_create_rgb(0.843, 0.000, 0.000);
    t->colors[161] = cairo_pattern_create_rgb(0.843, 0.000, 0.373);
    t->colors[162] = cairo_pattern_create_rgb(0.843, 0.000, 0.529);
    t->colors[163] = cairo_pattern_create_rgb(0.843, 0.000, 0.686);
    t->colors[164] = cairo_pattern_create_rgb(0.843, 0.000, 0.843);
    t->colors[165] = cairo_pattern_create_rgb(0.843, 0.000, 1.000);
    t->colors[166] = cairo_pattern_create_rgb(0.843, 0.373, 0.000);
    t->colors[167] = cairo_pattern_create_rgb(0.843, 0.373, 0.373);
    t->colors[168] = cairo_pattern_create_rgb(0.843, 0.373, 0.529);
    t->colors[169] = cairo_pattern_create_rgb(0.843, 0.373, 0.686);
    t->colors[170] = cairo_pattern_create_rgb(0.843, 0.373, 0.843);
    t->colors[171] = cairo_pattern_create_rgb(0.843, 0.373, 1.000);
    t->colors[172] = cairo_pattern_create_rgb(0.843, 0.529, 0.000);
    t->colors[173] = cairo_pattern_create_rgb(0.843, 0.529, 0.373);
    t->colors[174] = cairo_pattern_create_rgb(0.843, 0.529, 0.529);
    t->colors[175] = cairo_pattern_create_rgb(0.843, 0.529, 0.686);
    t->colors[176] = cairo_pattern_create_rgb(0.843, 0.529, 0.843);
    t->colors[177] = cairo_pattern_create_rgb(0.843, 0.529, 1.000);
    t->colors[178] = cairo_pattern_create_rgb(0.843, 0.686, 0.000);
    t->colors[179] = cairo_pattern_create_rgb(0.843, 0.686, 0.373);
    t->colors[180] = cairo_pattern_create_rgb(0.843, 0.686, 0.529);
    t->colors[181] = cairo_pattern_create_rgb(0.843, 0.686, 0.686);
    t->colors[182] = cairo_pattern_create_rgb(0.843, 0.686, 0.843);
    t->colors[183] = cairo_pattern_create_rgb(0.843, 0.686, 1.000);
    t->colors[184] = cairo_pattern_create_rgb(0.843, 0.843, 0.000);
    t->colors[185] = cairo_pattern_create_rgb(0.843, 0.843, 0.373);
    t->colors[186] = cairo_pattern_create_rgb(0.843, 0.843, 0.529);
    t->colors[187] = cairo_pattern_create_rgb(0.843, 0.843, 0.686);
    t->colors[188] = cairo_pattern_create_rgb(0.843, 0.843, 0.843);
    t->colors[189] = cairo_pattern_create_rgb(0.843, 0.843, 1.000);
    t->colors[190] = cairo_pattern_create_rgb(0.843, 1.000, 0.000);
    t->colors[191] = cairo_pattern_create_rgb(0.843, 1.000, 0.373);
    t->colors[192] = cairo_pattern_create_rgb(0.843, 1.000, 0.529);
    t->colors[193] = cairo_pattern_create_rgb(0.843, 1.000, 0.686);
    t->colors[194] = cairo_pattern_create_rgb(0.843, 1.000, 0.843);
    t->colors[195] = cairo_pattern_create_rgb(0.843, 1.000, 1.000);
    t->colors[196] = cairo_pattern_create_rgb(1.000, 0.000, 0.000);
    t->colors[197] = cairo_pattern_create_rgb(1.000, 0.000, 0.373);
    t->colors[198] = cairo_pattern_create_rgb(1.000, 0.000, 0.529);
    t->colors[199] = cairo_pattern_create_rgb(1.000, 0.000, 0.686);
    t->colors[200] = cairo_pattern_create_rgb(1.000, 0.000, 0.843);
    t->colors[201] = cairo_pattern_create_rgb(1.000, 0.000, 1.000);
    t->colors[202] = cairo_pattern_create_rgb(1.000, 0.373, 0.000);
    t->colors[203] = cairo_pattern_create_rgb(1.000, 0.373, 0.373);
    t->colors[204] = cairo_pattern_create_rgb(1.000, 0.373, 0.529);
    t->colors[205] = cairo_pattern_create_rgb(1.000, 0.373, 0.686);
    t->colors[206] = cairo_pattern_create_rgb(1.000, 0.373, 0.843);
    t->colors[207] = cairo_pattern_create_rgb(1.000, 0.373, 1.000);
    t->colors[208] = cairo_pattern_create_rgb(1.000, 0.529, 0.000);
    t->colors[209] = cairo_pattern_create_rgb(1.000, 0.529, 0.373);
    t->colors[210] = cairo_pattern_create_rgb(1.000, 0.529, 0.529);
    t->colors[211] = cairo_pattern_create_rgb(1.000, 0.529, 0.686);
    t->colors[212] = cairo_pattern_create_rgb(1.000, 0.529, 0.843);
    t->colors[213] = cairo_pattern_create_rgb(1.000, 0.529, 1.000);
    t->colors[214] = cairo_pattern_create_rgb(1.000, 0.686, 0.000);
    t->colors[215] = cairo_pattern_create_rgb(1.000, 0.686, 0.373);
    t->colors[216] = cairo_pattern_create_rgb(1.000, 0.686, 0.529);
    t->colors[217] = cairo_pattern_create_rgb(1.000, 0.686, 0.686);
    t->colors[218] = cairo_pattern_create_rgb(1.000, 0.686, 0.843);
    t->colors[219] = cairo_pattern_create_rgb(1.000, 0.686, 1.000);
    t->colors[220] = cairo_pattern_create_rgb(1.000, 0.843, 0.000);
    t->colors[221] = cairo_pattern_create_rgb(1.000, 0.843, 0.373);
    t->colors[222] = cairo_pattern_create_rgb(1.000, 0.843, 0.529);
    t->colors[223] = cairo_pattern_create_rgb(1.000, 0.843, 0.686);
    t->colors[224] = cairo_pattern_create_rgb(1.000, 0.843, 0.843);
    t->colors[225] = cairo_pattern_create_rgb(1.000, 0.843, 1.000);
    t->colors[226] = cairo_pattern_create_rgb(1.000, 1.000, 0.000);
    t->colors[227] = cairo_pattern_create_rgb(1.000, 1.000, 0.373);
    t->colors[228] = cairo_pattern_create_rgb(1.000, 1.000, 0.529);
    t->colors[229] = cairo_pattern_create_rgb(1.000, 1.000, 0.686);
    t->colors[230] = cairo_pattern_create_rgb(1.000, 1.000, 0.843);
    t->colors[231] = cairo_pattern_create_rgb(1.000, 1.000, 1.000);
    t->colors[232] = cairo_pattern_create_rgb(0.031, 0.031, 0.031);
    t->colors[233] = cairo_pattern_create_rgb(0.071, 0.071, 0.071);
    t->colors[234] = cairo_pattern_create_rgb(0.110, 0.110, 0.110);
    t->colors[235] = cairo_pattern_create_rgb(0.149, 0.149, 0.149);
    t->colors[236] = cairo_pattern_create_rgb(0.188, 0.188, 0.188);
    t->colors[237] = cairo_pattern_create_rgb(0.227, 0.227, 0.227);
    t->colors[238] = cairo_pattern_create_rgb(0.267, 0.267, 0.267);
    t->colors[239] = cairo_pattern_create_rgb(0.306, 0.306, 0.306);
    t->colors[240] = cairo_pattern_create_rgb(0.345, 0.345, 0.345);
    t->colors[241] = cairo_pattern_create_rgb(0.376, 0.376, 0.376);
    t->colors[242] = cairo_pattern_create_rgb(0.400, 0.400, 0.400);
    t->colors[243] = cairo_pattern_create_rgb(0.463, 0.463, 0.463);
    t->colors[244] = cairo_pattern_create_rgb(0.502, 0.502, 0.502);
    t->colors[245] = cairo_pattern_create_rgb(0.541, 0.541, 0.541);
    t->colors[246] = cairo_pattern_create_rgb(0.580, 0.580, 0.580);
    t->colors[247] = cairo_pattern_create_rgb(0.620, 0.620, 0.620);
    t->colors[248] = cairo_pattern_create_rgb(0.659, 0.659, 0.659);
    t->colors[249] = cairo_pattern_create_rgb(0.698, 0.698, 0.698);
    t->colors[250] = cairo_pattern_create_rgb(0.737, 0.737, 0.737);
    t->colors[251] = cairo_pattern_create_rgb(0.776, 0.776, 0.776);
    t->colors[252] = cairo_pattern_create_rgb(0.816, 0.816, 0.816);
    t->colors[253] = cairo_pattern_create_rgb(0.855, 0.855, 0.855);
    t->colors[254] = cairo_pattern_create_rgb(0.894, 0.894, 0.894);
    t->colors[255] = cairo_pattern_create_rgb(0.933, 0.933, 0.933);

    t->default_rows = 24;
    t->default_cols = 80;
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
    if (!strcmp(key, "font")) {
        free(t->font_name);
        t->font_name = runes_config_parse_string(val);
    }
    else if (!strcmp(key, "bold_is_bright")) {
        t->bold_is_bright = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bold_is_bold")) {
        t->bold_is_bold = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "audible_bell")) {
        t->audible_bell = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bell_is_urgent")) {
        t->bell_is_urgent = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bgcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(t->bgdefault);
            t->bgdefault = newcolor;
        }
    }
    else if (!strcmp(key, "fgcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(t->fgdefault);
            t->fgdefault = newcolor;
        }
    }
    else if (!strcmp(key, "mousecursorcolor")) {
        cairo_pattern_t *newcolor;
        newcolor = runes_config_parse_color(val);
        if (newcolor) {
            cairo_pattern_destroy(t->mousecursorcolor);
            t->mousecursorcolor = newcolor;
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
            cairo_pattern_destroy(t->colors[i]);
            t->colors[i] = newcolor;
        }
    }
    else if (!strcmp(key, "rows")) {
        t->default_rows = runes_config_parse_uint(val);
    }
    else if (!strcmp(key, "cols")) {
        t->default_cols = runes_config_parse_uint(val);
    }
    else if (!strcmp(key, "command")) {
        t->cmd = runes_config_parse_string(val);
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
