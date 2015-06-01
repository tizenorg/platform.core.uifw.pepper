#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <assert.h>
#include "pepper.h"

#define PEPPER_ASSERT(expr)	assert(expr)
#define PEPPER_INLINE		__inline__
#define PEPPER_IGNORE(x)	(void)x

/* TODO: Change logging destination. */

#define PEPPER_ERROR(fmt, ...)                                          \
    do {                                                                \
        printf("%s:%s: "fmt, __FILE__, __FUNCTION__, ##__VA_ARGS__);	\
    } while (0)

#define PEPPER_TRACE	PEPPER_ERROR

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
