#include "pepper-utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

static FILE *pepper_log_file;
static int perpper_log_verbosity = 3;
static int cached_tm_mday = -1;

void __attribute__ ((constructor))
before_main(void)
{
    pepper_log_file = stdout;
}

static int
pepper_print_timestamp(void)
{
    struct timeval tv;
    struct tm *brokendown_time;
    char string[128];

    gettimeofday(&tv, NULL);

    brokendown_time = localtime(&tv.tv_sec);
    if (brokendown_time == NULL)
        return fprintf(pepper_log_file, "[(NULL)localtime] ");

    if (brokendown_time->tm_mday != cached_tm_mday) {
        strftime(string, sizeof string, "%Y-%m-%d %Z", brokendown_time);
        fprintf(pepper_log_file, "Date: %s\n", string);

        cached_tm_mday = brokendown_time->tm_mday;
    }

    strftime(string, sizeof string, "%H:%M:%S", brokendown_time);

    return fprintf(pepper_log_file, "[%s.%03li] ", string, tv.tv_usec/1000);
}

static int
pepper_print_domain(const char* log_domain)
{
    if (log_domain == NULL)
        return fprintf(pepper_log_file, "UNKNOWN: ");
    else
        return fprintf(pepper_log_file, "%s: ", log_domain);
}

static int
pepper_vlog(const char *format, va_list ap)
{
    return vfprintf(pepper_log_file, format, ap);
}

PEPPER_API int
pepper_log(const char* domain, int level, const char *format, ...)
{
    int l;
    va_list argp;

    if (level > perpper_log_verbosity || level < 0)
        return 0;

    l = pepper_print_timestamp();
    l += pepper_print_domain(domain);

    va_start(argp, format);
    l += pepper_vlog(format, argp);
    va_end(argp);

    return l;
}
