#include "common.h"
#include <stdlib.h>
#include <string.h>

char *
pepper_string_alloc(int len)
{
    return (char *)pepper_malloc((len + 1) * sizeof (char));
}

char *
pepper_string_copy(const char *str)
{
    int len = strlen(str);
    char *ret = pepper_string_alloc(len);

    if (ret)
	memcpy(ret, str, (len + 1) * sizeof (char));

    return ret;
}

void
pepper_string_free(char *str)
{
    pepper_free(str);
}

void *
pepper_malloc(size_t size)
{
    return malloc(size);
}

void *
pepper_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

void
pepper_free(void *ptr)
{
    free(ptr);
}
