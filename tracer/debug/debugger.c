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


#if OQ_COMMAND

#include "debug/debug.h"

/********************/

void
ToHex(char * str, u32 val, int digits)
{
    while (digits-- > 0)
    {
        str[digits] = HexString[val & 0xf];
        val >>= 4;
    }
}

/**********************************************************************/

u32
GetHex(char * sp)
{
    u32 num = 0;

    while
    (
        (*sp >= '0' && *sp <= '9')
        ||
        (*sp >= 'a' && *sp <= 'f')
        ||
        (*sp >= 'A' && *sp <= 'F')
    )
    {
        num *= 16;
        if (*sp <= '9')
            num += *sp - '0';
        else if (*sp <= 'F')
            num += *sp - 'A' + 10;
        else
            num += *sp - 'a' + 10;
        sp++;
    }

    return num;
}

/**********************************************************************/

/*
 *  Move the given memory, using the largest transfer size we can
 *  1, 2 or 4 bytes).  Return 1 if we were able to move the memory,
 *  or zero if we failed for any reason, such as a page fault or
 *  machine check.
 */
int
DebugMove(u8 * from, u8 * to, uint count)
{
    // extern int DataAbortExceptionFlag;
    // DataAbortExceptionFlag = 1;
    int ret = 0;

    /*
     *  The normal memory read/write.
     */
    if (((int)from & 3) == 0 && ((int)to & 3) == 0)
    {
        u32 * t = (u32 *)to;
        u32 * f = (u32 *)from;

        while (count >= 4)
        {
            *t++ = *f++;
            count -= 4;
        }

        to = (u8 *)t;
        from = (u8 *)f;
    }

    if (((int)from & 1) == 0 && ((int)to & 1) == 0)
    {
        u16 * t = (u16 *)to;
        u16 * f = (u16 *)from;

        while (count >= 2)
        {
            *t++ = *f++;
            count -= 2;
        }

        to = (u8 *)t;
        from = (u8 *)f;
    }

    while (count-- > 0)
        *to++ = *from++;

    /*
     *  Check if there was a memory access error.
     */
    ret = 1;    // ret = DataAbortExceptionFlag;
// out:
    // DataAbortExceptionFlag = 0;

    return ret == 1;
}

/**********************************************************************/

static void
debugCompare(int argc, char ** argv)
{
    if (argc < 4)
        return;

    unsigned left = GetHex(argv[1]);
    unsigned right = GetHex(argv[2]);
    unsigned len = GetHex(argv[3]);

    int cnt = 0;
    while (len-- > 0)
    {
        u8 l, r;

        if (!DebugMove((u8 *)left, &l, 1))
        {
            dprintf("memory fault at %x\n", left);
            return;
        }
        if (!DebugMove((u8 *)right, &r, 1))
        {
            dprintf("memory fault at %x\n", right);
            return;
        }

        if (l != r)
        {
            dprintf("%08x = %02x,  %08x = %02x\n", left, l, right, r);
            if (++cnt >= 20)
            {
                dprintf("too many diffs;  stopping\n");
                return;
            }
        }

        left++;
        right++;
    }
}

/**********************************************************************/

static void
debugMemory(int argc, char ** argv)
{
    static u32  addr;
    int         i;
    char *      ptr;
    int         mod;
    int         sz;
    int         cnt;
    union {
        u8 x8[16];
        u16 x16[8];
        u32 x32[4];
    } data;
    char        line[128];

    /*
     *      Figure out what we are doing.
     */
    ptr = argv[0];

    if (*ptr == 'm' || *ptr == 'M')
        mod = 1;
    else
        mod = 0;

    while (ptr[1])
        ptr++;

    sz = 1, cnt = 16;
    if (*ptr == 's' || *ptr == 'S' || *ptr == 'h' || *ptr == 'H')
        sz = 2, cnt = 16;
    else if (*ptr == 'l' || *ptr == 'L' || *ptr == 'w' || *ptr == 'W')
        sz = 4, cnt = 16;
    else if (*ptr == '1')
        sz = 1, cnt = 1;
    else if (*ptr == '2')
        sz = 2, cnt = 2;
    else if (*ptr == '4')
        sz = 4, cnt = 4;

    /*
     *      Get the memory address.  Use the previous address if
     *      we didn't get one.
     */
    if (argv[1])
        addr = GetHex(argv[1]);

    do
    {
    newline:
        ptr = &line[0];
        ToHex(ptr, addr, 8);
        ptr += 8;
        *ptr++ = ' ';

        for (i = 0; i < cnt; i += sz)
        {
            if (i % 4 == 0)
                *ptr++ = ' ';

            if (DebugMove((u8 *)addr + i, &data.x8[i], sz))
            {
                *ptr++ = ' ';
                if (sz == 4)
                {
                    ToHex(ptr, data.x32[i/4], 8);
                    ptr += 8;
                }
                else if (sz == 2)
                {
                    ToHex(ptr, data.x16[i/2], 4);
                    ptr += 4;
                }
                else
                {
                    ToHex(ptr, data.x8[i], 2);
                    ptr += 2;
                }
            }
            else
            {
                int j;

                *ptr++ = ' ';
                for (j = 0; j < sz; j++)
                {
                    *ptr++ = '-';
                    *ptr++ = '-';
                    data.x8[i+j] = 0;
                }
            }
        }

        *ptr++ = ' ';
        *ptr++ = ' ';

        for (i = 0; i < cnt; i++)
        {
            if (data.x8[i] >= ' ' && data.x8[i] <= '~')
                *ptr++ = data.x8[i];
            else
                *ptr++ = '.';
        }

        *ptr = '\0';
        dprintf("%s", line);

        if (mod)
        {
            int pos;
            int idx;
            int chr;
            u32 val;

            i = 0;
            pos = -1;
            for (;;)
            {
                idx = pos;
                switch (sz)
                {
                case 1:
                    pos = 11 + i*3 + i/4;
                    val = data.x8[i];
                    break;
                case 2:
                    pos = 11 + (i/2)*5 + i/4;
                    val = data.x16[i/2];
                    break;
                case 4:
                    pos = 11 + (i/4)*10;
                    val = data.x32[i/4];
                    break;

                default:
                    val = 0;
                    break;
                }

                if (idx != pos)
               {
                    if (idx < 0 || idx > pos)
                    {
                        dprintf("\r");
                        idx = 0;
                    }
                    while (idx < pos)
                        dprintf("%c", line[idx++]);
                }

                idx = 0;
                for (;;)
                {
                    chr = DebugGetCharBlocking();
                    if (chr == '\033')
                    {
                        dprintf("\n");
                        return;
                    }
                    else if (chr == '\b')
                    {
                        if (idx > 0)
                        {
                            idx--;
                            dprintf("\b");
                        }
                        else
                        {
                            if (i > 0)
                            {
                                i -= sz;
                                break;
                            }
                            else
                            {
                                addr -= cnt;
                                dprintf("\r");
                                goto newline;
                            }
                        }
                    }
                    else if (chr == '-')
                    {
                        addr -= cnt;
                        dprintf("\r");
                        goto newline;
                    }
                    else if (chr == '+')
                    {
                        addr += cnt;
                        dprintf("\r");
                        goto newline;
                    }
                    else if (chr == ' ' ||
                             chr == '\r' || chr == '\n')
                    {
                        switch (sz)
                        {
                        case 1:
                            data.x8[i] = val;
                            break;
                        case 2:
                            data.x16[i/2] = val;
                            break;
                        case 4:
                            data.x32[i/4] = val;
                            break;
                        }
                        if (idx > 0)
                            DebugMove(&data.x8[i], (u8 *)addr+i, sz);

                        if (chr == ' ')
                            i += sz;
                        else
                            i = cnt;
                        if (i >= cnt)
                        {
                            addr += cnt;
                            dprintf("\n");
                            goto newline;
                        }
                        else
                        {
                            pos += idx;
                            break;
                        }
                    }
                    else if (chr >= '0' && chr <= '9')
                    {
                        chr -= '0';
                        goto digit;
                    }
                    else if (chr >= 'a' && chr <= 'f')
                    {
                        chr -= 'a' - 10;
                        goto digit;
                    }
                    else if (chr >= 'A' && chr <= 'F')
                    {
                        chr -= 'A' - 10;
                    digit:
                        if (line[pos] == '-')
                        {
                            dprintf("\007");
                            continue;
                        }
                        if (idx >= sz*2)
                            continue;
                        line[pos + idx] = HexString[chr];
                        dprintf("%c", HexString[chr]);
                        idx++;
                        val &= ~(0xf << (sz*2 - idx) * 4);
                        val |= chr << (sz*2 - idx) * 4;
                    }
                }
            }
        }

        addr += cnt;
        dprintf("\n");

    } while (DebugGetCharBlocking() != '\033');
}


COMMAND(900)
{
	debugMemory, "Display", "d1",
	"display [address]", "Display memory (type \"help display\" for more)",
	"Memory can be displayed in various formats.  Typing \"display\", or\n"
	"just \"d\" will display bytes, 16 per line.  Typing \"d1\" will\n"
	"display a single byte per line.  \"ds\" or \"dh\" displays in 16-bit\n"
	"words, 8 per line;  \"d2\" for one per line.  \"dl\", \"dw\" or\n"
	"\"d4\" for 32-bit words.  ESC key to exit memory display.  SPACE to\n"
	"move to the next line of 16-bytes."

};
COMMAND(901)
{
	debugMemory, "DS", "D2"
};
COMMAND(902)
{
	debugMemory, "DH", "DW"
};
COMMAND(903)
{
	debugMemory, "DL", "D4"
};
COMMAND(904)
{
	debugMemory, "Modify", "M1",
	"modify [address]", "Modify memory (type \"help modify\" for more)",
	"The modify command uses the same syntax as the display command,\n"
	"but memory can be modified just by typing over the current values.\n"
	"ESC to exit, SPACE to move to the next byte/word, RETURN to move to\n"
	"the next line, BS to move backwards."
};
COMMAND(905)
{
	debugMemory, "MS", "M2"
};
COMMAND(906)
{
	debugMemory, "MH", "MW"
};
COMMAND(907)
{
	debugMemory, "ML", "M4"
};
COMMAND(908)
{
	debugCompare, "CMP", 0,
	"cmp <left> <right> <len>", "Compare memory."
};

#endif // OQ_COMMAND

/**********************************************************************/
