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
 *  Flash storage management.
 *
 *
 *  The flash storage handler manages a contiguous region of flash for log
 *  style storage.  The region is organized as a circular buffer
 *  containing storage records.  (On the nRF52, a page is an erasable unit
 *  of 4096 bytes;  a block is a 512 byte piece of a page;  a record is a
 *  item of storage in this software.)
 *
 *  Since the region is written as a circular buffer, it implements very
 *  simple wear leveling.  The Nordic nRF52 allows for 10,000 erase cycles
 *  per page, which gives us 60,000 page updates for the life time of the
 *  product (assuming a 60 page region, with a page size of 4k bytes).
 *
 *  On system start up, the entire region is read to determine the most
 *  recent information.  Record handlers take care of determining the most
 *  recent of a particular record type.  The storage manager will
 *  determine the most recent page, and continue writing from there.
 *
 *  The region is organized as follows:
 *
 *      +---------------+ 0x40000   (start of region)
 *      |    page 0     |
 *      +---------------+ 0x41000
 *      |    page 1     |
 *      +---------------+ 0x42000
 *      |               |
 *      |  | | | | | |  |
 *      |               |
 *      +---------------+
 *      |    page 58    |
 *      +---------------+ 0x7b000   (last page)
 *      |    page 59    |
 *      +---------------+ 0x7c000   (end of region + 1)
 *      |     ....      |
 *      +---------------+ 0x7d000
 *      |   BLE conf    |
 *      +---------------+ 0x7e000
 *      |   BLE conf    |
 *      +---------------+ 0x7f000
 *      |  boot loader  |
 *      +---------------+ 0x80000   (end of flash)
 *
 *  Records are a collection of words.  The first word has its MSB set
 *  to 1, contains a 5-bit record type, and 26-bits of payload.  Subsequent
 *  words have their MSB set to 0, and contain 31-bits of payload.  The
 *  first word has a layout as follows:
 *
 *      31   30        26                                  0
 *      +--------------------------------------------------+
 *      |  1  |  TTTTT  |             payload              |
 *      +--------------------------------------------------+
 *
 *  Subsequent words have the following layout:
 *
 *      31   30                                            0
 *      +--------------------------------------------------+
 *      |  0  |                  payload                   |
 *      +--------------------------------------------------+
 *
 *  All pages must contain a sequence record, which has a type of 0x1f
 *  (all ones).  The sequence record will be located after any continuation
 *  words from a record in the previous page.
 *
 *  A storage management record is used to mark a page with a sequence
 *  number, which is used on start up to find the most recently written
 *  page.  Sequence numbers increase by 1 for each new page.  Given the
 *  permitted 10,000 erase cycles for each page, we get about 60,000 total
 *  page erases.  That needs 16-bits of sequence numbers.  We have 26
 *  bits.  We will never wrap sequence numbers before the device fails.
 *
 *  Flash writes are assumed to be atomic operations;  that is, a word
 *  write is either completed by the hardware or fails.  This may or may
 *  not be true, but we ignore that in this implementation.
 */

/**********************************************************************/

/*
 *  Available space for storage words (26-bit):
 *
 *       1   26 (0)
 *       2   57 (31)
 *       3   88 (62)
 *       4  119 (93)                This was just a note for myself to
 *       5  150 (124)               help figure the size of a record once
 *       6  181 (155)               stored.  The first number is the count
 *       7  212 (186)               of words for a record, the second is
 *       8  243 (217)               the total number of bits in the record,
 *       9  274 (248)               and the third is the number of bits in
 *      10  305 (279)               all but the first word.
 *      11  336 (310)
 *      12  367 (341)
 *      13  398 (372)
 *      14  429 (403)
 *      15  460 (434)
 *      16  491 (465)
 *      17  522 (496)
 *      18  553 (527)
 *      19  584 (558)
 *      20  615 (589)
 *      21  646 (620)
 *      22  677 (651)
 *      23  708 (682)
 *      24  739 (713)
 *      25  770 (744)
 *      26  801 (775)
 *      27  832 (806)
 *      28  863 (837)
 *      29  894 (868)
 *      30  925 (899)
 *      31  956 (930)
 *      32  987 (961)
 *      33 1018 (992)
 *      34 1049 (1023)
 *      35 1080 (1054)
 *      36 1111 (1085)
 *      37 1142 (1116)
 *      38 1173 (1147)
 *      39 1204 (1178)
 */

/**********************************************************************/

#include <stdbool.h>
#include "defs.h"
#include "timer.h"
#include "stdlib.h"
#include "store/store.h"
#include "store/config.h"
#include "debug/debug.h"

#include "nrf52.h"

/*
 *  Record types, and related.
 */
enum
{
    RT_ZERO = 0,            //  Zero record -- no operation
    RT_CONFIG = 0x01,       //  OttoQ configuration structure
    // RT_SLOG = 0x08,         //  Sensor log record
    RT_SU_HDR = 0x0c,       //  Software update header
    RT_SU_DATA = 0x0d,      //  Software update chunk
    RT_SU_EXEC = 0x0e,      //  Software update "execute"
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
    // OF_SENSOR_DATA = 0x02,      //  Store sensor log data
    OF_SW_UPDATE = 0x04,        //  Software update version info
    OF_SW_CHUNK = 0x08,         //  Software update data chunk
    OF_SW_EXEC = 0x10,          //  Software update execute
};
static unsigned     opFlag;

/*
 *  Configuration save timer.
 */
Tempus_t configSaveTimer;
int configSaving;

/*
 *  Staging area.  Note that this staging area is used for both reading
 *  and writing of flash.  Callers must ensure that both are not attempted
 *  at the same time.
 */
struct
{
    unsigned        sequence;
    Config_t        config;

    union
    {
        SuInfo_t    sui;
        SuData_t    sud;
        SuExec_t    sux;
    };
}
    staging;

/**********************************************************************/
/*
 *  Flash protect / un-protect operations.
 */

/*
 *  Clear the memory watch unit (MWU) region watch.
 */
static inline u32
flashRegionEnClear(void)
{
    u32 v = NRF_MWU->REGIONEN;
    NRF_MWU->REGIONENCLR = v;

    // while (NRF_MWU->REGIONEN != 0)
    //     ;
#if !defined(OQ_TESTING)
    asm volatile ("dsb; nop; nop; nop");
    asm volatile ("nop; nop; nop; nop");
#endif // !defined(OQ_TESTING)

    return v;
}


/*
 *  Restore the memory watch unit's region watch.
 */
static inline void
flashRegionEnSet(u32 v)
{
#if !defined(OQ_TESTING)
    asm volatile ("dsb; nop; nop; nop");
    asm volatile ("nop; nop; nop; nop");
#endif // !defined(OQ_TESTING)

    NRF_MWU->REGIONENSET = v;
    // while (NRF_MWU->REGIONEN != v)
    //     ;
}

/**********************************************************************/
/*
 *  Flash operations.
 */

/*
 *  Flash operation in progress.  `flashOp' is the operation, `flashTries'
 *  is the number of remaining retries, `flashComplete' is set when the
 *  operation completes successfully, and the other values allow the
 *  operation to be restarted, if needed.  The Nordic soft device flash
 *  operations can fail if there is not a large enough time slot in the
 *  near future for the operation to complete before the next radio event.
 *  We cannot tolerate a failed operation, so after trying a few times, we
 *  will just write directly to the flash, which will always work, but
 *  this can cause the soft device to get unhappy.  Oh well.
 */
u8    flashComplete;
u8    flashWriteOp;
u8    flashEraseOp;
s8    flashTries;
u32 * flashAddress;
u32 * flashSource;
u32   flashCount;


/*
 *  Wait for the previous flash operation to complete.  (This may not be
 *  strictly necessary, since the CPU stops for any flash operation, and
 *  so by the time we get to check this, it will no longer be busy.)
 */
static inline void
flashWaitRaw(void)
{
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        ;
}


/*
 *  Erase the flash page at the given address.
 */
static void
flashEraseRaw(u32 * addr)
{
    /*
     *  Clear the regions that the MWU is protecting.
     */
    u32 prev = flashRegionEnClear();

    /*
     *   Turn on the flash erase enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);
    flashWaitRaw();

    /*
     *  Erase the page.
     */
    NRF_NVMC->ERASEPAGE = (u32)addr & ~(OQ_FLASH_PAGE - 1);
    flashWaitRaw();

    /*
     *  Turn off the flash erase enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    flashWaitRaw();

    /*
     *  Reset the regions that the MWU is protecting.
     */
    flashRegionEnSet(prev);
}


/*
 *  Write a block of words to flash.  The start address must be word aligned,
 *  and the count an even multiple of words.
 */
static void
flashWriteRaw(u32 * dst, u32 * src, unsigned words)
{
    /*
     *  Clear the regions that the MWU is protecting.
     */
    u32 prev = flashRegionEnClear();

    /*
     *  Turn on the flash write enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    flashWaitRaw();

    /*
     *  Loop over the block, writing.
     */
    while (words-- >= 1)
    {
        *dst++ = *src++;
        flashWaitRaw();
    }

    /*
     *  Turn off the flash write enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    flashWaitRaw();

    /*
     *  Reset the regions that the MWU is protecting.
     */
    flashRegionEnSet(prev);
}

/******************************/

/*
 *  Schedule a flash erase operation.
 */
static void NOINLINE
flashErase(u32 * addr)
{
    flashComplete = false;
    flashEraseOp = true;
    flashWriteOp = false;
    flashTries = 4;
    flashAddress = addr;

    sd_flash_page_erase((u32)addr / OQ_FLASH_PAGE);
}


/*
 *  Schedule a flash write operation.
 */
static void NOINLINE
flashWrite(u32 * dst, u32 * src, unsigned cnt)
{
    flashComplete = false;
    flashEraseOp = false;
    flashWriteOp = true;
    flashTries = 4;
    flashAddress = dst;
    flashSource = src;
    flashCount = cnt;

    sd_flash_write((uint32_t *)dst, (uint32_t *)src, cnt);
}


/*
 *  Called when the soft device completes a flash operation.  `ok' is true
 *  if the operation was successful.
 */
void
StoreFlashed(int ok)
{
    if (ok)
    {
        /*
         *  All is well.
         */
        flashComplete = true;
        return;
    }

    if (flashComplete)
    {
        /*
         *  Huh?  How did we get here?
         */
        return;
    }

    /*
     *  Try the operation again.
     */
    if (flashTries > 0)
    {
        flashTries--;

        /*
         *  Try the soft device again, for good measure.
         */
        if (flashEraseOp)
            sd_flash_page_erase((u32)flashAddress / OQ_FLASH_PAGE);
        if (flashWriteOp)
            sd_flash_write((uint32_t *)flashAddress,
                           (uint32_t *)flashSource,
                           flashCount);
    }
    else
    {
        /*
         *  The soft device failed us too many times.  When you want
         *  something done properly, do it yourself!
         */
        if (flashEraseOp)
        {
            dprintf("RAW ERASE - ");
            flashEraseRaw(flashAddress);
            dprintf("DONE\n");
        }
        if (flashWriteOp)
        {
            dprintf("RAW WRITE - ");
            flashWriteRaw(flashAddress, (u32 *)flashSource, flashCount);
            dprintf("DONE\n");
        }
        flashComplete = true;
    }
}

/**********************************************************************/
/*
 *  Flash write record formatter, page manager and flash write.
 */

/*
 *  Flash page info.
 */
static u32 *    currentWrite;       //  Current write pointer for new records
static unsigned sequence;           //  Next sequence number

static u32 *    newCurrentWrite;    //  Current write pointer candidate
static unsigned newSequence;        //  Sequence number candidate

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

#if OQ_DEBUG
static u32 * storeDebugOldp = 0;
static u32 storeDebugOldq = 0xffffffff;
static u32 * storeDebugNewp = 0;
static u32 storeDebugNewq = 0;
static int storeDebugFlag = false;
static void printStoreRecord(int ty);
#endif // OQ_DEBUG

/******************************/

/*
 *  Start a new storage record in the staging buffer.  Add the type.
 */
static void
wrNew(unsigned ty)
{
    ty &= RT_MASK;
    buffer[0] = 0x80000000 | (ty << RT_SHIFT);
    bufferIndex = 0;
    bufferPointer = RT_SHIFT;
}


/*
 *  Write `bits' of new `data' into the staging area, ready to be written
 *  to flash.
 */
static void
wr(unsigned bits, u32 data)
{
    unsigned idx = bufferIndex;
    unsigned ptr = bufferPointer;

    while (bits > ptr)
    {
        if (ptr > 0)
        {
            u32 msk = (1 << ptr) - 1;

            buffer[idx] = (buffer[idx] & ~msk) |
                          (data & msk);

            data >>= ptr;
            bits -= ptr;
        }

        if (idx < ARRAY_SIZE(buffer) - 1)
            idx++;
        bufferIndex = idx;
        buffer[idx] = 0;
        ptr = 31;
    }

    u32 msk = (1 << ptr) - 1;
    ptr -= bits;
    data <<= ptr;

    buffer[idx] = (buffer[idx] & ~msk) |
                  (data & msk);

    bufferPointer = ptr;
}


/*
 *  Append a sequence number record to the buffer.
 */
static void
wrSeq(void)
{
    unsigned idx = bufferIndex;
    if (++idx < ARRAY_SIZE(buffer))
    {
        u32 s = 0x80000000 |
                (RT_SEQUENCE << RT_SHIFT) |
                (++sequence & RD_MASK);
        buffer[idx] = s;
        bufferIndex = idx;
    }
}

/******************************/

/*
 *  Start a flash write, with the data we've just collected.  It manages
 *  paging correctly.  Returns true if there was a flash operation.  If it
 *  returns true, the flash state machine should arrange to call it again
 *  once the flash operation is complete to possibly start the next step
 *  of a write, and should not move on to the next until it returns false.
 *  (Worst case, `wrGo()' could need three flash operations -- an erase,
 *   and two writes.)
 */
static int
wrGo(void)
{
    /*
     *  The flash write state machine.
     */
    static enum
    {
        IDLE = 0,
        ERASE1, ERASE2, ERASE3,     //  Erase state, including retries
        WRITE1, WRITE2, WRITE3,     //  Write state, including retries
                    //  XXX   These retry states can probably be removed now
    }
        state = IDLE;
    static bool needSeq = false;

    /*
     *  Set up our collection of values.  We collect:
     *  -   the next flash write location
     *  -   the start of the current page
     *  -   the end of this page + 1
     *  -   the word count remaining in this page
     *  -   the number of words we need to write for this record
     */
    u32 * ptr = currentWrite;
    u32 * page = (u32 *)((unsigned)ptr & ~(OQ_FLASH_PAGE - 1));
    u32 * pe = (u32 *)((unsigned)page + OQ_FLASH_PAGE);
    unsigned remain = pe - ptr;
    unsigned cnt = bufferIndex + 1;

    /*
     *  If the write pointer is at the very beginning of the page, then we
     *  have just finished filling the last page exactly, and have not yet
     *  touched the next page.  So we need to treat the pointer as if it
     *  is within the last page.
     */
    if (ptr == page)
    {
        pe = page;
        page = (u32 *)((unsigned)page - OQ_FLASH_PAGE);
        if (page < (u32 *)OQ_FLASH_STORE)
            page = (u32 *)(OQ_FLASH_STORE_END - OQ_FLASH_PAGE);
        remain = 0;
    }

    /*
     *  Run the state machine.
     */
    switch (state)
    {
    case IDLE:
        /*
         *  If there is nothing to write, we are done.  (An excessive
         *  count is the indication of an invalid record.)
         */
        if (cnt > ARRAY_SIZE(buffer))
            return 0;

        /*
         *  Start with the page erase state.  (Most of the time, it'll
         *  go right through that state to write.)
         */
        state = ERASE1;
        goto erase;

    case ERASE1:
        state = ERASE2;
        goto erase;

    case ERASE2:
        state = ERASE3;
        goto erase;

    erase:
        if (cnt > remain)
        {
            /*
             *  Figure the address of the next page to erase/write.
             */
            u32 * next = pe;    //  The next page to write, wrapping as needed
            if (next >= (u32 *)OQ_FLASH_STORE_END)
                next = (u32 *)OQ_FLASH_STORE;

            /*
             *  We have more to write than there are words remaining in
             *  this page.  We need another page.  If the next page is not
             *  erased, we first need to erase it.  (Since we always write
             *  from start to finish, if the first word is erased, we can
             *  assume the whole page is erased.)
             */
            if (*next != 0xffffffff)
            {
                /*
                 *  Trigger a page erase.  The soft device will let us
                 *  know later when it is done.
                 */
                flashErase(next);

                /*
                 *  Have the flash state machine come back to us until
                 *  we're done.
                 */
                return 1;
            }

            /*
             *  We needed a new page, and we have one.  In the normal
             *  case, we can just write from the current pointer in the
             *  current page over the page boundary into the new page.
             *  But, if the next page is actually the first page
             *  (wrapped), that ain't gunna work.  We take the easy way
             *  out, and just discard the remaining words in the current
             *  page, and write from the start of the new page.
             */
            if (ptr > next)         //  Skip the remaining words, and start
            {
                ptr = next;         //  ... at the bottom of the new page
                currentWrite = ptr;
            }

            /*
             *  Note that we need to write a new sequence record for the
             *  new page into the write stream.
             */
            needSeq = true;
        }
        state = WRITE1;
        goto write;

    case WRITE1:
        state = WRITE2;
        goto write;

    case WRITE2:
        state = WRITE3;
        goto write;

    write:
        /*
         *  Write the record to flash, if it's not already there.  (The
         *  assumption here is that if the first word is no longer erased,
         *  then the write succeeded.  So long as the soft device writes
         *  the whole record, or nothing, that'll work.  If not, there is
         *  nothing we can do.)
         */
        if (*ptr != 0xffffffff)
        {
            /*
             *  The write completed;  update the pointer and go home.
             */
            ptr += cnt;
            currentWrite = ptr;
            goto done;
        }

        /*
         *  If we are about to write a new page, we need to make sure that
         *  a sequence record appears in that page.
         */
        if (needSeq)
        {
            needSeq = false;
            wrSeq();
            cnt = bufferIndex + 1;
        }

#if 0
if (0)
{
 dprintf("flash write: %x =", (uint32_t *)ptr);
 for (int i = 0; i < cnt; i++)
  dprintf(" %08x", buffer[i]);
 dprintf("\n");
}
#endif // 0

        /*
         *  Write the entire record to flash in one operation.  This may
         *  straddle a page in some cases.
         */
        flashWrite(ptr, &buffer[0], cnt);

        /*
         *  Have the flash state machine come back to us until we're done.
         */
        return 1;

    case ERASE3:
    case WRITE3:
    done:
    default:
        /*
         *  Either we tried three times to write/erase and failed, or we
         *  finished successfully.  Wrap it up.
         */
        bufferIndex = ARRAY_SIZE(buffer);       //  Mark as empty
        state = IDLE;                           //  Back to the start
        return 0;
    }
}

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
    default:
        break;

    case RT_SEQUENCE:           //  Storage manager sequence record
        staging.sequence = rd(RT_SHIFT);
        break;

    case RT_CONFIG:             //  OttoQ configuration structure
        for (int i = 0; i < CONFIG_WORDS; i++)
            staging.config.confArray[i] = rd(32);
        break;

    case RT_SU_HDR:             //  Software update header
        {
            SuInfo_t * si = &staging.sui;
            si->sequence = rd(20);                     //  Sequence #
            si->version = rd(32);                      //  Version
            si->start = rd(14) << OQ_SU_CHUNK_SHIFT;   //  Start address
            si->end = rd(14) << OQ_SU_CHUNK_SHIFT;     //  End address
        }
        break;

    case RT_SU_DATA:            //  Software update chunk
        {
            SuData_t * sd = &staging.sud;
            sd->sequence = rd(20);
            sd->address = rd(13) << OQ_SU_CHUNK_SHIFT;
            for (int i = 0; i < OQ_SU_CHUNK; i++)
                sd->data[i] = rd(8);
        }
        break;

    case RT_SU_EXEC:            //  Software update "execute"
        {
            SuExec_t * si = &staging.sux;
            si->sequence = rd(20);                     //  Sequence #
            si->version = rd(32);                      //  Version
        }
        break;
    }

#if OQ_DEBUG
    printStoreRecord(ty);
#endif // OQ_DEBUG

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
    default:
        break;

    case RT_CONFIG:             //  OttoQ configuration structure
        /*
         *  Check the CRC of the record.
         */
        if (ops & OF_CONFIG)
        {
            if (staging.config.confCRC == ConfigCRC(&staging.config))
            {
                /*
                 *  The stored CRC was correct.  Pass this up.
                 */
                StoreCallbackConfiguration(&staging.config);
            }
        }
        break;

    case RT_SU_HDR:             //  Software update header
        if (ops & OF_SW_UPDATE)
            ; // StoreCallbackSoftwareUpdateInfo(&staging.sui);
        break;

    case RT_SU_DATA:            //  Software update chunk
        if (ops & OF_SW_CHUNK)
            ; // StoreCallbackSoftwareUpdateChunk(&staging.sud);
        break;

    case RT_SU_EXEC:            //  Software update "execute"
        if (ops & OF_SW_EXEC)
            ; // StoreCallbackSoftwareUpdateExecute(&staging.sux);
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
    for (page = (u32 *)OQ_FLASH_STORE;
         page < (u32 *)OQ_FLASH_STORE_END;
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

#if OQ_DEBUG
    storeDebugOldp = oldp;
    storeDebugOldq = oldq;
    storeDebugNewp = newp;
    storeDebugNewq = newq;
#endif // OQ_DEBUG

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
            if (page >= (u32 *)OQ_FLASH_STORE_END)
                page = (u32 *)OQ_FLASH_STORE;

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

        /*
         *  We have what we want -- the current page is the page with the
         *  highest sequence number, and the current write pointer is the
         *  next word to be written to storage.  The current pointer
         *  should never be less than the 2nd word in a page, but may be
         *  the word beyond the last word of the page.  Beyond the last is
         *  ok, in which case we will create a new page on the next write.
         */
        newCurrentWrite = xp;
        newSequence = newq;
    }
    else
    {
        /*
         *  We found nothing -- set up for an empty flash.
         */
        newCurrentWrite = 0;
        newSequence = 0;
    }
}

/**********************************************************************/

static enum
{
    INIT0 = 0,          //  System start up -- sift through circular flash
    IDLE,               //  Waiting for an operation
    WRITING,            //  Flash write started, waiting for result

    CONFIG0,            //  Saving configuration
    CONFIG1,
    CONFIG2,
    CONFIG3,

    SU_HDR_0,           //  Storing a software update header
    SU_DATA_0,          //  Storing a software update data chunk
    SU_EXEC_0,          //  Storing a software update execute record
}
    state = INIT0, savedState;


int
StoreSuperLoop(void)
{
    /*
     *  Switch for the current state.
     */
  again:
    switch (state)
    {
    case INIT0:
        /*
         *  Read the flash storage, extracting the configuration, if it exists.
         */
        readFlash(OF_CONFIG | OF_SW_UPDATE | OF_SW_CHUNK | OF_SW_EXEC);

        /*
         *  Get ready to write to flash.
         */
        if (newCurrentWrite == 0)
        {
            /*
             *  The flash is completely empty.  Set things up so that when
             *  it writes the first record, the first page is correctly
             *  set up.
             */
            newCurrentWrite = (u32 *)OQ_FLASH_STORE;
            newSequence = 0;
        }

        /*
         *  Set up for future flash writes.
         */
        currentWrite = newCurrentWrite;
        sequence = newSequence;
        state = IDLE;
        return 0;

    case IDLE:
        /*
         *  While we are idling, and there is nothing to do, return that
         *  we did no work.
         */
        if (!opFlag)
            return 0;

        if ((opFlag & OF_CONFIG) && TempusTimeout(&configSaveTimer))
        {
            opFlag &= ~OF_CONFIG;
            state = CONFIG0;
            goto CONFIG0;
        }
        else if (opFlag & OF_SW_UPDATE)
        {
            opFlag &= ~OF_SW_UPDATE;
            state = SU_HDR_0;
            goto SU_HDR_0;
        }
        else if (opFlag & OF_SW_CHUNK)
        {
            opFlag &= ~OF_SW_CHUNK;
            state = SU_DATA_0;
            goto SU_DATA_0;
        }
        else if (opFlag & OF_SW_EXEC)
        {
            opFlag &= ~OF_SW_EXEC;
            state = SU_EXEC_0;
            goto SU_EXEC_0;
        }
#if 0
        else
        {
            /*
             *  Bogus flags.  Just clear everything.  (Should never happen.)
             */
            opFlag = 0;
            return 0;
        }
#endif // 0
        return 0;

    case CONFIG0:
    CONFIG0:
        /*
         *  Update the sequence number and copy the configuration to the
         *  local staging area.
         */
        Config.confSequence++;
        memcpy(&staging.config, &Config, sizeof staging.config);

        /*
         *  Calculate the CRC for the configuration.
         */
        staging.config.confCRC = ConfigCRC(&staging.config);

        /*
         *  Locate the next free slot in the config page, and if found,
         *  write to it.
         */
        {
            u32 * ps = (u32 *)OQ_FLASH_CONFIG;
            u32 * pe = (u32 *)(OQ_FLASH_CONFIG + OQ_FLASH_PAGE);
            u32 * cp;

            for (cp = ps; cp < pe; cp += CONFIG_WORDS)
            {
                if (*cp == 0xffffffff)
                {
                    /*
                     *  There is free space;  write the new configuration to
                     *  the free slot, and go wait for it to complete.
                     */
                    flashWrite(cp,
                               &staging.config.confArray[0],
                               ARRAY_SIZE(staging.config.confArray));
                    state = CONFIG3;
                    return 1;
                }
            }
        }

        /*
         *  There was no free space in the config page.  Before we erase
         *  that page, save a copy of the configuration in the circular
         *  flash area, just in case.
         */
        wrNew(RT_CONFIG);
        for (int i = 0; i < CONFIG_WORDS; i++)
            wr(32, staging.config.confArray[i]);

        savedState = CONFIG1;
        goto doWrite;

    case CONFIG1:
        /*
         *  Now that the config has been saved to the storage area, we can
         *  erase the config page safely.
         */
        flashErase((u32 *)OQ_FLASH_CONFIG);
        state = CONFIG2;
        return 1;

    case CONFIG2:

        /*
         *  Waiting for the flash erase to complete.
         */
        if (!flashComplete)
            return 0;

        /*
         *  We know that the whole config page is free, so just
         *  write to the first slot.
         */
        flashWrite((u32 *)OQ_FLASH_CONFIG,
                   &staging.config.confArray[0],
                   ARRAY_SIZE(staging.config.confArray));

        /*
         *  Go wait for the flash write to complete.
         */
        state = CONFIG3;
        return 1;

    case CONFIG3:
        /*
         *  Waiting for the flash write to complete.
         */
        if (!flashComplete)
            return 0;

        /*
         *  Voila.
         */
        configSaving = false;
        state = IDLE;
        return 1;

    case SU_HDR_0:
    SU_HDR_0:
        {
            SuInfo_t * si = &staging.sui;

            /*
             *  Pack the data into storage record format.
             */
            wrNew(RT_SU_HDR);
            wr(20, si->sequence);                       //  Sequence #
            wr(32, si->version);                        //  Version
            wr(14, si->start >> OQ_SU_CHUNK_SHIFT);     //  Start address
            wr(14, si->end >> OQ_SU_CHUNK_SHIFT);       //  End address

            /*
             *  Write it.
             */
            savedState = IDLE;
            goto doWrite;
        }

    case SU_DATA_0:
    SU_DATA_0:
        {
            SuData_t * sd = &staging.sud;

            /*
             *  Pack the data into storage record format.
             */
            wrNew(RT_SU_DATA);
            wr(20, sd->sequence);
            wr(13, sd->address >> OQ_SU_CHUNK_SHIFT);
            for (int i = 0; i < OQ_SU_CHUNK; i++)
            {
                wr(8, sd->data[i]);
            }

            /*
             *  Write it.
             */
            savedState = IDLE;
            goto doWrite;
        }

    case SU_EXEC_0:
    SU_EXEC_0:
        {
            SuExec_t * sx = &staging.sux;

            /*
             *  Pack the data into storage record format.
             */
            wrNew(RT_SU_EXEC);
            wr(20, sx->sequence);                       //  Sequence #
            wr(32, sx->version);                        //  Version

            /*
             *  Write it.
             */
            savedState = IDLE;
            goto doWrite;
        }

    doWrite:
        if (wrGo() == 0)
        {
            /*
             *  The write completed immediately, or failed.  We are done.
             */
            state = savedState;
            goto again;
        }

        state = WRITING;
        return 1;

    case WRITING:
        if (!flashComplete)
            return 0;

        if (wrGo() == 0)
        {
            state = savedState;
            goto again;
        }

        return 1;
    }

    return 0;
}

/**********************************************************************/

void
StoreRead(unsigned ops)
{
    readFlash(ops & (OF_CONFIG | OF_SW_CHUNK | OF_SW_UPDATE | OF_SW_EXEC));
}


void
StoreConfiguration(bool force)
{
    TempusSet(&configSaveTimer, Future(force ? 0 : 5));
    opFlag |= OF_CONFIG;
    configSaving = true;
}


bool
StoreConfigurationIsDone(void)
{
    return !configSaving;
}


void
StoreSoftwareUpdateInfo(SuInfo_t * sui)
{
    staging.sui = *sui;
    opFlag |= OF_SW_UPDATE;
}


void
StoreSoftwareUpdateChunk(SuData_t * sud)
{
    staging.sud = *sud;
    opFlag |= OF_SW_CHUNK;
}


void
StoreSoftwareUpdateExecute(SuExec_t * sux)
{
    staging.sux = *sux;
    opFlag |= OF_SW_EXEC;
}

/**********************************************************************/

#if OQ_DEBUG

static void
printStoreRecord(int ty)
{
    if (!storeDebugFlag)
        return;

    switch (ty)
    {
    case RT_ZERO:               //  Zero record -- no operation
    default:
        dbprintf("   ukn:  \n");
        break;

    case RT_SEQUENCE:           //  Storage manager sequence record
        dbprintf("   seq:  %d\n", staging.sequence);
        break;

    case RT_CONFIG:             //  OttoQ configuration structure
        dbprintf("config:  CRC %x (%x)\n", staging.config.confCRC,
                                           ConfigCRC(&staging.config));
        break;

    case RT_SU_HDR:             //  Software update header
        dbprintf("su-hdr:  seq=%x, ver=%x, start=%x, end=%x\n",
                                                    staging.sui.sequence,
                                                    staging.sui.version,
                                                    staging.sui.start,
                                                    staging.sui.end);
        break;

    case RT_SU_DATA:            //  Software update chunk
        dbprintf("su-dat:  seq=%x, addr=%x\n", staging.sud.sequence,
                                               staging.sud.address);
        break;

    case RT_SU_EXEC:            //  Software update "execute"
        dbprintf("su-exc:  seq=%x, version=%x\n", staging.sux.sequence,
                                                  staging.sux.version);
        break;
    }
}


static void
storeCommand(int argc, char ** argv)
{
    if (argc >= 2)
    {
        const char * arg = argv[1];

        if (StrcmpCmd("Info", arg) <= 1)
        {
            dprintf("Storage details:\n");
            dprintf("    Oldest page: %x (seq = %d)\n",
                                    storeDebugOldp, storeDebugOldq);
            dprintf("    Newest page: %x (seq = %d)\n",
                                    storeDebugNewp, storeDebugNewq);
            dprintf("    Current write pointer: %x\n", currentWrite);
        }
        else if (StrcmpCmd("List", arg) <= 1)
        {
            storeDebugFlag = true;
            readFlash(0);
            storeDebugFlag = false;
        }
    }
}

COMMAND(672)
{
    storeCommand, "STORE", 0,
    "STORE [command]", "Examine and manipulate flash storage",
    "store info  -- Print information about the storage\n"
    "store list  -- Print a diatribe of the entire circular flash store\n"
};

#endif // OQ_DEBUG

/**********************************************************************/
