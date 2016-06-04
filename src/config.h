#ifndef _RUNES_CONFIG_H
#define _RUNES_CONFIG_H

#include <cairo.h>

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

    int redraw_rate;

    char *cmd;
    char *font_name;

    unsigned int bell_is_urgent: 1;
    unsigned int bold_is_bright: 1;
    unsigned int bold_is_bold: 1;
    unsigned int audible_bell: 1;
};

RunesConfig *runes_config_new(int argc, char *argv[]);
void runes_config_delete(RunesConfig *config);

#endif
