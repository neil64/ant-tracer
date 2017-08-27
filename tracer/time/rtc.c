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
*   Portions of this code are based on published source code, originally      *
*   written by Neil Russell.                                                  *
*                                                                             *
\*****************************************************************************/

/*
 *  RTC "slow" timer
 *
 *  The Nordic nRF52 RTC2 device is used here to provide a slow timer tick
 *  service.  It is programmed to run at something like 1 Hz - 16 Hz,
 *  depending on the application.  It is used to manage time based
 *  services, such as sensor probing, time-of-day, and the Tempus callout
 *  timer mechanism.  It is design to use the minimal amount of CPU to save
 *  power.
 */


#include "defs.h"
#include "timer.h"
#include "debug/debug.h"
#include "debug/tachyon.h"

/**********************************************************************/

#define EIGHT_HOURS     (8*60*60)
#define HALF_HOUR       (30*60)
#define ONE_HOUR        (1*60*60)
#define QUARTER_HOUR    (15*60)

static u32 tod0;            //  Seconds since boot
static u32 tod0Frac;        //  Fractional in n/256 seconds, << 24
static u64 todOffset = (u64)EIGHT_HOURS << 32;
                            //  Offset from `tod0' to get the real TOD

static u32 todWraps;        //  # times the RTC counter has wrapped
static u32 todSecs;         //  # seconds since RTC counter was zero

static u32 hibernate;       //  The TOD0 time we should hibernate to

#if 0
  static u32 ltod0;         //  Debug for time sync
#endif // 0


/*
 *  Return the Time-of-Day, as seconds since boot.
 */
unsigned
GetTODZero(void)
{
    return tod0;
}


/*
 *  Return the Time-of-Day, as seconds since boot in the upper word and the
 *  fractional portion in 1/256 second increments in the upper byte of the
 *  lower word.
 */
u64
GetTODZero64(void)
{
    volatile u32 * ptod = &tod0;
    volatile u32 * ptodFrac = &tod0Frac;
    u32 s0, f0, s1;

    do
    {
        s0 = *ptod;
        f0 = *ptodFrac;
        s1 = *ptod;
    } while(s0 != s1);

    return ((u64)s0 << 32) | f0;
}


/*
 *  Return the time-of-day -- seconds since the 1st of January 1970 UTC.
 */
unsigned
GetTOD(void)
{
    return GetTOD64() >> 32;
}


/*
 *  Return the time-of-day.  The top 32 bits are the seconds since the
 *  1st of January 1970 UTC.  The upper byte of the lower word represents
 *  the fractional portion of the time in units of 1/256 seconds.
 */
u64
GetTOD64(void)
{
    return GetTODZero64() + todOffset;
}


/*
 *  Set the time-of-day, by recording the offset from the seconds portion of
 *  the effectively 64 bit time-of-day.  The time-of-day is only updated if
 *  either the time has not been set in a while, or if the hop count is better
 *  (or matches) our currently seen best hop count.
 */
void
SetTOD64(u64 t, u8 hopcount)
{
    todOffset = t - GetTODZero64();
}


/*
 *  Return the Time-of-Day since boot, `secs' seconds into the future
 *  (system time).
 */
unsigned
Future(unsigned secs)
{
    return tod0 + secs;
}


/*
 *  Convert a TOD into TOD0.
 */
unsigned
ConvertTOD(unsigned tod)
{
    u64 tod64 = (u64)tod << 32;

    if (tod64 >= todOffset)
    {
        tod64 -= todOffset;
        return tod64 >> 32;
    }
    else if (tod > 0)
        return 1;   //  Return almost 0
    else
        return 0;
}

/**********************************************************************/

/*
 *  Return true if the TOD has been set from an external source.
 */
int
HasTODBeenSet(void)
{
#if OQ_DEBUG
    return GetTODZero() > 15;
#endif // OQ_DEBUG
    return (todOffset > 0);
}

/******************************/

/*
 *  Update the time-of-day.  Called from the RTC interrupt.
 */
static void
updateTOD(unsigned cntr)
{
    /*
     *  We take the RTC counter and strip off the fractional part of the
     *  value to get just the number of seconds since the counter was
     *  zero.  Then if that value is less than the last time we recorded
     *  it, the counter has wrapped.  There is a ridiculously small chance
     *  that the RTC could wrap the whole 512 seconds without us getting
     *  here, which would result in a loss of 512 seconds, or more.  We
     *  don't bother dealing with that case.
     *
     *  Note that we might be getting interrupts more often than once per
     *  second, in which case the following code will produce the same
     *  result for those fractions of a second.  We ignore this, since the
     *  extra time wasted is only a little more than the code to count the
     *  partial seconds, and because this way we will automatically be
     *  synchronized by whole seconds to the RTC counter.
     *
     *  Additionally, to maintain a precise(er) time of day, we store the
     *  fractional portion of the counter in units of 1/256ths of a second.
     */
    unsigned wraps = todWraps;
    unsigned frac = (cntr % TICK_UNIT) / (TICK_UNIT/256);
    unsigned secs = cntr / TICK_UNIT;
    if (secs < todSecs)
        wraps++;
    todWraps = wraps;
    todSecs = secs;

    tod0 = (wraps * (TICK_MAX/TICK_UNIT))   //  seconds for the # of wraps
            + secs;                         //  seconds since last wrap
    tod0Frac = frac << 24;                  //  fractional in ms-byte

#if 0
{
  NRF_GPIO_Type * p0 = NRF_P0;
  unsigned x = 1 << PinAssign[PIN_LED0 + 0];
  u32 ntod = GetTOD();
  if (ltod0 == ntod)
    p0->OUTSET = x;
  else
    p0->OUTCLR = x;
  ltod0 = ntod;
}
#endif // 0

// TachyonLog2(910, cntr, tod0);
// TachyonLog2(911, wraps, secs);
}

/**********************************************************************/

/*
 *  System wide timer calls.
 *
 *  Add function calls here to run system period events, such as sensor
 *  data acquisition.  But do not be wasteful.
 */
static void
callRTCFunctions(unsigned cntr)
{
}

/**********************************************************************/

/*
 *  Callout interrupt.  Called in response to a match event on RTC 2.
 *  Look to running callout events.
 */
void
RTC2_IRQHandler(void)
{
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    NVIC_ClearPendingIRQ(RTC2_IRQn);

    /*
     *  Calculate the next timer match value.  It is rounded to a multiple
     *  of the tick rate -- if an interrupt is late on one occasion, it
     *  will be back on track on the next interrupt, and in the unlikely
     *  case that the interrupt is very late, it might skip a round (or
     *  more) and will be back on track on the next one.
     *
     *  If we are trying to hibernate, set the timer compare to the
     *  hibernate time, but no more than half the range of the RTC (512
     *  seconds; we use no more than 256 seconds).  When we get that next
     *  interrupt, the math here will all work out nicely.
     */
    unsigned cntr = NRF_RTC2->COUNTER;
    updateTOD(cntr);            //  Maintain the Time-of-Day

    unsigned incr = OQ_LFCLK / HZ;
    unsigned match;

    if (hibernate <= tod0)
    {
        match = cntr + incr;
        match -= (match % incr);
    }
    else
    {
        unsigned dly = hibernate - tod0;
        if (dly > HIBERNATE_MAX_SECONDS)
            dly = HIBERNATE_MAX_SECONDS;
        dly *= TICK_UNIT;

        match = cntr + dly;
    }

    NRF_RTC2->CC[0] = match;
TachyonLog2(900, cntr, match);

    callRTCFunctions(cntr);
}

/**********************************************************************/

/*
 *  Our selected interrupt priority.  (I just copied the priority used by
 *  Nordic's App Timer.  It could use a check to see if this is right.)
 */
#define APP_IRQ_PRIORITY_LOW    7


/*
 *  Configure the various timers that we will use.
 */
void
TimerInit(void)
{
    /*
     *  Stop RTC2, if it's somehow running.
     */
    NVIC_DisableIRQ(RTC2_IRQn);

    NRF_RTC2->EVTENCLR = 0xffffffff;
    NRF_RTC2->INTENCLR = 0xffffffff;

    NRF_RTC2->TASKS_STOP = 1;
    TimerDelaySpin(50);             //  Wait for stop to take effect

    NRF_RTC2->TASKS_CLEAR = 1;
    TimerDelaySpin(50);             //  Wait for clear

    /*
     *  Set up RTC2 to run at full speed (32 KHz), and interrupt at a
     *  relatively low priority.
     */
    NRF_RTC2->PRESCALER = 0;
    NVIC_SetPriority(RTC2_IRQn, APP_IRQ_PRIORITY_LOW);

    /*
     *  Set RTC2 running, with a compare register just before the current
     *  count.  This will give us an interrupt in about 512 seconds from
     *  now.  We expect that there will almost always be a Timer_t on the
     *  callout list, so the compare register will have a legitimate value;
     *  but in the case where it doesn't, it's better to take an interrupt
     *  that does nothing than to continually disable and enable the timer,
     *  especially with the busy wait loops.
     */
    NRF_RTC2->EVTENSET = RTC_EVTEN_COMPARE0_Msk;
    NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk;

    NVIC_ClearPendingIRQ(RTC2_IRQn);
    NVIC_EnableIRQ(RTC2_IRQn);

    NRF_RTC2->CC[0] = NRF_RTC2->COUNTER + TICK_UNIT;    //  1 second from now

    NRF_RTC2->TASKS_START = 1;
    TimerDelaySpin(50);             //  Wait for start
}

/**********************************************************************/
/*
 *  Called just before and after calling the soft device to sleep for the
 *  next event.  We use this to implement the long "hibernate" sleep.
 */
void
TimerAboutToSleep(void)
{
    NRF_RTC2->EVENTS_COMPARE[0] = 1;        //  Cause a RTC2 interrupt
}

void
TimerJustWokeUp(void)
{
    NRF_RTC2->EVENTS_COMPARE[0] = 1;        //  Cause a RTC2 interrupt
}

/******************************/

/*
 *  Cause the RTC to run in a hibernate mode until the time hibernate time
 *  `hit'.  That is, `hit' is the time relative to TOD0 we should come out
 *  of hibernation.  (Use `Future(x)'.)
 *
 *  We do this by tweaking the RTC compare register to trigger many seconds
 *  from now.  Assuming there is nothing else enabled in the system, this
 *  will cause the nRF52 to sleep for many seconds, consuming only the power
 *  needed by the RTC, 32KHz crystal and the power supply, which is
 *  something like 1 - 2 uA.  If other devices are enabled, such as other
 *  timers, the radio, serial device, etc, they will still wake up the
 *  system, nullifying the effect of the hibernation.
 *
 *  Other than keeping track of how much time we need to hibernate, we
 *  don't need any house keeping.  So long as we tweak less than 512
 *  seconds, the above TOD code will handle the extra delay on its own.
 */
void
TimerHibernate(unsigned hit)
{
    if (hit == 0 || hit > hibernate)
        hibernate = hit;
}

/**********************************************************************/

void
TimerDelaySpin(unsigned us)
{
    us *= 2;
    asm volatile (  "1: cbz %0, 2f\n"
                    "   nop; nop; nop; nop\n"
                    "   nop; nop; nop; nop\n"
                    "   nop; nop; nop; nop\n"
                    "   nop; nop; nop; nop\n"
                    "   nop; nop; nop; nop\n"
                    "   nop; nop; nop; nop\n"
                    "   nop; nop; nop\n"
                    "   subs %0, %0, #1\n"
                    "   b   1b\n"
                    "2:"
                : "+r" (us));
}

/**********************************************************************/

#ifdef OQ_COMMAND

static void
timeCmd(int argc, char ** argv)
{
    if (argc > 2)
    {
        u32 tod = GetDecimal(argv[1]);
        u8 frac = GetDecimal(argv[2]);

        u8 hops = 0;
        if (argc > 3)
            hops = GetDecimal(argv[3]);

        SetTOD64((((u64)tod) << 32) | ((u64)(frac & 0xff) << 24), hops);
    }

    dprintf("Time:  tod0 = %08x%08x\n"
            "     offset = %08x%08x\n"
            "      wraps = %d, secs = %d\n",
            tod0, tod0Frac,
            (u32)(todOffset >> 32), (u32)todOffset,
            todWraps, todSecs);
}


COMMAND(789)
{
    timeCmd, "TIME", 0,
    "print RTC debugging info",
    "time                           -   prints RTC debugging info\n"
    "time <tod> <frac> [<hopct=0>]  -   sets the time\n"
};

#endif // OQ_COMMAND

/**********************************************************************/
