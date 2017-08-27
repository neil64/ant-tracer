/*****************************************************************************\
*                  ____  __  __        ____      ____                         *
*                 / __ \/ /_/ /_____  / __ \    /  _/___  _____               *
*                / / / / __/ __/ __ \/ / / /    / // __ \/ ___/               *
*               / /_/ / /_/ /_/ /_/ / /_/ /   _/ // / / / /___                *
*               \____/\__/\__/\____/\___\_\  /___/_/ /_/\___(_)               *
*                                                                             *
*   Copyright (c) 2015-2017 OttoQ Inc.                                        *
*   All rights reserved.                                                      *
*                                                                             *
*   Redistribution  and use in  source and  binary forms,  with  or without   *
*   modification, are  permitted provided that the following conditions are   *
*   met:                                                                      *
*                                                                             *
*   1.  Redistributions  of  source  code  must retain the above  copyright   *
*       notice, this list of conditions and the following disclaimer.         *
*   2.  Redistributions in binary  form must reproduce  the above copyright   *
*       notice, this list  of conditions and the  following  disclaimer  in   *
*       the   documentation   and/or  other  materials  provided  with  the   *
*       distribution.                                                         *
*   3.  All  advertising  materials  mentioning  features  or  use of  this   *
*       software  must display the following acknowledgment:  "This product   *
*       includes software developed by OttoQ Inc."                            *
*   4.  The  name OttoQ Inc may not be used to endorse or  promote products   *
*       derived   from   this   software  without  specific  prior  written   *
*       permission.                                                           *
*                                                                             *
*   THIS  SOFTWARE  IS PROVIDED BY OTTOQ INC AND CONTRIBUTORS ``AS IS'' AND   *
*   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED  TO,  THE   *
*   IMPLIED  WARRANTIES  OF  MERCHANTABILITY  AND  FITNESS FOR A PARTICULAR   *
*   PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL OTTOQ INC OR CONTRIBUTORS BE   *
*   LIABLE  FOR  ANY  DIRECT,  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR   *
*   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  TO,  PROCUREMENT  OF   *
*   SUBSTITUTE  GOODS  OR  SERVICES;   LOSS  OF  USE, DATA, OR PROFITS;  OR   *
*   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY  OF  LIABILITY,   *
*   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR   *
*   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  IF   *
*   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                *
*                                                                             *
*   (This license is derived from the Berkeley Public License.)               *
*                                                                             *
\*****************************************************************************/

/*
 *  Debugging glue.
 */

#include "debug.h"
#include "lib/rtt/SEGGER_RTT.h"


#if OQ_DEBUG

/**********************************************************************/

static void
setMode(bool block)
{
    static int mode = false;

    if (mode == block)
        return;
    mode = block;

    u32 f = block ? SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL :
                    SEGGER_RTT_MODE_NO_BLOCK_SKIP;

    SEGGER_RTT_ConfigUpBuffer(0, (const char *)0, (void *)0, 0, f);
}


static void
wrStr(const char * str, unsigned len)
{
    const char * s = str;
    const char * se = str + len;

    while (s < se)
    {
        while (s < se && *s != '\n')
            s++;
        SEGGER_RTT_Write(0, str, (s - str));

        if (s < se && *s == '\n')
        {
            DebugPutChar('\n');
            str = s + 1;
        }
        else
            str = s;
    }
}


static void
wrChr(int chr)
{
    if (chr == '\n')
        wrChr('\r');
    char c = chr;
    SEGGER_RTT_Write(0, &c, 1);
}

/******************************/

void
DebugPutChar(int chr)
{
    setMode(false);
    wrChr(chr);
}


void
DebugPutString(const char * str, unsigned len)
{
    setMode(false);
    wrStr(str, len);
}


void
DebugPutCharBlocked(int chr)
{
    setMode(true);
    wrChr(chr);
}


void
DebugPutStringBlocked(const char * str, unsigned len)
{
    setMode(true);
    wrStr(str, len);
}


int
DebugPutAvail(void)
{
    return SEGGER_RTT_WriteAvailSpace(0);
}

/**********************************************************************/

int
DebugGetChar(void)
{
    char c;
    unsigned x = SEGGER_RTT_Read(0, &c, 1);
    if (x > 0)
        return c;
    else
        return -1;
}


int
DebugGetCharBlocking(void)
{
    return SEGGER_RTT_WaitKey();
}

/**********************************************************************/

/*
 *  Issued when the system is about to shutdown.  This allows us to invalidate
 *  any RTT buffers before the system goes down.
 */
void
DebugShutdown(void)
{
    SEGGER_RTT_Shutdown();
}

/**********************************************************************/

#endif // OQ_DEBUG

/**********************************************************************/
