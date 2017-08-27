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
 *  Tachyon logging implementation.
 */
/*
 */

#if defined(OQ_DEBUG) && defined(OQ_TACHYON)

#include "debug/tachyon.h"
#include "cpu/atomic.h"

/**********************************************************************/
/*
 *  Tachyon implementation.
 */

/*
 *  Tachyon log table size (must be a power of 2)
 */
#define LOGSZ   (512)

/*
 *  A tachyon log entry
 */
typedef struct log_t	log_t;
struct log_t
{
    unsigned    time;
    unsigned    id;
    unsigned    x1, x2;
};

/*
 *  Tachyon log table, and index.  GDB will grab these symbols to extract data
 */
static log_t    TachyLog[LOGSZ];
static Atomic_t TachyIndex = { LOGSZ - 1 };
static int TachyWraps = -1;
static unsigned TachySize = LOGSZ;

/******************************/

/*
 *  Return a pointer to the next log entry.
 */
void // noinline
TachyonLog2(int id, unsigned x1, unsigned x2)
{
    /*
     *  Find a log entry.  On the off chance that we are logging an entry in
     *  an interrupt handler and non-interrupt at the same time, we will log
     *  to different entries.  (Note the logging will start at the second
     *  entry because the Atomic op below is like a "++index".)
     */
    unsigned i = AtomicAddAndReturn(&TachyIndex, 1);

    /*
     *  Restrict the index to the log buffer.
     */
    // i %= LOGSZ;
    i %= TachySize;
    if (i == 0)
        TachyWraps++;

    /*
     *  Store the log entry.
     */
    log_t * lp = &TachyLog[i];
    lp->time = TachyonGet();
    lp->id = id;
    lp->x1 = x1;
    lp->x2 = x2;
}


void
TachyonLog(int id)
{
    TachyonLog2(id, 0, 0);
}


void
TachyonLog1(int id, unsigned x1)
{
    TachyonLog2(id, x1, 0);
}


/*
 *  This function is expected to be called *once* as soon as possible
 *  during start up.  It is responsible for setting up the main 32 bit DWT
 *  cycle count timer, for use with Tachyon.
 */
void
TachyonInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**********************************************************************/

#endif // defined(OQ_DEBUG) && defined(OQ_TACHYON)
