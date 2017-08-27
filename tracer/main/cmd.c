/*
 *  LiMon - Command processing.
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
 *     "This product includes software developed by Neil Russell."
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

#if OQ_COMMAND

#include "debug/debug.h"

/**********************************************************************/
/*
 *  Support.
 */

/*
 *  Case insensitive compare.  Returns 0 if both strings match, 1 if `text'
 *  is shorter than `cmd' and the remaining characters in `cmd' are lower
 *  case, and 2 otherwise.  This is used to compare command line text with
 *  command names.
 */
int
StrcmpCmd(const char * cmd, const char * text)
{
    if (cmd && text)
      do {
        char c1 = *cmd;
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += 'a' - 'A';

        char c2 = *text;
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += 'a' - 'A';

        if (c1 != c2)
        {
            if (c2 == '\0')
            {
                for (; *cmd; cmd++)
                    if (*cmd >= 'A' && *cmd <= 'Z')
                        return 2;
                return 1;
            }
            return 2;
        }

        text++;
    } while (*cmd++ != '\0');

    return 0;
}


u32
GetDecimal(const char * sp)
{
    u32 num = 0;
    int neg = false;

    if (*sp == '-')
    {
        sp++;
        neg = true;
    }
    else if (*sp == '+')
        sp++;

    while (*sp >= '0' && *sp <= '9')
    {
        num *= 10;
        num += *sp - '0';
        sp++;
    }

    if (neg)
        num = -num;
    return num;
}


/*
 *  Get a fixed point number.  Return the value as an integer multiplied
 *  by 1000;  that is, only 3 decimal places.
 */
int
GetFixed(const char * sp)
{
    u32 num = 0;
    u32 frac = 0;
    int sign = 0;

    while (*sp == ' ' || *sp == '\t')
        sp++;

    if (*sp == '-')
    {
        sign = 1;
        sp++;
    }

    while (*sp >= '0' && *sp <= '9')
    {
        num *= 10;
        num += *sp - '0';
        sp++;
    }

    if (*sp == '.')
    {
        sp++;
        int x = 0;
        while (*sp >= '0' && *sp <= '9')
        {
            frac *= 10;
            frac += *sp - '0';
            sp++;
            if (++x >= 3)
                break;
        }
        while (x++ < 3)
            frac *= 10;
    }

    int ret = num * 1000;
    ret += frac;
    if (sign)
        ret = -ret;

    return ret;
}

/**********************************************************************/

static const Cmd_t *
matchCmd(char * cmd)
{
    const Cmd_t * const * cpp = &CommandList[0];
    const Cmd_t * cp;
    while ((cp = *cpp++))
    {
        if ((cp->cmd && StrcmpCmd(cp->cmd, cmd) <= 1) ||
            (cp->cmd1 && StrcmpCmd(cp->cmd1, cmd) <= 1))
                return cp;
    }
    return 0;
}


static void
help(int argc, char ** argv)
{
    void
    pr(const Cmd_t * cp)
    {
        const char * h0 = cp->help0;
        if (h0)
            dbprintf(" %-21s  %s\n", h0, cp->help1 ? : "");
    }

    if (argv[1])
    {
        const Cmd_t * cp = matchCmd(argv[1]);
        if (cp)
        {
            pr(cp);
            if (cp->help9)
                dbprintf("\n%s", cp->help9);
            dbprintf("\n");
        }
        else
            dbprintf("no such command: %s\n", argv[1]);
    }
    else
    {
        const Cmd_t * const * cpp = &CommandList[0];
        const Cmd_t * cp;
        while ((cp = *cpp++))
            pr(cp);
    }
}


COMMAND(000)
{
    help, "HELP", "?"
};

/**********************************************************************/

#if 0

static void
version(int argc, char ** argv)
{
    dprintf("Relay2 application v" VERSION_STRING "\n");
}


COMMAND(001)
{
    version, "VERsion",
};

#endif // 0

/**********************************************************************/

static char     line[256];
static char *   linep = &line[0];
static char *   args[16];


/*
 *  Command handler.  Checks for new characters from the debug port,
 *  collects them into a group of words and executes them as commands.
 *  Each time it's called, it checks for a new character.  If one is
 *  found, it tosses it at a state machine.
 */
void
CommandLoop(void)
{
    /*
     *  If this is the first time here, say hello.
     */
    static bool beenHere;
    if (!beenHere)
    {
        if (DebugPutAvail() < 80)
            return;

        dprintf("\nTracer debugger        "
                "(type \"help\" for more information)\n\n");

        beenHere = true;
        goto prompt;
    }

    /*
     *  Wait for enough characters to become available in the
     *  transmit queue for the longest message we will print
     *  here.
     */
    if (DebugPutAvail() < 32)
        return;

    /*
     *  Check for a new character, and return if none was found.
     */
    int c = DebugGetChar();
    if (c < 0)
        return;

    /*
     *  Fold CR & LF -- a CR or LF, or CRLF all amount to a single
     *  line end.
     */
    {
        static int CR = false;
        if (c == '\r')
        {
            CR = true;
            c = '\n';
        }
        else
        {
            int CR1 = CR;
            CR = false;
            if (CR1 && c == '\n')
                return;
        }
    }

    /*
     *  Do line processing with the new character.
     */
    switch (c)
    {
    case '\r':
    case '\n':
        dprintf("\n");
        *linep = '\0';
        break;

    case '\b':
        if (linep > &line[0])
        {
            linep--;
            dprintf("\b \b");
        }
        return;

    default:
        if (c >= ' ' && c <= '~' && linep < &line[sizeof line] - 1)
        {
            *linep++ = c;
            dprintf("%c", c);
        }
        return;
    }

    /*
     *  We have a complete line.  Start processing.
     *  First, break the command up into words.
     */
    linep = &line[0];
    char * cp = &line[0];
    char ** ap = &args[0];

    /*
     *  Break the command up into words.
     */
    while (ap < &args[(sizeof args / sizeof args[0]) - 1])
    {
        while (*cp == ' ' || *cp == '\t')
            cp++;
        if (!*cp)
            break;

        *ap++ = cp++;

        while (*cp != '\0' && *cp != ' ' && *cp != '\t')
            cp++;

        if (*cp)
            *cp++ = '\0';
    }
    if (ap == &args[0])
        goto prompt;            //  Nothing typed

    char ** ap1 = ap;
    while (ap1 < &args[(sizeof args / sizeof args[0])])
        *ap1++ = (char *)0;

    /********************/
    /*
     *  Figure out which command, and execute it.
     */
    const Cmd_t * cmd = matchCmd(args[0]);
    if (cmd)
        (*cmd->func)(ap - &args[0], &args[0]);
    else
        dprintf("Unknown command: %s\n", args[0]);

prompt:
    dprintf("Tracer> ");
}

#endif // OQ_COMMAND

/**********************************************************************/
