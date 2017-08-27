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
 *  Random number generator tests.
 */


#include <stdbool.h>
#include "stdlib.h"
#include "defs.h"
#include "debug/debug.h"

/**********************************************************************/

#define RAND_BUF_SIZE 128
#define RAND_REFIL_THRESHOLD (RAND_BUF_SIZE / 4)

static struct
{
    unsigned read;
    unsigned write;
    unsigned count;
    u8       data[RAND_BUF_SIZE];
}
    randBuf =
{
    .read = 0,
    .write = 0,
    .count = 0,
    .data = {0},
};

/**********************************************************************/

static unsigned
getOne(void)
{
    if (randBuf.count == 0)
        return 0;

    u8 v = randBuf.data[randBuf.read++];
    if (randBuf.read == RAND_BUF_SIZE)
        randBuf.read = 0;

    randBuf.count -= 1;
    return v;
}

unsigned
RandGet(unsigned count)
{
    u32 v = 0;
    int c = (count-1) & 0x3;

    do
    {
        v = (v << 8) | getOne();
    } while (c-- > 0);
    return v;
}


/*
 *  Invoked by the `RandSuperLoop' to refill the buffer using the
 *  softdevice's app random vector pool.
 */
void
RandRefill(void)
{
    /*
     *  The size less the count should tell us the number of bytes missing
     *  from the random pool.  If this is 0, then we have a full buffer of
     *  juicy random bytes.  However, if we have used less than the threshold
     *  number of bytes, we early exit.  This is mostly done to prevent a query
     *  for a single number to cause this buffer to fetch more random digits
     *  from the softdevice.
     */
    unsigned available = RAND_BUF_SIZE - randBuf.count;
    if (available < RAND_REFIL_THRESHOLD)
        return;

    /*
     *  Figure out how many bytes the softdevice's random vector pool has.
     */
    u8 appAvailable = 0;
    u32 err = sd_rand_application_bytes_available_get(&appAvailable);
    (void)err; // OQ_NRF_ERROR_CHECK("sd_rand_application_bytes_available_get", err);

    /*
     *  If the softdevice's pool of random bytes is empty, we return.
     */
    if (appAvailable == 0)
        return;

    unsigned length = (appAvailable > available) ? available : appAvailable;
    unsigned n = (length < (RAND_BUF_SIZE - randBuf.write))
                         ? length : (RAND_BUF_SIZE - randBuf.write);
    unsigned rem = length - n;

    sd_rand_application_vector_get(&randBuf.data[randBuf.write], n);
    if (rem > 0)
    {
        sd_rand_application_vector_get(&randBuf.data[0], rem);
        randBuf.write = rem;
    }
    else
        randBuf.write += n;

    randBuf.count += length;
}


/*
 *  Called from the main superloop.  Ensures that we have filled our local
 *  random byte buffer.
 */
void
RandStateMachine(void)
{
    RandRefill();
}

/**********************************************************************/

#if defined(OQ_DEBUG) && defined(OQ_COMMAND)

static void
RNGTest(int argc, char ** argv)
{
    u8 capacity = 0;
    u8 available = 0;
    sd_rand_application_pool_capacity_get(&capacity);
    sd_rand_application_bytes_available_get(&available);
dprintf("cap=%d available=%d\n", capacity, available);

    u8 i;
    for (i = 0; i < 20; i++)
    {
        u32 a = RandGet(1);
        u32 b = RandGet(2);
        u32 c = RandGet(3);
        u32 d = RandGet(4);
dprintf("available=%d buffer count=%d\n", available, randBuf.count);
dprintf("random: %08x %08x %08x %08x\n", a, b, c, d);
    }
}

/**********************************************************************/

COMMAND(400)
{
    RNGTest, "RNGTest", "RN",
    "RN", "Random number generator test",
};

#endif // defined(OQ_DEBUG) && defined(OQ_COMMAND)

/**********************************************************************/
