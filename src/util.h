#ifndef _RUNES_UTIL_H
#define _RUNES_UTIL_H

#define UNUSED(x) ((void)x)

void runes_warn(const char *fmt, ...);
int sprintf_dup(char **out, const char *fmt, ...);

#endif
