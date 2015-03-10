/**
 * @file        debug_ch.c
 * @brief       Management of the debugging channels
 *
 * @author
 * @date
 * @attention
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <features.h>
# include <sys/types.h>
# include <sys/utsname.h>

#include "debug_ch.h"

#if defined(__thumb__) && !defined(__thumb2__)
#  define  __ATOMIC_SWITCH_TO_ARM \
            "adr r3, 5f\n" \
            "bx  r3\n" \
            ".align\n" \
            ".arm\n" \
            "5:\n"
/* note: the leading \n below is intentional */
#  define __ATOMIC_SWITCH_TO_THUMB \
            "\n" \
            "adr r3, 6f\n" \
            "bx  r3\n" \
            ".thumb" \
            "6:\n"

#  define __ATOMIC_CLOBBERS   "r3"  /* list of clobbered registers */

#  warning Rebuilding this source file in ARM mode is highly recommended for performance!!

#else
#  define  __ATOMIC_SWITCH_TO_ARM   /* nothing */
#  define  __ATOMIC_SWITCH_TO_THUMB /* nothing */
#  define  __ATOMIC_CLOBBERS        /* nothing */
#endif


static inline int
interlocked_xchg_add( int *dest, int incr )
{
#if defined (__i386__) || defined (__x86_64__)
    int ret;
    __asm__ __volatile__( "lock; xaddl %0,(%1)"
                          : "=r" (ret)
                          : "r" (dest), "0" (incr)
                          : "memory" );
    return ret;
#elif defined (__arm__)
    int ret, tmp, status;
    do {
        __asm__ __volatile__ (
            __ATOMIC_SWITCH_TO_ARM
            "ldrex %0, [%4]\n"
            "add %1, %0, #1\n"
            "strex %2, %1, [%4]"
            __ATOMIC_SWITCH_TO_THUMB
            : "=&r" (ret), "=&r" (tmp), "=&r" (status), "+m"(*dest)
            : "r" (dest)
            : __ATOMIC_CLOBBERS "cc");
    } while (__builtin_expect(status != 0, 0));
    return ret;
#else
#       warning "unknown platform!!"
#endif
}


static const char * const debug_classes[] = {"fixme", "err", "warn", "trace", "info"};

#define MAX_DEBUG_OPTIONS 256

/*
static unsigned char default_flags = (1 << __DBCL_ERR) | (1 << __DBCL_FIXME) | (1 << __DBCL_INFO);
*/
static unsigned char default_flags = (1 << __DBCL_ERR)  | (1 << __DBCL_INFO);
static int nb_debug_options = -1;
static struct _debug_channel debug_options[MAX_DEBUG_OPTIONS];

static void
debug_init(void);

static int
cmp_name(const void *p1, const void *p2)
{
    const char *name = p1;
    const struct _debug_channel *chan = p2;
    return strcmp( name, chan->name );
}

/* get the flags to use for a given channel, possibly setting them too in case of lazy init */
unsigned char
_dbg_get_channel_flags( struct _debug_channel *channel )
{
    if (nb_debug_options == -1)
        debug_init();

    if(nb_debug_options)
    {
        struct _debug_channel *opt;

        /* first check for multi channel */
        opt = bsearch( channel->multiname,
                       debug_options,
                       nb_debug_options,
                       sizeof(debug_options[0]), cmp_name );
        if (opt)
            return opt->flags;

        opt = bsearch( channel->name,
                       debug_options,
                       nb_debug_options,
                       sizeof(debug_options[0]), cmp_name );
        if (opt)
            return opt->flags;
    }

    /* no option for this channel */
    if (channel->flags & (1 << __DBCL_INIT))
        channel->flags = default_flags;

    return default_flags;
}

/* set the flags to use for a given channel; return 0 if the channel is not available to set */
int
_dbg_set_channel_flags(struct _debug_channel *channel,
                       unsigned char          set,
                       unsigned char          clear )
{
    if (nb_debug_options == -1)
        debug_init();

    if (nb_debug_options)
    {
        struct _debug_channel *opt;

        /* first set for multi channel */
        opt = bsearch( channel->multiname,
                       debug_options,
                       nb_debug_options,
                       sizeof(debug_options[0]), cmp_name );
        if (opt)
        {
            opt->flags = (opt->flags & ~clear) | set;
            return 1;
        }

        opt = bsearch( channel->name,
                       debug_options,
                       nb_debug_options,
                       sizeof(debug_options[0]), cmp_name );
        if (opt)
        {
            opt->flags = (opt->flags & ~clear) | set;
            return 1;
        }
    }
    return 0;
}

/* add a new debug option at the end of the option list */
static void
add_option( const char *name, unsigned char set, unsigned char clear )
{
    int min = 0, max = nb_debug_options - 1, pos, res;

    if (!name[0])  /* "all" option */
    {
        default_flags = (default_flags & ~clear) | set;
        return;
    }

    if (strlen(name) >= sizeof(debug_options[0].name))
        return;

    while (min <= max)
    {
        pos = (min + max) / 2;
        res = strcmp( name, debug_options[pos].name );
        if (!res)
        {
            debug_options[pos].flags = (debug_options[pos].flags & ~clear) | set;
            return;
        }
        if (res < 0)
            max = pos - 1;
        else
            min = pos + 1;
    }
    if (nb_debug_options >= MAX_DEBUG_OPTIONS)
        return;

    pos = min;
    if (pos < nb_debug_options)
    {
        memmove( &debug_options[pos + 1],
                 &debug_options[pos],
                 (nb_debug_options - pos) * sizeof(debug_options[0]) );
    }

    strcpy( debug_options[pos].name, name );
    debug_options[pos].flags = (default_flags & ~clear) | set;
    nb_debug_options++;
}

/* parse a set of debugging option specifications and add them to the option list */
static void
parse_options( const char *str )
{
    char *opt, *next, *options;
    unsigned int i;

    if (!(options = strdup(str)))
        return;
    for (opt = options; opt; opt = next)
    {
        const char *p;
        unsigned char set = 0, clear = 0;

        if ((next = strchr( opt, ',' )))
            *next++ = 0;

        p = opt + strcspn( opt, "+-" );
        if (!p[0])
            p = opt;  /* assume it's a debug channel name */

        if (p > opt)
        {
            for (i = 0; i < sizeof(debug_classes)/sizeof(debug_classes[0]); i++)
            {
                int len = strlen(debug_classes[i]);
                if (len != (p - opt))
                    continue;
                if (!memcmp( opt, debug_classes[i], len ))  /* found it */
                {
                    if (*p == '+')
                        set |= 1 << i;
                    else
                        clear |= 1 << i;
                    break;
                }
            }
            if (i == sizeof(debug_classes)/sizeof(debug_classes[0])) /* bad class name, skip it */
                continue;
        }
        else
        {
            if (*p == '-')
                clear = ~0;
            else
                set = ~0;
        }
        if (*p == '+' || *p == '-')
            p++;
        if (!p[0])
            continue;

        if (!strcmp( p, "all" ))
            default_flags = (default_flags & ~clear) | set;
        else
            add_option( p, set, clear );
    }
    free( options );
}

/* print the usage message */
static void
debug_usage(void)
{
    static const char usage[] =
        "Syntax of the DEBUGCH variable:\n"
        "  DEBUGCH=[class]+xxx,[class]-yyy,...\n\n"
        "Example: DEBUGCH=+all,warn-heap\n"
        "    turns on all messages except warning heap messages\n"
        "Available message classes: err, warn, fixme, trace\n";
    const int ret = write( 2, usage, sizeof(usage) - 1 );
    assert(ret >= 0);
    exit(1);
}

/* initialize all options at startup */
static void
debug_init(void)
{
    char *debug = NULL;
    FILE *fp = NULL;
    char *tmp = NULL;

    if (nb_debug_options != -1)
        return;  /* already initialized */

    nb_debug_options = 0;

    fp= fopen("DEBUGCH", "r");
    if( fp == NULL)
    {
        debug = getenv("DEBUGCH");
    }
    else
    {
        if ((tmp= (char *)malloc(1024 + 1)) == NULL)
        {
            fclose(fp);
            return;
        }

        fseek(fp, 0, SEEK_SET);
        const char* str = fgets(tmp, 1024, fp);
        if (str)
        {
            tmp[strlen(tmp)-1] = 0;
            debug = tmp;
        }

        fclose(fp);
    }

    if( debug != NULL )
    {
        if (!strcmp( debug, "help" ))
            debug_usage();
        parse_options( debug );
    }

    if( tmp != NULL )
    {
        free(tmp);
    }
}

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
char *
get_dbg_temp_buffer( size_t size )
{
    static char *list[32];
    static int pos;
    char *ret;
    int idx;

    idx = interlocked_xchg_add( &pos, 1 ) % (sizeof(list)/sizeof(list[0]));

    if ((ret = realloc( list[idx], size )))
        list[idx] = ret;

    return ret;
}

/* release unused part of the buffer */
void
release_dbg_temp_buffer( char *buffer, size_t size )
{
    /* don't bother doing anything */
    (void)( buffer );
    (void)( size );
}

/* jjh */
static FILE* logfile = NULL;
void
dbg_set_logfile(const char* filename)
{
    if (filename)
        logfile = fopen(filename, "a");

    if (logfile == NULL)
        logfile = stderr;
    else
        setvbuf(logfile, NULL, _IOLBF, 256);     /* block buffered -> line buffered */
}

static int
dbg_vprintf( const char *format, va_list args )
{
#if 0
    int ret = vfprintf( stdout, format, args );
    fflush(stdout);
#else
    int ret = vfprintf( logfile, format, args );
    fflush(logfile);
#endif
    return ret;
}

int dbg_printf( const char *format, ... )
{
    int ret;
    va_list valist;

    /* lock */

    va_start(valist, format);
    ret = dbg_vprintf( format, valist );
    va_end(valist);

    /* unlock */

    return ret;
}

int
dbg_printf_nonewline( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = dbg_vprintf( format, valist );
    va_end(valist);

    return ret;
}

/* printf with temp buffer allocation */
const char *
dbg_sprintf( const char *format, ... )
{
    static const int max_size = 200;
    char *ret;
    int len;
    va_list valist;

    va_start(valist, format);
    ret = get_dbg_temp_buffer( max_size );
    len = vsnprintf( ret, max_size, format, valist );
    if (len == -1 || len >= max_size)
        ret[max_size-1] = 0;
    else
        release_dbg_temp_buffer( ret, len + 1 );
    va_end(valist);
    return ret;
}

/* default implementation of dbg_vlog */
static int
dbg_vlog(enum _debug_class      cls,
         struct _debug_channel *channel,
         const char            *func,
         const char            *format,
         va_list                args )
{
    int ret = 0;

    if (cls < sizeof(debug_classes)/sizeof(debug_classes[0]))
    {
        (void)(channel);
        (void)(func);
        /* to print debug class, channel */
        ret += dbg_printf_nonewline("[%s:%s"
                                    , debug_classes[cls] , channel->name);

        if (*channel->multiname)
            ret += dbg_printf_nonewline(":%s]", channel->multiname);
        else
            ret += dbg_printf_nonewline("]");
    }
    if (format)
    {
        ret += dbg_vprintf( format, args );
    }
    return ret;
}

int
dbg_log(enum _debug_class      cls,
        struct _debug_channel *channel,
        const char            *func,
        const char            *format,
        ... )
{
    int ret;
    va_list valist;

    if (!(_dbg_get_channel_flags( channel ) & (1 << cls)))
        return -1;

    va_start(valist, format);
    ret = dbg_vlog( cls, channel, func, format, valist );
    va_end(valist);

    return ret;
}

void
assert_fail(char *exp, const char *file, int line)
{
    fprintf(stderr, "[%s][%d] Assert(%s) failed \n"
            , file, line, exp);
    fprintf(stdout, "[%s][%d] Assert(%s) failed \n"
            , file, line, exp);
    exit(0);
}

/* end of debug_ch.c */
