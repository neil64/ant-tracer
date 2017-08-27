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
 *  Tachyon tracing.
 */

#ifndef __TACHYON_H__
#define __TACHYON_H__

#include <stdbool.h>
#include "cpu/nrf52.h"

/******************************/

#define TACHY_UNIT  ((unsigned)64000000)

#define TACHY2US(t) ((unsigned)(t) / ((TACHY_UNIT + 999999u) / 1000000u))
#define US2TACHY(t) ((unsigned)(t) * ((TACHY_UNIT + 999999u) / 1000000u))

#define TACHY2MS(t) ((unsigned)(t) / ((TACHY_UNIT + 999u) / 1000u))
#define MS2TACHY(t) ((unsigned)(t) * ((TACHY_UNIT + 999u) / 1000u))

/**********************************************************************/

#if defined(OQ_DEBUG) && defined(OQ_TACHYON)

typedef struct
{
    unsigned   start;          // Start time
    unsigned   delay;          // Delay time
}
    Tachyon_t;

/******************************/

static inline unsigned
TachyonGet(void)
{
    return DWT->CYCCNT;
}


static inline void
TachyonSetUs(Tachyon_t * tp, unsigned us)
{
    tp->delay = US2TACHY(us);
    tp->start = TachyonGet();
}


static inline void
TachyonSetMs(Tachyon_t * tp, unsigned ms)
{
    tp->delay = MS2TACHY(ms);
    tp->start = TachyonGet();
}


static inline void
TachyonSet(Tachyon_t * tp, unsigned tick)
{
    tp->delay = tick;
    tp->start = TachyonGet();
}


static inline unsigned
TachyonElapsedUs(Tachyon_t * tp)
{
    unsigned t = (TachyonGet() - tp->start);
    return TACHY2US(t);
}


static inline unsigned
TachyonElapsedMs(Tachyon_t * tp)
{
    unsigned t = (TachyonGet() - tp->start);
    return TACHY2MS(t);
}


static inline unsigned
TachyonElapsedSinceUs(Tachyon_t * tp)
{
    unsigned t = TachyonGet();
    unsigned tx = (t - tp->start);
    tp->start = t;
    return TACHY2US(tx);
}


static inline unsigned
TachyonElapsedSinceMs(Tachyon_t * tp)
{
    unsigned t = TachyonGet();
    unsigned tx = (t - tp->start);
    tp->start = t;
    return TACHY2MS(tx);
}


static inline bool
TachyonTimeout(Tachyon_t * tp)
{
    unsigned now = TachyonGet();
    if ((now - tp->start) > tp->delay)
        return true;
    else
        return false;
}


static inline void
TachyonDelay(unsigned us)
{
    us = US2TACHY(us);
    unsigned s = TachyonGet();
    while ((TachyonGet() - s) < us)
        ;
}

/******************************/

/*
 *  Logging.
 */
extern void         TachyonLog(int id);
extern void         TachyonLog1(int id, unsigned x1);
extern void         TachyonLog2(int id, unsigned x1, unsigned x2);
extern void         TachyonInit(void);

/**********************************************************************/

#else //  defined(OQ_DEBUG) && defined(OQ_TACHYON)

/**********************************************************************/

#define TACHY_UNIT  ((unsigned)64000000)

#define TACHY2US(t) (((unsigned)(t) * 100u) / ((TACHY_UNIT + 9999u) / 10000u))
#define US2TACHY(t) (((unsigned)(t) * ((TACHY_UNIT + 9999u) / 10000u)) / 100u)

#define TACHY2MS(t) ((unsigned)(t) / ((TACHY_UNIT + 999u) / 1000u))
#define MS2TACHY(t) ((unsigned)(t) * ((TACHY_UNIT + 999u) / 1000u))

/******************************/

typedef struct
{
    unsigned   start;          // Start time
    unsigned   delay;          // Delay time
}
    Tachyon_t;

/******************************/

static inline unsigned
TachyonGet(void)
{
    return 0;
}


static inline void
TachyonSetUs(Tachyon_t * tp, unsigned us)
{
}


static inline void
TachyonSetMs(Tachyon_t * tp, unsigned ms)
{
}


static inline void
TachyonSet(Tachyon_t * tp, unsigned tick)
{
}


static inline unsigned
TachyonElapsedUs(Tachyon_t * tp)
{
    return 0;
}


static inline unsigned
TachyonElapsedMs(Tachyon_t * tp)
{
    return 0;
}


static inline unsigned
TachyonElapsedSinceUs(Tachyon_t * tp)
{
    return 0;
}


static inline unsigned
TachyonElapsedSinceMs(Tachyon_t * tp)
{
    return 0;
}


static inline bool
TachyonTimeout(Tachyon_t * tp)
{
    return true;
}


static inline void
TachyonDelay(unsigned us)
{
}


static inline void
TachyonLog(int id)
{
}


static inline void
TachyonLog1(int id, unsigned x1)
{
}


static inline void
TachyonLog2(int id, unsigned x1, unsigned x2)
{
}


static inline void
TachyonInit(void)
{
}

/**********************************************************************/

#endif // defined(OQ_DEBUG) && defined(OQ_TACHYON)

#endif // __TACHYON_H__

/**********************************************************************/
