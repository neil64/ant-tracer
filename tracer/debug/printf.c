/*
 *  Linux Monitor (LiMon) - Inline debugger.
 *
 *  Copyright (c) 1994, 1995, 2000, 2001 Neil Russell.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or
 *  without modification, are permitted provided that the following
 *  conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *  3. All advertising materials mentioning features or use of
 *     this software must display the following acknowledgement:
 *          "This product includes software developed by Neil Russell."
 *  4. The name Neil Russell may not be used to endorse or promote
 *     products derived from this software without specific prior
 *     written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY NEIL RUSSELL AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 *  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 *  EVENT SHALL NEIL RUSSELL OR CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (This license is derived from the Berkeley Public License.)
 */

/*
 *  Simple printf routine.
 */

#include "types.h"
#include "stdlib.h"
#include <stdarg.h>

#include "defs.h"
#include "store/config.h"
#include "debug/debug.h"


#if OQ_DEBUG

/**********************************************************************/

typedef struct
{
    char * str;
    int	sz;
} pr_t;


#define DEBUGOUT        ((char *)1)
#define DEBUGBLOCKEDOUT ((char *)2)

static pr_t prd = { DEBUGOUT, 0 };
static pr_t prdb = { DEBUGBLOCKEDOUT, 0 };

/*
 *  dprintf levels are managed using the below `printMask' which can be managed
 *  using the `level' command.  The mask is represented by any printf called
 *  which leads with a `<l>' where `l' represents the logging level for that
 *  statement.  Each level maps to a certain type of function which is described
 *  in the `debugLevels' structure below.
 *
 */
static const struct
{
    char level;         //  Debug level
    const char * msg;   //  Message to display
}
    debugLevels[] =
{
    {'t', "ANT protocol"},
    {'b', "broadcast messages"},
    {'s', "scan messages"},
    {'a', "app messages"},
    {'i', "spi message"},
    {'m', "modem state"},
    {'n', "Nordic related"},
};

static u32 printMask = 0;
static u32 currentLevel = 0;

const char HexString[] = "0123456789abcdef";

/**********************************************************************/

#if OQ_COMMAND

/*
 *  Returns the appropriate level (0-25) for a given char.  Returns -1
 *  if the input is not a valid level.
 */
static int
getLevel(const char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    else if (c >= 'a' && c <= 'z')
        return c - 'a';
    return -1;
}


static void
level(int argc, char ** argv)
{
    unsigned bits = 0;
    unsigned neg = 0;

    argv++;
    char * a;
    while ((a = *argv++))
    {
        if (*a == '+')
            a++;
        else if (*a == '-')
        {
            neg = 1;
            a++;
        }

        char c;
        while ((c = *a++))
        {
            int x = getLevel(c);
            if (x < 0)
                continue;
            bits |= (1 << x);
        }

        if (!neg)
            printMask |= bits;
        else
        {
            if (bits)
                printMask &= ~bits;
            else
                printMask = 0;
        }

        Config.printMask = printMask;
        ConfigSave(false);
    }

    dprintf("Level is 0x%08x\n", printMask);
    for (int i = 0; i < ARRAY_SIZE(debugLevels); i++)
    {
        char level = debugLevels[i].level;
        int x = getLevel(level);
        if (x < 0)
            continue;

        char enabled = ' ';
        if ((1 << x) & printMask)
            enabled = '+';

        dprintf("    <%c>  %c  %s\n", level, enabled, debugLevels[i].msg);
    }
}

COMMAND(042)
{
    level, "LEVel", 0,
    "level [-[bits]]", "change the printf filtering mask",
    "`bits' is a collection of zero or more letters (A-Z or a-z),\n"
    "each of which turn on printf statements from paricular sub-systems.\n"
    "If the letters are prepended by a `-', printf statements are turned\n"
    "off.  A `-' will turn off all messages."
};


INITFUNC(007)
{
    printMask = Config.printMask;
}

#endif // OQ_COMMAND

/**********************************************************************/

/*
 *	putchar() and puts() are used by GCC rather than calling printf()
 *	directly should the printf() string be a simple string or character.
 *	It can't be turned off without turning off a lot of other useful
 *	stuff.
 */
// int	putchar(int c)	{ putc(c); return 0; }
// int	puts(char * s)	{ while (*s) putc(*s++);  putc('\n');  return 0; }


static void
xputc(pr_t * pr, int c)
{
    char * out = pr->str;

    if (out == DEBUGOUT || out == DEBUGBLOCKEDOUT)
    {
        if (currentLevel == 0 || (currentLevel & printMask) != 0)
        {
            if (out == DEBUGOUT)
                DebugPutChar(c);
            else if (out == DEBUGBLOCKEDOUT)
                DebugPutCharBlocked(c);
        }

        if (c == '\n')
            currentLevel = 0;
    }
    else if (pr->sz > 0)
    {
        *out++ = c;
        pr->str = out;
        pr->sz--;
    }
}

/******************************/

static void
checkLevel(const char ** fmt)
{
    int x = -1;
    const char * f = *fmt;

    if (f[0] != '<')
        return;

    if (f[1] >= 'A' && f[1] <= 'Z')
        x = f[1] - 'A';
    else if (f[1] >= 'a' && f[1] <= 'z')
        x = f[1] - 'a';
    if (x < 0)
        return;

    if (f[2] != '>')
        return;

    currentLevel = 1 << x;
    // f += 3;
    *fmt = f;
}

/******************************/

static void
number(pr_t * out, unsigned n, int prec, int zero, int sign, unsigned base)
{
    char buf[16];
    char * cp;

    if (prec > sizeof buf - 2)
        prec = sizeof buf - 2;
    cp = &buf[sizeof buf];
    *--cp = '\0';
    if (n == 0)
    {
        *--cp = '0';
        prec--;
    }
    else while (n)
    {
        *--cp = HexString[n % base];
        n /= base;
        prec--;
    }

    if (sign)
    {
        if (zero)
            prec--;
        else
            *--cp = '-';
    }
    while (prec-- > 0)
    {
        if (zero)
            *--cp = '0';
        else
            *--cp = ' ';
    }
    if (sign && zero)
        *--cp = '-';

    while (*cp)
    {
        xputc(out, *cp);
        cp++;
    }
}

/******************************/

static void
xprintf(pr_t * out, const char * fmt, va_list ap)
{
    int prec;
    int zero;
    int neg;

    for (; *fmt; fmt++)
    {
        if (*fmt != '%')
            xputc(out, *fmt);
        else
        {
            prec = zero = neg = 0;
            if (*++fmt == '-')
            {
                neg = 1;
                fmt++;
            }
            if (*fmt == '0')
            {
                zero = 1;
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9')
            {
                prec *= 10;
                prec += *fmt++ - '0';
            }

            switch (*fmt)
            {
            case '\0':
                fmt--;
            case '%':
                xputc(out, '%');
                break;

            case 'd':
                {
                    int sign;
                    int num = va_arg(ap, int);
                    if (num < 0)
                    {
                        num = -num;
                        sign = 1;
                    }
                    else
                        sign = 0;
                    number(out, (unsigned)num, prec, zero, sign, 10);
                }
                break;

            case 'u':
            case 'x':
            case 'o':
            case 'b':
                {
                    unsigned num = va_arg(ap, unsigned);
                    number(out, num, prec, zero, 0,
                            *fmt == 'x' ? 16 :
                             (*fmt == 'o' ? 8 :
                              (*fmt == 'b' ? 2 :
                               10)));
                }
                break;

            case 's':
                {
                    char * cp = va_arg(ap, char *);
                    int l = strlen(cp);

                    if (!neg)
                        while (prec > l)
                        {
                            xputc(out, ' ');
                            prec--;
                        }

                    while (*cp)
                    {
                        xputc(out, *cp);
                        cp++;
                        prec--;
                    }

                    if (neg)
                        while (prec-- > 0)
                            xputc(out, ' ');
                }
                break;

            case 'c':
                {
                    int c;

                    c = va_arg(ap, int);
                    xputc(out, c);
                }
                break;

            case 'C':
                {
                    int c = va_arg(ap, int);
                    if (c >= ' ' && c <= '~' && c != '\\')
                        xputc(out, c);
                    else
                    {
                        xputc(out, '\\');
                        if (c == '\\')
                            xputc(out, '\\');
                        else if (c == '\t')
                            xputc(out, 't');
                        else if (c == '\v')
                            xputc(out, 'v');
                        else if (c == '\f')
                            xputc(out, 'f');
                        else if (c == '\r')
                            xputc(out, 'r');
                        else if (c == '\n')
                            xputc(out, 'n');
                        else if (c == '\a')
                            xputc(out, 'a');
                        else if (c == '\e')
                            xputc(out, 'e');
                        else if (c == '\0')
                            xputc(out, '0');
                        else
                        {
                            xputc(out, 'x');
                            number(out, c, 2, 1, 0, 16);
                        }
                    }
                }
                break;
            }
        }
    }
}

/******************************/

#if 0           //  Don't include this one.  Everyone should use snprintf().

int
sprintf(char * out, const char * fmt, ...)
{
    pr_t pr = { out, 0x7fffffff };
    va_list ap;

    va_start(ap, fmt);
    xprintf(&pr, fmt, ap);
    va_end(ap);

    *pr.str = '\0';

    return 0;
}

#endif // 0

/******************************/

int
snprintf(char * out, uint sz, const char * fmt, ...)
{
    pr_t pr = { out, sz-1 };
    va_list ap;

    va_start(ap, fmt);
    xprintf(&pr, fmt, ap);
    va_end(ap);

    *pr.str = '\0';

    return 0;
}

/******************************/

int
dprintf(const char * fmt, ...)
{
    checkLevel(&fmt);

    va_list ap;
    va_start(ap, fmt);
    xprintf(&prd, fmt, ap);
    va_end(ap);

    return 0;
}

/******************************/

int
printf(const char * fmt, ...)
{
    checkLevel(&fmt);

    va_list ap;
    va_start(ap, fmt);
    xprintf(&prd, fmt, ap);
    va_end(ap);

    return 0;
}

/******************************/

int
dbprintf(const char * fmt, ...)
{
    checkLevel(&fmt);

    va_list ap;
    va_start(ap, fmt);
    xprintf(&prdb, fmt, ap);
    va_end(ap);

    return 0;
}

/**********************************************************************/

#endif // OQ_DEBUG

/**********************************************************************/
