#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <assert.h>
#include "pepper.h"

#define PEPPER_IGNORE(x)	(void)x

char *
pepper_string_alloc(int len);

char *
pepper_string_copy(const char *str);

void
pepper_string_free(char *str);

void *
pepper_malloc(size_t size);

void *
pepper_calloc(size_t num, size_t size);

void
pepper_free(void *ptr);

#endif /* COMMON_H */
