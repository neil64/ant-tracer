/*
 *  Gist -- C++ data type library
 *
 *  Copyright (c) 2004-2006 Neil Russell.
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


/*
 *  The following base-85 encoding is inspired by the one found in
 *  PostScript.  However, the character set is different, as are a few of
 *  the surrounding details.  The ' ' is used for zero.  All other
 *  characters are illegal.
 */

#include "types.h"
#include "defs.h"

/**********************************************************************/
/*
 *  Mapping tables for the base-85 character set.
 *
 *  The first table maps a printable character (between 0x20 and 0x7e) to
 *  a base-85 digit (0 - 84), with -1 being an illegal characters.  The
 *  second table maps a base-85 digit (0 - 84) to a character.
 */

static const s8 map0[] =
{
    -1,   0,  -1,  -1,    -1,   1,   2,   3,        //  0x20
     4,   5,   6,  -1,     7,   8,   9,  10,        //  0x28
    11,  12,  13,  14,    15,  16,  17,  18,        //  0x30
    19,  20,  21,  22,    23,  24,  25,  26,        //  0x38
    27,  28,  -1,  29,    30,  31,  32,  33,        //  0x40
    34,  35,  36,  37,    38,  39,  40,  -1,        //  0x48
    41,  42,  -1,  43,    44,  45,  46,  47,        //  0x50
    48,  49,  50,  51,    -1,  52,  -1,  53,        //  0x58
    54,  55,  56,  57,    58,  59,  60,  61,        //  0x60
    62,  63,  64,  65,    66,  67,  68,  69,        //  0x68
    70,  71,  72,  73,    74,  75,  76,  77,        //  0x70
    78,  79,  80,  81,    82,  83,  84,  -1,        //  0x78
};

static const char map1[] =  "!%&'()*,-."            //   0 - 9
                            "/012345678"            //  10 - 19
                            "9:;<=>?@AC"            //  20 - 29
                            "DEFGHIJKLM"            //  30 - 39
                            "NPQSTUVWXY"            //  40 - 49
                            "Z[]_`abcde"            //  50 - 59
                            "fghijklmno"            //  60 - 69
                            "pqrstuvwxy"            //  70 - 79
                            "z{|}~";                //  80 - 84

/**********************************************************************/

/*
 *  Convert `cnt' bytes from the 32-bit word `w' into base-85, and store
 *  the result into the string at `s'.  The string `s' must be at least 5
 *  characters in size.  The number of characters actually converted is
 *  returned.  `w' must be a "big-endian" representation of the data to be
 *  output;  if there are less than 4 bytes to convert, the valid data is
 *  assumed to be in the most significant bytes of `w'.  The string is
 *  not '\0' terminated.
 */
static int
bin_to_b85(int cnt, u32 w, char * s)
{
#if 0           //  We are not encoding zeros specially
    if (cnt == 4 && w == 0)
    {
        s[0] = ' ';
        return 1;
    }
#endif // 0

    s[4] = map1[w % 85];    w /= 85;
    s[3] = map1[w % 85];    w /= 85;
    s[2] = map1[w % 85];    w /= 85;
    s[1] = map1[w % 85];    w /= 85;
    s[0] = map1[w % 85];

    return cnt + 1;
}


/*
 *  Convert a sequence of binary bytes into a base-85 string.  The binary
 *  data from `from'/`size' is converted, and the result stored in `to'.
 *  The `to' string gets a '_' prefix, and a "\r\n\0" suffix.  It is
 *  expected to be large enough to hold the result, which is
 *  "((size * 5) / 4 + 4".  The number of characters written to `to' is
 *  returned.  If `size' is 0, `to' is set to "", and 0 returned.
 */
unsigned
BinaryToBase85(char * to, const u8 * from, unsigned size)
{
    char * to0 = to;

    if (size > 0)
    {
        //  Add the prefix
        *to++ = '_';

        //  Convert in bulk
        while (size > 4)
        {
            int x = bin_to_b85(4, OqGet32Big(from), to);
            to += x;
            from += 4;
            size -= 4;
        }

        //  There are still 1 - 4 bytes remaining (cannot be 0).  The
        //  remaining bytes must be in the MSB, and the LSB must be zero.
        int sh = (4-size) * 8;
        u32 m = -(1 << sh);
        int x = bin_to_b85(size, (OqGet32Big(from) & m), to);
        to += x;

        //  Terminate
        *to++ = '\r';
        *to++ = '\n';
    }

    *to = '\0';
    return to - to0;
}

/**********************************************************************/

/*
 *  Convert a base-85 string to binary.  `cnt' is the number of characters
 *  to parse from `s'.  The result is stored in `w', in "big-endian"
 *  format.  If there are less than 5 characters, `w' will contain the
 *  bytes available shifted to the most significant bytes.  The number of
 *  bytes converted is returned.  Zero is returned on error.  The caller
 *  should parse the zero (' ');  it is not parsed here.
 */
static int
b85_to_bin(int cnt, const char * s, u32 * w)
{
    if (cnt <= 1)
        return 0;

    u32 n = 0;
    int i = 0;
    do
    {
        u32 x = n * 85;
        if (x < n)
            return 0;       // result is greater than 2^32

        int c = *s++ - 0x20;
        if (c >= 0 && c <= 0x5f)
        {
            c = map0[c];
            if (c < 0)
                return 0;
            n = x + c;
        }
        else
            return 0;
    } while (++i < cnt);

    if (cnt == 2)
    {
        n = n * (85*85*85) + 0xffffff;
        n &= 0xff000000;
    }
    else if (cnt == 3)
    {
        n = n * (85*85) + 0xffff;
        n &= 0xffff0000;
    }
    else if (cnt == 4)
    {
        n = n * 85 + 0xff;
        n &= 0xffffff00;
    }

    *w = n;
    return cnt - 1;
}


/*
 *  Convert the base-85 string `from'/`size' into binary.  The string is
 *  expected to be only the base-85 string, with all prefixes and suffixes
 *  removed.  The number of binary bytes converted is returned, or zero if
 *  there was an error.  (Extra bytes in the destination are written with
 *  zeros, padding the output to a multiple of 4 bytes.)
 */
unsigned
Base85ToBinary(u8 * to, const char * from, unsigned size)
{
    int cnt = 0;

    while (size > 0)
    {
#if 0           //  We are not encoding zeros specially
        if (*from == ' ')
        {
            OqPut32Big(to, 0);      //  Note that with the Zero code removed,
            to += 4;                //  we can do the convertion in place.
            cnt += 4;
            from += 1;
            size -= 1;
        }
        else
#endif // 0
        {
            unsigned sz = size;
            if (sz > 5)
                sz = 5;

            u32 w;
            int x = b85_to_bin(sz, from, &w);
            if (x == 0)
                return 0;
            cnt += x;

            OqPut32Big(to, w);
            to += 4;
            from += sz;
            size -= sz;
        }
    }

    return cnt;
}

/**********************************************************************/

#if 0

#define N 4

int
main(int ac, char ** av)
{
    int i;

    for (i = 0; i < (1 << (8 * (N-1))); i++)
    {
        unsigned long l = i << 24;
        unsigned long r;
        char b[5];
        int c = bin_to_b85(N-1, l, b);
        if (c != N)
        {
            printf("wrong chars\n");
            return 1;
        }
        b85_to_bin(N, b, &r);
        // printf("%6d  %5.*s  %08lx, %08lx\n", i, 5, b, l, r);
        if (l != r)
        {
            printf("not equal at %d (%lx, %lx)\n", i, l, r);
            return 1;
        }
    }

    return 0;
}

#endif

/**********************************************************************/
