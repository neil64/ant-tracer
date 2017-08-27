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
 *  Boot loader definitions.
 */

#ifndef __BL_H__
#define __BL_H__

#include    <stdbool.h>
#include    <stddef.h>
#include    <stdint.h>

/**********************************************************************/

typedef unsigned int        uint;
typedef unsigned char       uchar;
typedef signed char         schar;
typedef unsigned short      ushort;
typedef unsigned long       ulong;

typedef schar       s8;
typedef uchar       u8;
typedef short       s16;
typedef ushort      u16;
typedef int         s32;
typedef unsigned    u32;

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

#define MAXINT  ((int)(((uint)-1) >> 1))
#define MININT  ((int)((uint)MAX32 + 1))
#define MAXUINT ((uint)-1)

#define BITS_PER_BYTE   8

/******************************/

#define BIT(s)          1
#define BIT_RANGE(s, e) (e - s + 1)

#define ALIGN_16(x) (MAXU16 & ((x + 1) & ~1))
#define ALIGN_32(x) (MAXU32 & ((x + 3) & ~3))

#define ARRAY_SIZE(t)   (sizeof (t) / sizeof (t)[0])

/**********************************************************************/

/*
 *  High frequency clock.  This clock runs periodically when needed, mostly
 *  to clock the CPU when its active, and to generate the carrier frequency
 *  for the radio.
 */
#define OQ_HFCLK            64000000

/*
 *  Low frequency clock.  This clock is used to time long running events.
 *  It's used to schedule radio packets, time application events, and keep
 *  time-of-day.  It's always enabled, and is extremely power efficient.
 */
#define OQ_LFCLK            32768

/******************************/

/*
 *  Flash space values.
 */
#define OQ_FLASH_START      0x0                 //  Flash start address
#define OQ_FLASH_BLOCK      512                 //  Flash block size
#define OQ_FLASH_PAGE       4096                //  Flash page size
#define OQ_FLASH_PAGES      128                 //  Flash pages on nRF52
#define OQ_FLASH_SIZE       (128*OQ_FLASH_PAGE) //  Flash size

/*
 *  Software update parameters.
 */
#define OQ_SU_CHUNK            64              //  Software update chunk size
#define OQ_SU_CHUNK_SHIFT      6               //  Software update chunk size

/**********************************************************************/

// misc/crc.c

typedef struct
{
    u32 crc;
}
    CRC_t;

extern void     CRCInit(CRC_t * crc);
extern void     CRC(CRC_t * crc, u8 * data, unsigned len);
extern void     CRC8(CRC_t * crc, u32 data);
extern void     CRC16(CRC_t * crc, u32 data);
extern void     CRC32(CRC_t * crc, u32 data);

/**********************************************************************/

extern u32      StoreStart;
extern u32      StoreEnd;
extern u32      ConfigStart;


typedef struct
{
    unsigned    sequence;           //  Sequence ID this record was stored with
    u32         version;            //  Software update version
    unsigned    start;              //  Software update start address
    unsigned    end;                //  Software update end address (+1)
}
    SuInfo_t;


typedef struct
{
    unsigned    sequence;           //  Sequence ID this record was stored with
    unsigned    address;
    u8          data[64];
}
    SuData_t;


typedef struct
{
    unsigned    sequence;           //  Sequence ID this record was stored with
    u32         version;            //  Software update version
}
    SuExec_t;


extern void     StoreCallbackSoftwareUpdateInfo(SuInfo_t * info);
extern void     StoreCallbackSoftwareUpdateChunk(SuData_t * data);
extern void     StoreCallbackSoftwareUpdateExecute(SuExec_t * info);

extern void     StoreRead(void);

/********************/

extern void     GoSoftDevice(void);

extern void *   memset(void * dest, int c, size_t sz);
extern int      memcmp(const void * l, const void * r, size_t cnt);

/**********************************************************************/

#endif // __BL_H__

/**********************************************************************/
