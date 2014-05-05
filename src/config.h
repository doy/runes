#ifndef _RUNES_CONFIG_H
#define _RUNES_CONFIG_H

struct runes_config {
    cairo_pattern_t *mousecursorcolor;
    cairo_pattern_t *cursorcolor;

    cairo_pattern_t *fgdefault;
    cairo_pattern_t *bgdefault;
    cairo_pattern_t *colors[256];

    int default_rows;
    int default_cols;

    int scroll_lines;
    int scrollback_length;

    char *cmd;
    char *font_name;

    char bell_is_urgent: 1;
    char bold_is_bright: 1;
    char bold_is_bold: 1;
    char audible_bell: 1;
};

void runes_config_init(RunesTerm *t, int argc, char *argv[]);
void runes_config_cleanup(RunesTerm *t);

#endif
