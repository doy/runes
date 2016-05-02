#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "runes.h"

void runes_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    /* XXX make this do something else on windows */
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

int sprintf_dup(char **out, const char *fmt, ...)
{
    int outlen = 0;
    va_list ap;

    va_start(ap, fmt);
    outlen = vsnprintf(*out, outlen, fmt, ap);
    va_end(ap);

    *out = malloc(outlen + 1);

    va_start(ap, fmt);
    outlen = vsnprintf(*out, outlen + 1, fmt, ap);
    va_end(ap);

    return outlen;
}
