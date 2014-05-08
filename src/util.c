#include <stdarg.h>
#include <stdio.h>

#include "runes.h"

void runes_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /* XXX make this do something else on windows */
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
