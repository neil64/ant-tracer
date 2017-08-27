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
 *  Tempus callout implementation.
 */

#include "timer.h"
#include "debug/tachyon.h"

/**********************************************************************/

/*
 *  Callout list.
 */
static TempusCallout_t * tempusList = 0;

/**********************************************************************/

/*
 *  Schedule a callout event to run at time `tod'.
 *
 *  A callout can be changed by calling the API again with the updated
 *  time, or can be deleted before it fires by setting the time to zero.
 *
 *  NOTE:   The `time' field is used as a flag to indicate if the
 *          callout is already active.  Typically the caller will have
 *          initialized the whole TempusCallout_t structure to zero
 *          before the first call, but if not, having a falsely set
 *          `time' flag will only mean a wasted search to delete it
 *          from the list.
 */
void
TempusCallout(TempusCallout_t * tmr, unsigned tod)
{
TachyonLog1(950, tod);

    /*
     *  If the callout is already active, remove it from the callout list
     *  before inserting it again.
     */
    TempusCallout_t ** cpp = &tempusList;
    TempusCallout_t * cp = *cpp;
    if (tmr->time)
    {
        while (cp)
        {
            if (cp == tmr)
            {
                *cpp = cp->next;
                break;
            }

            cpp = &cp->next;
            cp = *cpp;
        }
    }

    /*
     *  Set the new time to expire.  And now that we know the new callout
     *  is not active, check if it is supposed to be deleted, and return
     *  if so.  Otherwise mark it as active.
     */
    tmr->time = tod;
    if (tod == 0)
        return;

    /*
     *  Insert the new callout onto the list.
     */
    cpp = &tempusList;
    cp = *cpp;
    while (cp)
    {
        if (tod < cp->time)
            break;
        cpp = &cp->next;
        cp = *cpp;
    }
    tmr->next = *cpp;
    *cpp = tmr;
}


void
TempusCalloutVar(TempusCallout_t * tmr, unsigned tod,
                 int * var)
{
    tmr->func = 0;
    tmr->data[0] = var;
    TempusCallout(tmr, tod);
}


void
TempusCalloutFunc(TempusCallout_t * tmr, unsigned tod,
                  TempusCallout_f * func, void * d0, void * d1)
{
    tmr->func = func;
    tmr->data[0] = d0;
    tmr->data[1] = d1;
    TempusCallout(tmr, tod);
}

/******************************/

/*
 *  The Super Loop Time worker -- run entries from the callout list.
 */
void
TempusMagnaCirculi(void)
{
    unsigned tod = GetTODZero();

    /*
     *  Run all entries on the top of the callout list that have
     *  expired time values.
     */
    for (;;)
    {
        TempusCallout_t * tp = tempusList;
        if (!tp)
            break;

        /*
         *  If the top entry has not expired, we are done.
         *  (The list is sorted.)
         */
        if (tod < tp->time)
            break;

        /*
         *  Remove the entry from the list now, so there won't be
         *  any confusion if the callback adds the entry again.
         */
        tempusList = tp->next;

        /*
         *  Tiggered!!
         */
        if (tp->func)
        {
            /*
             *  !!!!!!!!!!!!   CALLBACK   !!!!!!!!!!!!
             *
             *  Call the callout callback function, passing it's entry
             *  to it.  The callback function is responsible for
             *  scheduling itself again if needed, and can also
             *  schedule DSRs to be called.
             */
            (*tp->func)(tp);                    //   <---------  Callback
        }
        else
        {
            /*
             *  !!!!!!!!!!!!   TRIGGERED   !!!!!!!!!!!!
             *
             *  Set the given flag, typically used to trigger events in
             *  other parts of the super loop.
             */
            int * var = tp->data[0];
            *var = 1;
        }
    }
}

/**********************************************************************/
