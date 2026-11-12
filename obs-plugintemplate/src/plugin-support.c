#include <plugin-support.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *PLUGIN_NAME = "PelotenseAssets";
const char *PLUGIN_VERSION = "1.0.0";

void obs_log(int log_level, const char *format, ...)
{
    size_t length = 4 + strlen(PLUGIN_NAME) + strlen(format);

    char *template = (char *)malloc(length + 1);

    if (!template)
        return;

    snprintf(template, length + 1, "[%s] %s", PLUGIN_NAME, format);

    va_list args;
    va_start(args, format);
    blogva(log_level, template, args);
    va_end(args);

    free(template);
}
