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
 *  Project wide type definitions.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#include    <stdbool.h>
#include    <stddef.h>
#include    <stdint.h>

/**********************************************************************/

typedef unsigned int        uint;
typedef unsigned char       uchar;
typedef signed char         schar;
typedef unsigned short      ushort;
typedef unsigned long       ulong;
typedef signed long long    shuge;
typedef unsigned long long  uhuge;

typedef schar       s8;
typedef uchar       u8;
typedef short       s16;
typedef ushort      u16;
typedef int         s32;
typedef unsigned    u32;
typedef shuge       s64;
typedef uhuge       u64;

/******************************/

#define MAX8    0x7f
#define MAXU8   0xff
#define MIN8    0x80
#define MAX16   0x7fff
#define MAXU16  0xffff
#define MIN16   0x8000
#define MAX32   0x7fffffff
#define MAXU32  0xffffffff
#define MIN32   0x80000000
#define MAX64   0x7fffffffffffffffLL
#define MAXU64  0xffffffffffffffffLL
#define MIN64   0x8000000000000000LL

#define MAXINT  ((int)(((uint)-1) >> 1))
#define MININT  ((int)((uint)MAX32 + 1))
#define MAXUINT ((uint)-1)

/******************************/

#define BIT(s)          1
#define BIT_RANGE(s, e) (e - s + 1)

#define BITS_PER_BYTE 8

#define ALIGN_16(x) (MAXU16 & ((x + 1) & ~1))
#define ALIGN_32(x) (MAXU32 & ((x + 3) & ~3))

#define ARRAY_SIZE(t)   (sizeof (t) / sizeof (t)[0])

#define NOINLINE    __attribute__ ((__noinline__))

/**********************************************************************/

#ifndef BIG_ENDIAN
#  define BIG_ENDIAN 4321
#endif
#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1234
#endif

#ifndef BYTE_ORDER
#  define BYTE_ORDER LITTLE_ENDIAN
#endif

/****************************************/

/*
 *  Read a 16-bit value from memory (unaligned), and byte swap it to
 *  machine byte order from the protocol byte order (little endian).
 */
static inline u32
OqGet16(const u8 * buf)
{
    u16 v = *(u16 *)buf;
#if BYTE_ORDER == BIG_ENDIAN
    v = ((v >> 8) & 0x000000ff) |
        ((v << 8) & 0x0000ff00);
#endif
    return v;
}

/*
 *  Read a 32-bit value from memory (unaligned), and byte swap it to
 *  machine byte order from the protocol byte order (little endian).
 */
static inline u32
OqGet32(const u8 * buf)
{
    u32 v = *(u32 *)buf;
#if BYTE_ORDER == BIG_ENDIAN
    v = ((v >> 24) & 0x000000ff) |      //  Compiler pattern, gets optimized
        ((v >>  8) & 0x0000ff00) |      //  into a single insn on the Cortex-M
        ((v <<  8) & 0x00ff0000) |
        ((v << 24) & 0xff000000);
#endif
    return v;
}


/*
 *  Read a 64-bit value from memory (unaligned), and byte swap it to
 *  machine byte order from the protocol byte order (little endian).
 */
static inline u64
OqGet64(const u8 * buf)
{
#if BYTE_ORDER != BIG_ENDIAN
    u64 v = ((u64)OqGet32(buf + 0*sizeof (u32)) << 0) |
            ((u64)OqGet32(buf + 1*sizeof (u32)) << 32);
#else
    u64 v = ((u64)OqGet32(buf + 0*sizeof (u32)) << 32) |
            ((u64)OqGet32(buf + 1*sizeof (u32)) << 0);
#endif

#if 0       //  Cortex-M doesn't do unaligned on all 64-bit load/store insns
    u64 v = *(u64 *)buf;
#if BYTE_ORDER == BIG_ENDIAN
    v = ((v >> 56) & 0x00000000000000ffLLU) |       //  Compiler pattern
        ((v >> 40) & 0x000000000000ff00LLU) |       //  It should get optimized
        ((v >> 24) & 0x0000000000ff0000LLU) |       //      into 1 or 2 insns
        ((v >>  8) & 0x00000000ff000000LLU) |
        ((v <<  8) & 0x000000ff00000000LLU) |
        ((v << 24) & 0x0000ff0000000000LLU) |
        ((v << 40) & 0x00ff000000000000LLU) |
        ((v << 56) & 0xff00000000000000LLU);
#endif
#endif
    return v;
}


/*
 *  Byte swap a 16-bit value from machine order to little endian, and
 *  write it to memory (unaligned).
 */
static inline void
OqPut16(u8 * buf, u32 v)
{
#if BYTE_ORDER == BIG_ENDIAN
    v = ((v >>  8) & 0x000000ff) |
        ((v <<  8) & 0x0000ff00);
#endif
    *(u16 *)buf = v;
}


/*
 *  Byte swap a 32-bit value from machine order to little endian, and
 *  write it to memory (unaligned).
 */
static inline void
OqPut32(u8 * buf, u32 v)
{
#if BYTE_ORDER == BIG_ENDIAN
    v = ((v >> 24) & 0x000000ff) |
        ((v >>  8) & 0x0000ff00) |
        ((v <<  8) & 0x00ff0000) |
        ((v << 24) & 0xff000000);
#endif
    *(u32 *)buf = v;
}


/*
 *  Byte swap a 64-bit value from machine order to little endian, and
 *  write it to memory (unaligned).
 */
static inline void
OqPut64(u8 * buf, u64 v)
{
#if BYTE_ORDER != BIG_ENDIAN
    OqPut32(buf + 0*sizeof (u32), v >> 0);
    OqPut32(buf + 1*sizeof (u32), v >> 32);
#else
    OqPut32(buf + 0*sizeof (u32), v >> 32);
    OqPut32(buf + 1*sizeof (u32), v >> 0);
#endif


#if 0       //  Cortex-M doesn't do unaligned on all 64-bit load/store insns
#if BYTE_ORDER == BIG_ENDIAN
    v = ((v >> 56) & 0x00000000000000ffLLU) |
        ((v >> 40) & 0x000000000000ff00LLU) |
        ((v >> 24) & 0x0000000000ff0000LLU) |
        ((v >>  8) & 0x00000000ff000000LLU) |
        ((v <<  8) & 0x000000ff00000000LLU) |
        ((v << 24) & 0x0000ff0000000000LLU) |
        ((v << 40) & 0x00ff000000000000LLU) |
        ((v << 56) & 0xff00000000000000LLU);
#endif
    *(u32 *)(buf+0) = v;
    *(u32 *)(buf+4) = v >> 32;
#endif
}

/****************************************/

/*
 *  Read a 16-bit value from memory (unaligned), and byte swap it to
 *  machine byte order from the protocol byte order (little endian).
 */
static inline u32
OqGet16Big(const u8 * buf)
{
    u16 v = *(u16 *)buf;
#if BYTE_ORDER != BIG_ENDIAN
    v = ((v >> 8) & 0x000000ff) |
        ((v << 8) & 0x0000ff00);
#endif
    return v;
}

/*
 *  Read a 32-bit value from memory (unaligned), and byte swap it to
 *  machine byte order from the protocol byte order (little endian).
 */
static inline u32
OqGet32Big(const u8 * buf)
{
    u32 v = *(u32 *)buf;
#if BYTE_ORDER != BIG_ENDIAN
    v = ((v >> 24) & 0x000000ff) |
        ((v >>  8) & 0x0000ff00) |
        ((v <<  8) & 0x00ff0000) |
        ((v << 24) & 0xff000000);
#endif
    return v;
}


/*
 *  Read a 64-bit value from memory (unaligned), and byte swap it to
 *  machine byte order from the protocol byte order (little endian).
 */
static inline u64
OqGet64Big(const u8 * buf)
{
#if BYTE_ORDER == BIG_ENDIAN
    u64 v = ((u64)OqGet32(buf + 0*sizeof (u32)) << 0) |
            ((u64)OqGet32(buf + 1*sizeof (u32)) << 32);
#else
    u64 v = ((u64)OqGet32(buf + 0*sizeof (u32)) << 32) |
            ((u64)OqGet32(buf + 1*sizeof (u32)) << 0);
#endif

#if 0       //  Cortex-M doesn't do unaligned on all 64-bit load/store insns
    u64 v = *(u64 *)buf;
#if BYTE_ORDER != BIG_ENDIAN
    v = ((v >> 56) & 0x00000000000000ffLLU) |
        ((v >> 40) & 0x000000000000ff00LLU) |
        ((v >> 24) & 0x0000000000ff0000LLU) |
        ((v >>  8) & 0x00000000ff000000LLU) |
        ((v <<  8) & 0x000000ff00000000LLU) |
        ((v << 24) & 0x0000ff0000000000LLU) |
        ((v << 40) & 0x00ff000000000000LLU) |
        ((v << 56) & 0xff00000000000000LLU);
#endif
#endif
    return v;
}


/*
 *  Byte swap a 16-bit value from machine order to little endian, and
 *  write it to memory (unaligned).
 */
static inline void
OqPut16Big(u8 * buf, u32 v)
{
#if BYTE_ORDER != BIG_ENDIAN
    v = ((v >>  8) & 0x000000ff) |
        ((v <<  8) & 0x0000ff00);
#endif
    *(u16 *)buf = v;
}


/*
 *  Byte swap a 32-bit value from machine order to little endian, and
 *  write it to memory (unaligned).
 */
static inline void
OqPut32Big(u8 * buf, u32 v)
{
#if BYTE_ORDER != BIG_ENDIAN
    v = ((v >> 24) & 0x000000ff) |
        ((v >>  8) & 0x0000ff00) |
        ((v <<  8) & 0x00ff0000) |
        ((v << 24) & 0xff000000);
#endif
    *(u32 *)buf = v;
}


/*
 *  Byte swap a 64-bit value from machine order to little endian, and
 *  write it to memory (unaligned).
 */
static inline void
OqPut64Big(u8 * buf, u64 v)
{
#if BYTE_ORDER == BIG_ENDIAN
    OqPut32(buf + 0*sizeof (u32), v >> 0);
    OqPut32(buf + 1*sizeof (u32), v >> 32);
#else
    OqPut32(buf + 0*sizeof (u32), v >> 32);
    OqPut32(buf + 1*sizeof (u32), v >> 0);
#endif


#if 0       //  Cortex-M doesn't do unaligned on all 64-bit load/store insns
#if BYTE_ORDER != BIG_ENDIAN
    v = ((v >> 56) & 0x00000000000000ffLLU) |
        ((v >> 40) & 0x000000000000ff00LLU) |
        ((v >> 24) & 0x0000000000ff0000LLU) |
        ((v >>  8) & 0x00000000ff000000LLU) |
        ((v <<  8) & 0x000000ff00000000LLU) |
        ((v << 24) & 0x0000ff0000000000LLU) |
        ((v << 40) & 0x00ff000000000000LLU) |
        ((v << 56) & 0xff00000000000000LLU);
#endif
    *(u32 *)(buf+0) = v;
    *(u32 *)(buf+4) = v >> 32;
#endif
}

/****************************************/
/*
 *  Byte and bit swapping.
 */

/*
 *  Byte swap a 32-bit value.
 */
static inline u32
Swap32(u32 v)
{
    v = ((v >> 24) & 0x000000ff) |          //  Optimizes to a single insn
        ((v >>  8) & 0x0000ff00) |
        ((v <<  8) & 0x00ff0000) |
        ((v << 24) & 0xff000000);
    return v;
}


/*
 *  Byte swap a 16-bit value.
 */
static inline u32
Swap16(u32 v)
{
    v = ((v >>  8) & 0x000000ff) |          //  Optimizes to a single insn
        ((v <<  8) & 0x0000ff00);
    return v;
}


/*
 *  Byte swap to make a big-endian value, if necessary.
 */
static inline u32
Swap32Big(u32 v)
{
#if BYTE_ORDER != BIG_ENDIAN
    return Swap32(v);
#else
    return v;
#endif
}


/*
 *  Byte swap to make a big-endian value, if necessary.
 */
static inline u32
Swap16Big(u32 v)
{
#if BYTE_ORDER != BIG_ENDIAN
    return Swap16(v);
#else
    return v;
#endif
}


/*
 *  Bit swap a 32-bit value.
 */
static inline u32
SwapBits32(u32 v)
{
    asm ("rbit %0, %1" : "=r" (v) : "r" (v));
    return v;

#if 0
        //  Compiler didn't recognize this pattern
    v = (((v & 0xaaaaaaaa) >> 1) | ((v & 0x55555555) << 1));
    v = (((v & 0xcccccccc) >> 2) | ((v & 0x33333333) << 2));
    v = (((v & 0xf0f0f0f0) >> 4) | ((v & 0x0f0f0f0f) << 4));
    return Swap32(v);
#endif
}


/*
 *  Bit swap a 16-bit value.
 */
static inline u32
SwapBits16(u32 v)
{
    return SwapBits32(v) >> 16;
}


/*
 *  Bit swap a 16-bit value.
 */
static inline u32
SwapBits8(u32 v)
{
    return SwapBits32(v) >> 24;
}

/****************************************/

/*
 *  Returns the bit number of the highest bit set in the given word `w'.
 *  If `w' is zero, it returns -1.
 */
static inline int
FindTopSet(u32 w)
{
    return 31 - __builtin_clz(w);
}


/*
 *  Returns the bit number of the lowest bit set in the given word `w'.
 *  If `w' is zero, it returns 32.
 */
static inline int
FindBottomSet(u32 w)
{
    return __builtin_ctz(w);        //  Single insn on Cortex-M
}

/****************************************/

/*
 *  Return the bit size of a field `f' in the given structure type `t'.
 *  (Returns a value between 1 and 32.)
 */
#define BitSizeField(t, f)          \
    ({                              \
        t sss;                      \
        sss.f = -1;                 \
        FindTopSet(sss.f) + 1;      \
    })

/*
 *  Move a value from `s' to 'd', normalizing.  That is, make sure the top
 *  bit of `s' ends up in the top bit of `d'.
 */
#define NormalizedMove(ds, dm, ss, sm)                          \
    ({                                                          \
        int dz = BitSizeField(typeof (*ds), dm);                \
        int sz = BitSizeField(typeof (*ss), sm);                \
        int sh = dz - sz;                                       \
        if (sh >= 0)                                            \
            (ds)->dm = (ss)->sm << sh;                          \
        else                                                    \
            (ds)->dm = (ss)->sm >> -sh;                         \
    })

#define NormalizedSet(ds, dm, v)                                \
    ({                                                          \
        struct { typeof (v) x; } xx;                            \
        xx.x = (v);                                             \
        NormalizedMove(ds, dm, &xx, x);                         \
    })

#define NormalizedGet(q, ds, dm)                                \
    ({                                                          \
        struct { typeof (q) x; } xx;                            \
        NormalizedMove(&xx, x, ds, dm);                         \
        (q) = xx.x;                                             \
    })

/**********************************************************************/

#endif // __TYPES_H__

/**********************************************************************/
