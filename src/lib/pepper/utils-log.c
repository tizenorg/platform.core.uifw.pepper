/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "pepper-utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

static FILE *pepper_log_file;
static int pepper_log_verbosity = 3;
static int cached_tm_mday = -1;

static void __attribute__ ((constructor))
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

	localtime_r(&tv.tv_sec, brokendown_time);
	if (brokendown_time == NULL)
		return fprintf(pepper_log_file, "[(NULL)localtime] ");

	if (brokendown_time->tm_mday != cached_tm_mday) {
		strftime(string, sizeof string, "%Y-%m-%d %Z", brokendown_time);
		fprintf(pepper_log_file, "Date: %s\n", string);

		cached_tm_mday = brokendown_time->tm_mday;
	}

	strftime(string, sizeof string, "%H:%M:%S", brokendown_time);

	return fprintf(pepper_log_file, "[%s.%03li] ", string, tv.tv_usec / 1000);
}

static int
pepper_print_domain(const char *log_domain)
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
pepper_log(const char *domain, int level, const char *format, ...)
{
	int l;
	va_list argp;

	if (level > pepper_log_verbosity || level < 0)
		return 0;

	l = pepper_print_timestamp();
	l += pepper_print_domain(domain);

	va_start(argp, format);
	l += pepper_vlog(format, argp);
	va_end(argp);

	return l;
}
