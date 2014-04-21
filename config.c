#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runes.h"

static void runes_config_set_defaults(RunesTerm *t);
static FILE *runes_config_get_config_file();
static void runes_config_process_config_file(RunesTerm *t, FILE *config_file);
static void runes_config_set(RunesTerm *t, char *key, char *value);
static char runes_config_parse_bool(char *val);
static int runes_config_parse_uint(char *val);
static char *runes_config_parse_string(char *val);
static cairo_pattern_t *runes_config_parse_color(char *val);

void runes_config_init(RunesTerm *t, int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    memset((void *)t, 0, sizeof(*t));

    runes_config_set_defaults(t);
    runes_config_process_config_file(t, runes_config_get_config_file());
}

static void runes_config_set_defaults(RunesTerm *t)
{
    t->font_name      = "monospace 10";
    t->bold_is_bright = 1;
    t->bold_is_bold   = 1;

    t->fgdefault = cairo_pattern_create_rgb(0.827, 0.827, 0.827);
    t->bgdefault = cairo_pattern_create_rgb(0.0,   0.0,   0.0);

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

static void runes_config_set(RunesTerm *t, char *key, char *val)
{
    if (!strcmp(key, "font")) {
        t->font_name = runes_config_parse_string(val);
    }
    else if (!strcmp(key, "bold_is_bright")) {
        t->bold_is_bright = runes_config_parse_bool(val);
    }
    else if (!strcmp(key, "bold_is_bold")) {
        t->bold_is_bold = runes_config_parse_bool(val);
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
    else if (!strcmp(key, "rows")) {
        t->default_rows = runes_config_parse_uint(val);
    }
    else if (!strcmp(key, "cols")) {
        t->default_cols = runes_config_parse_uint(val);
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
