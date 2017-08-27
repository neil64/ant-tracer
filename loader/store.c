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
 *  Flash storage management, boot loader style.
 *
 *  This is a stripped version of the flash storage management code from
 *  the main image.  I can only read the flash storage area.  It's used
 *  only to read a software update image from flash, which is then used to
 *  reprogram the code space.
 */

/**********************************************************************/

#include "bl.h"


/*
 *  Record types, and related.
 */
enum
{
    RT_ZERO = 0,            //  Zero record -- no operation
    RT_CONFIG = 0x01,       //  OttoQ configuration structure
    RT_SLOG = 0x08,         //  Sensor log record
    RT_SU_HDR = 0x0c,       //  Software update header
    RT_SU_DATA = 0x0d,      //  Software update chunk
    RT_SU_EXEC = 0x0e,      //  Software update execute
    RT_SEQUENCE = 0x1f,     //  Storage manager sequence record

    RT_MASK = 0x1f,         //  Mask of the record type data
    RT_SHIFT = 26,          //  Record type bit position

    RD_MASK = 0x03ffffff,   //  Record data mask
};

/*
 *  Operation flags.
 */
enum
{
    OF_CONFIG = 0x01,           //  Saved configuation
    OF_SENSOR_DATA = 0x02,      //  Store sensor log data
    OF_SW_UPDATE = 0x04,        //  Software update version info
    OF_SW_CHUNK = 0x08,         //  Software update data chunk
    OF_SW_EXEC = 0x10,          //  Software update execute
};

/**********************************************************************/
/*
 *  Flash write record formatter, page manager and flash write.
 */

/*
 *  Flash access staging area, and content details.  (It must contain
 *  enough space for the largest record that we could have.)
 *
 *  The number of used words in this buffer is `bufferIndex + 1'.  An
 *  index of greater than maximum is an indication that the buffer does
 *  not contain anything.  bufferPointer is the number of bits remaining
 *  in the current word (LSB bits).  `bufferLimit' is the number of words
 *  provided in a read/import.
 */
static u32      buffer[40];
static unsigned bufferIndex = ARRAY_SIZE(buffer);
static unsigned bufferLimit = 0;
static unsigned bufferPointer;

/**********************************************************************/

static u32
rd(unsigned bits)
{
    unsigned idx = bufferIndex;
    unsigned ptr = bufferPointer;
    unsigned ins = 0;
    u32 data = 0;

    while (bits > ptr)
    {
        if (ptr > 0)
        {
            u32 msk = (1 << ptr) - 1;

            data |= (buffer[idx] & msk) << ins;

            ins += ptr;
            bits -= ptr;
        }

        idx++;
        if (idx >= bufferLimit)
        {
            idx = 0;
            bufferLimit = 0;
        }
        bufferIndex = idx;
        ptr = 31;
    }

    ptr -= bits;
    u32 msk = (1 << bits) - 1;
    u32 x = (buffer[idx] >> ptr) & msk;
    data |= x << ins;

    bufferPointer = ptr;

    return data;
}


static void
importRecord(unsigned ops)
{
    SuInfo_t sui;
    SuData_t sud;
    SuExec_t sux;

    /*
     *  Extract the record type from the first word, then set up for
     *  the read.
     */
    u32 rec = buffer[0];
    unsigned ty = (rec >> RT_SHIFT) & RT_MASK;

    bufferLimit = bufferIndex + 1;
    bufferIndex = 0;
    bufferPointer = RT_SHIFT;

    /*
     *  Read the appropriate data.
     */
    switch (ty)
    {
    case RT_ZERO:               //  Zero record -- no operation
    case RT_SEQUENCE:           //  Storage manager sequence record
    case RT_CONFIG:             //  OttoQ configuration structure
    case RT_SLOG:               //  Sensor log record
    default:
        break;

    case RT_SU_HDR:            //  Software update header
        sui.sequence = rd(20);                         //  Sequence #
        sui.version = rd(32);                          //  Version
        sui.start = rd(14) << OQ_SU_CHUNK_SHIFT;      //  Start address
        sui.end = rd(14) << OQ_SU_CHUNK_SHIFT;        //  End address
        break;

    case RT_SU_DATA:           //  Software update chunk
        {
            sud.sequence = rd(20);     //  Sequence #
            sud.address = rd(13) << OQ_SU_CHUNK_SHIFT;
            for (int i = 0; i < OQ_SU_CHUNK; i++)
                sud.data[i] = rd(8);
        }
        break;

    case RT_SU_EXEC:           //  Software update execute
        sux.sequence = rd(20);                         //  Sequence #
        sux.version = rd(32);                          //  Version
        break;
    }

    /*
     *  Check for read errors.
     */
    if (bufferLimit == 0)
        return;                 //  The stored record was too small;  ignore it

    /*
     *  Pass the data to the appropriate handler.
     */
    switch (ty)
    {
    case RT_ZERO:               //  Zero record -- no operation
    case RT_SEQUENCE:           //  Storage manager sequence record
    case RT_CONFIG:             //  OttoQ configuration structure
    case RT_SLOG:               //  Sensor log record
    default:
        break;

    case RT_SU_HDR:            //  Software update header
        if (ops & OF_SW_UPDATE)
            StoreCallbackSoftwareUpdateInfo(&sui);
        break;

    case RT_SU_DATA:           //  Software update chunk
        if (ops & OF_SW_CHUNK)
            StoreCallbackSoftwareUpdateChunk(&sud);
        break;

    case RT_SU_EXEC:           //  Software update execute
        if (ops & OF_SW_EXEC)
            StoreCallbackSoftwareUpdateExecute(&sux);
        break;
    }
}


static void
importWord(unsigned ops, u32 record)
{
    if (record & 0x80000000)
    {
        /*
         *  We have a new record, hopefully.  Pass it up.
         */
        if (bufferIndex < ARRAY_SIZE(buffer))
            importRecord(ops);

        /*
         *  If this is an "erased" word, ignore it, leaving the buffer
         *  empty.
         */
        if (record == 0xffffffff)
        {
            bufferIndex = ARRAY_SIZE(buffer);
            return;
        }

        /*
         *  A legitimate start of record.  Store the first.  (Note that
         *  `bufferIndex' points to the word we just wrote, rather than
         *  the word beyond as typical C does.)
         */
        buffer[0] = record;
        bufferIndex = 0;
        return;
    }

    /*
     *  Add the new data to the buffer.
     */
    if (bufferIndex < ARRAY_SIZE(buffer) - 1)
        buffer[++bufferIndex] = record;
}


static void
importReset(unsigned ops)
{
    bufferIndex = ARRAY_SIZE(buffer);
}

/**********************************************************************/

/*
 *  Read the entire flash storage, parse records, and deliver them to
 *  interested parties.  As a side effect, record the flash bounds, and
 *  any other things of interest.
 */
static void
readFlash(unsigned ops)
{
    /*
     *  Scan the entire flash storage region to find the oldest and newest
     *  used pages.
     */
    u32 * oldp = 0;
    u32 oldq = 0xffffffff;
    u32 * newp = 0;
    u32 newq = 0;

    u32 * page;
    for (page = (u32 *)StoreStart;
         page < (u32 *)StoreEnd;
         page += (OQ_FLASH_PAGE / sizeof *page))
    {
        /*
         *  Scan records in this page, looking for a sequence record.
         */
        u32 * erp = page + (OQ_FLASH_PAGE / sizeof *page);
        u32 * rp = page;
        while (rp < erp)
        {
            u32 rec = *rp++;
            if (rec == 0xffffffff)
                goto cont0;         //  The page is erased from here on

            if (!(rec & 0x80000000))
                continue;           //  Skip continuation records

            unsigned ty = (rec >> RT_SHIFT) & RT_MASK;  //  Grab the record type
            if (ty == RT_SEQUENCE)
            {
                /*
                 *  Got the sequence record.  Grab it's info.
                 */
                unsigned seq = rec & RD_MASK;
                if (seq < oldq)
                {
                    oldq = seq;
                    oldp = page;
                }
                if (seq > newq)
                {
                    newq = seq;
                    newp = page;
                }

                goto cont0;
            }
        }

  cont0:;
    }

    /*
     *  If we found at least one used page, process ...
     */
    if (newp)
    {
        /*
         *  Reset the up stream handlers.
         */
        importReset(ops);

        /*
         *  Scan records in all pages from oldest to newest, applying
         *  their data.
         */
        page = oldp;
        for (;;)
        {
            if (page >= (u32 *)StoreEnd)
                page = (u32 *)StoreStart;

            /*
             *  Scan records in this page.
             */
            u32 * erp = page + (OQ_FLASH_PAGE / sizeof *page);
            u32 * rp = page;
            while (rp < erp)
            {
                u32 rec = *rp++;
                importWord(ops, rec);   //  Add to accumulated data

                if (rec == 0xffffffff)
                    break;              //  End of records in page
            }

            /*
             *  If we just finished processing the newest page, we are done.
             *  We need to inject a single "erased" word into the stream to
             *  help the handlers finish what they are doing.
             */
            if (page == newp)
            {
                importWord(ops, 0xffffffff);
                break;
            }

            /*
             *  Next...
             */
            page += (OQ_FLASH_PAGE / sizeof *page);
        }

        /*
         *  We have read everything we are interested in.  Now set up
         *  to continue to write from where we left off.
         */
        u32 * xp = newp + (OQ_FLASH_PAGE / sizeof *page);
        while (xp > newp)
        {
            u32 * xp1 = xp - 1;
            if (*xp1 != 0xffffffff)
                break;
            xp = xp1;
        }
    }
}

/**********************************************************************/

/*
 *  Search the flash storage for a software update record, and software
 *  update chunks, and call back with that information.
 */
void
StoreRead(void)
{
    readFlash(OF_SW_UPDATE | OF_SW_CHUNK | OF_SW_EXEC);
}

/**********************************************************************/
