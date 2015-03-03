#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: Change logging destination. */

#define PEPPER_ERROR(fmt, ...)			    \
    do {					    \
	printf(fmt, ##__VA_ARGS__);		    \
    } while (0)



#define PEPPER_TRACE	PEPPER_ERROR

#endif /* COMMON_H */
