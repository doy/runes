#ifndef _RUNES_DISPLAY_H
#define _RUNES_DISPLAY_H

void runes_display_init(RunesTerm *t);
void runes_display_get_term_size(RunesTerm *t, int *row, int *col, int *xpixel, int *ypixel);
void runes_display_glyph(RunesTerm *t, char *buf, size_t len);

#endif
