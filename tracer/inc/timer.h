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
 *  Time-Of-Day, timers, delays, ...
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include "types.h"
#include "nrf.h"

/**********************************************************************/
/*
 *  Time-of-Day.
 */

/*
 *  Return the Time-of-Day, as seconds since boot.
 */
extern unsigned     GetTODZero(void);

/*
 *  Return the Time-of-Day, as seconds since boot in the upper word and the
 *  fractional portion in lower word.
 */
extern u64          GetTODZero64(void);

/*
 *  Return the Time-of-Day, as seconds since the 1st of January, 1970 UTC.
 */
extern unsigned     GetTOD(void);

/*
 *  Return the time-of-day.  The top 32-bits are the seconds since the 1st
 *  of January 1970 UTC, and fractions of a second in the lower word.
 */
extern u64          GetTOD64(void);

/*
 *  Set the time-of-day, by recording the offset from the seconds portion of
 *  the 64-bit `tod0'.  The `hopcount' specifies the number of nodes away from
 *  the source-of-time node (usually a server).
 */
extern void         SetTOD64(u64 t, u8 hopcount);

/*
 *  Return the Time-of-Day since boot, `secs' seconds into the future
 *  (system time).
 */
extern unsigned     Future(unsigned secs);

/*
 *  Convert a TOD into TOD0.
 */
extern unsigned     ConvertTOD(unsigned tod);

/*
 *  Return true if the TOD has been set from an external source.
 */
extern int          HasTODBeenSet(void);

/*
 *  Cause the RTC to run in a hibernate mode until the hibernate time
 *  `hit'.  That is, `hit' is the time relative to TOD0 we should come out
 *  of hibernation.  (Use `Future(x)'.)
 */
extern void         TimerHibernate(unsigned hit);

/**********************************************************************/
/*
 *  Medium speed timers.
 *
 *  This timer facility is based on RTC2, running at 32 KHz.  It is used
 *  here as a non-interrupt driven event timer, with a maximum total time
 *  of 512 seconds.
 */


/*
 *  The Nordic RTC timer (32,768 Hz)
 *
 *  We use RTC2 as a continuous timer, clocked at the slow clock rate and
 *  a prescale of 1 -- clocked at 32,768 Hz).  This can be used to
 *  calculate elapsed time, or to delay for small intervals, and as the
 *  basis for the callout mechanism.
 *
 *  Callers must take care not to use time values greater than 24-bits of
 *  this time (exactly 512 seconds).
 */

#define TICK_UNIT       ((unsigned)32768)       //  Ticks from the 32 KHz clock
#define TICK_MAX        (1 << 24)
#define TICK_MASK       (TICK_MAX - 1)

#define TICK2US(t)      (((unsigned)(t) * 1000000u) / TICK_UNIT)
#define US2TICK(t)      (((unsigned)(t) * TICK_UNIT) / 1000000u)

#define TICK2MS(t)      (((unsigned)(t) * 1000u) / TICK_UNIT)
#define MS2TICK(t)      (((unsigned)(t) * TICK_UNIT) / 1000u)

#define TICK2S(t)       ((unsigned)(t) / TICK_UNIT)
#define S2TICK(t)       ((unsigned)(t) * TICK_UNIT)

#define SECOND          (1)
#define MINUTE          (60 * SECOND)
#define HOUR            (60 * MINUTE)

/*
 *  WARNING: The maximum amount of time that we set the hibernation max
 *           interval to, has to be less than then `TICK_MAX' amount of
 *           seconds.
 */
#define HIBERNATE_MAX_SECONDS   (16)
#define WDT_RELOAD_SECONDS      (4)

/******************************/

typedef struct Timer_t Timer_t;
struct Timer_t
{
    unsigned    start;      // Start time
    unsigned    delay;      // Delay time
};

/******************************/

static inline unsigned
TimerGetTick(void)
{
    return NRF_RTC2->COUNTER;
}


static inline void
TimerSetMs(Timer_t * tp, unsigned ms)
{
    tp->delay = MS2TICK(ms);
    tp->start = TimerGetTick();
}


static inline void
TimerSetUs(Timer_t * tp, unsigned us)
{
    if (us > ((1<<31) / TICK_UNIT))
        tp->delay = MS2TICK(us / 1000);
    else
        tp->delay = US2TICK(us);

    tp->start = TimerGetTick();
}


static inline void
TimerSetTick(Timer_t * tp, unsigned tick)
{
    tp->delay = tick;
    tp->start = TimerGetTick();
}


static inline unsigned
TimerElapsedMs(Timer_t * tp)
{
    unsigned t = (TimerGetTick() - tp->start) & TICK_MASK;
    return TICK2MS(t);
}


static inline unsigned
TimerElapsedSinceMs(Timer_t * tp)
{
    unsigned t = TimerGetTick();
    unsigned tx = (t - tp->start) & TICK_MASK;
    tp->start = t;
    return TICK2MS(tx);
}


static inline bool
TimerTimeout(Timer_t * tp)
{
    unsigned now = TimerGetTick();
    if (((now - tp->start) & TICK_MASK) > tp->delay)
        return true;
    else
        return false;
}


static inline bool
TimerRunning(Timer_t * tp)
{
    return tp->delay > 0;
}


static inline void
TimerReset(Timer_t * tp)
{
    tp->delay = 0;
}

/******************************/

extern void     TimerInit(void);
extern void     TimerDelaySpin(unsigned us);
extern void     TimerAboutToSleep(void);
extern void     TimerJustWokeUp(void);

/**********************************************************************/
/*
 *  Tempus.  Slow speed timer.
 *
 *  This timer is a non-interrupt driven timer based on the Time-Of-Day
 *  since boot (TOD0), with an unlimited maximum time (many years).
 */

typedef struct Tempus_t Tempus_t;
struct Tempus_t
{
    unsigned    time;       // Trigger time
};


static inline void
TempusSet(Tempus_t * t, unsigned tod)
{
    t->time = tod;
}


static inline int
TempusTimeout(Tempus_t * t)
{
    if (GetTODZero() >= t->time)
        return true;
    else
        return false;
}

/**********************************************************************/
/*
 *  The Tempus callout facility.
 *
 *  These timers are a callout mechanism implemented entirely in the App
 *  context.  They have a resolution of 1 Hz, and many years of maximum.
 */

typedef struct TempusCallout_t TempusCallout_t;
typedef void TempusCallout_f(TempusCallout_t * t);
struct TempusCallout_t
{
    unsigned            time;       // Trigger time
    TempusCallout_t *   next;       // Next in callout list
    TempusCallout_f *   func;       // Callout function
    void *              data[2];    // Callout data store
};

extern void     TempusMagnaCirculi(void);

extern void	TempusCallout(TempusCallout_t * t, unsigned tod);
extern void	TempusCalloutVar(TempusCallout_t * t, unsigned tod,
                                int * var);
extern void	TempusCalloutFunc(TempusCallout_t * t, unsigned tod,
                                  TempusCallout_f * func, void * d0, void * d1);

/**********************************************************************/

#endif // __TIMER_H__

/**********************************************************************/
