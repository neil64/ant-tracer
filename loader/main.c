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
 *         Boot loader
 *
 *  -------->   MAIN   <--------
 *
 *
 *  The boot loader is started by the MBR soon after reset.  Interrupts are
 *  disabled, and all devices should be disabled.  What the MBR has done is
 *  as follows:
 *
 *      -   If  UICR->NRFFW[1] != 0xffffffff
 *          -   Do a lot of crazy stuff to what ever it points to
 *              (Things like checking a key, CRC, comparing memory, erasing
 *               flash, resetting the CPU)
 *          -   Our implementation should not have this set.   !!!
 *      -   If  UICR->NRFFW[0] != 0xffffffff
 *              and what it points to is not 0xffffffff
 *          -   Execute from there (it's the boot loader)
 *      -   Otherwise, execute the soft device at 0x1000.
 *
 *  So when the boot loader is executed, it's almost directly from CPU reset,
 *  with no other initialization at all.
 *
 *  What we do:
 *
 *      -   Locate the storage flash:
 *              UICR->CUSTOMER[0] = storage flash start
 *              UICR->CUSTOMER[1] = storage flash end (+1)
 *              UICR->CUSTOMER[2] = config page start
 *      -   Read storage flash, looking for the most recent SU header.
 *      -   If none found, run the App.
 *      -   Compare the software update version with the App version.
 *      -   If equal, run the App.
 *      -   Collect all SU chunks, and verify we have all of them
 *      -   If not, run the App.
 *      -   Erase flash in the software update region
 *      -   Loop through all chunks and write them to flash.
 *      -   Run the App.
 *      -   (When we run the App, we set the protect bits on all of the code
 *           space for the App, SD, etc.  They can only be cleared with a
 *           reset.)
 */

/*
 *  Resource usage:
 *
 *  Flash memory:
 *      00000 - 01000       Master Boot Record (part of the soft device)
 *      01000 - sssss       Soft device.  The size of this depends on the
 *                          type of soft device and its version.  The ANT
 *                          only device ends at 0x12000, and the combined
 *                          ANT/BLE device ends at 0x27000.  The App starts
 *                          immeditately after.
 *      aaaaa - eeeee       The App should end up at around 64 KB in size.
 *      eeeee - fffff       8 KB for BLE pair details.
 *
 *                          The total of all of the above should a little
 *                          less than 256 KB.
 *
 *      40000 - 7f000       Circular flash storage (configuration, sensor
 *                          samples, software updates, etc).
 *
 *      7f000 - 80000       Boot loader (for software update)
 *
 *  RAM memory:  (20000000 - 2000ffff)
 *      0000 - 0004         MBR storage (vector table address)
 *      0004 - 4000         (unused)
 *      4000 - 8000         boot loader memory
 *      8000 - ffff         (unused)
 */

/**********************************************************************/

#include "bl.h"
#include "map.h"
#include "version.h"
#include "cpu/nrf.h"

/******************************/

const u32   Version = VERSION_HEX;
const u8    VersionVersion = VERSION_VERSION;
const u8    VersionRelease = VERSION_RELEASE;
const u16   VersionRevision = VERSION_REVISION;
const char  VersionDate[] = VERSION_DATE;

/******************************/

u32     StoreStart;         //  Storage area start
u32     StoreEnd;           //  Storage area end
u32     ConfigStart;        //  Configuration area start (1 page)

/**********************************************************************/
/*
 *  Simple debugging.
 */

static u32 * tracePtr = (u32 *)0x2000e000;    //  (Bottom of stack)

static void
trace(u32 v)
{
#if 0
    *tracePtr++ = v;
#else
    (void)tracePtr;
#endif
}

/**********************************************************************/

/*
 *  What we will do with the storage software update chunks.
 */
enum
{
    OP_SEARCH,
    OP_COMPARE,
    OP_WRITE,
}
    Op;

/*
 *  The most recent software update info we have found, and the most
 *  recent execute record.  They will be used to program the flash, and
 *  finally execute the new software.
 */
SuInfo_t   SuInfo;
SuExec_t   SuExec;

/*
 *  Maps to track which chunks we have found in the storage, and which pages
 *  we must erase and program to do this update.
 */
MAP_DEFINE(chunks, 0, 0x40000, OQ_SU_CHUNK);
MAP_DEFINE(pages, 0, 0x40000, OQ_FLASH_PAGE);

/**********************************************************************/
/*
 *  Manipulate the flash.
 */

static inline void
FlashWait(void)
{
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        ;
}


/*
 *  Erase the flash page at the given address.
 */
void
FlashErase(u32 * addr)
{
    /*
     *   Turn on the flash erase enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);
    FlashWait();

    /*
     *  Erase the page.
     */
    NRF_NVMC->ERASEPAGE = (u32)addr & ~(OQ_FLASH_PAGE - 1);
    FlashWait();

    /*
     *  Turn off the flash erase enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    FlashWait();
}


/*
 *  Write a block of words to flash.  The start address must be word aligned,
 *  and the count an even multiple of words.
 */
void
FlashProgram(u32 * dst, u32 * src, unsigned bytes)
{
    u32 * end = src + (bytes+3)/4;

    /*
     *  Turn on the flash write enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    FlashWait();

    /*
     *  Loop over the block, writing.
     */
    while (src < end)
    {
        *dst++ = *src++;
        FlashWait();
    }

    /*
     *  Turn off the flash write enable.
     */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    FlashWait();
}


void
Protect(void)
{
    /*
     *  Set the protect bits on all pages below the start of the storage area.
     */
    ;       //  Not done, for now
}


void
FlushCache(void)
{
    /*
     *  Disable then reenable the I-cache.  This will invalidate all cache
     *  entries.  The "nop" insns in between are just healthy paranoia.
     */
    NRF_NVMC->ICACHECNF =
            (NVMC_ICACHECNF_CACHEEN_Disabled << NVMC_ICACHECNF_CACHEEN_Pos);

    asm volatile ("nop; nop; nop; nop; nop; nop; nop; nop");
    asm volatile ("nop; nop; nop; nop; nop; nop; nop; nop");

    NRF_NVMC->ICACHECNF =
            (NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos);

    asm volatile ("nop; nop; nop; nop; nop; nop; nop; nop");
    asm volatile ("nop; nop; nop; nop; nop; nop; nop; nop");
}

/**********************************************************************/
/*
 *  Call backs from the storage management code.
 */

/*
 *  Called when storage found a software update information record.
 */
void
StoreCallbackSoftwareUpdateInfo(SuInfo_t * info)
{
    /*
     *  Make sure this software update header is sane.
     */
    if ((info->start < 0x1000 || info->start > 0x7f000) ||
        (info->end < info->start + 0x1000 || info->end > 0x80000))
            return;

    switch (Op)
    {
    case OP_SEARCH:
trace(0x69626373);
trace(info->sequence);
trace(SuInfo.sequence);
        /*
         *  We found a new software update header.  If it's more recent
         *  than the one we have (if we have one), over write it and start
         *  collecting chunks again.
         */
        if (info->sequence > SuInfo.sequence)
        {
            SuInfo = *info;

            MapClearAll(&chunks);
            MapSetRange(&chunks, info->start, info->end);

            SuExec.sequence = 0;
        }
        break;

    case OP_COMPARE:
        break;

    case OP_WRITE:
        break;
    }
}


/*
 *  Called when storage found a software update data chunk.
 */
void
StoreCallbackSoftwareUpdateChunk(SuData_t * data)
{
    /*
     *  Make sure the chunk is sane.
     */
    if (data->sequence != SuInfo.sequence ||
        data->address < SuInfo.start ||
        data->address + OQ_SU_CHUNK > SuInfo.end)
            return;

    switch (Op)
    {
    case OP_SEARCH:
        /*
         *  Mark that we have this chunk.
         */
        MapClear(&chunks, data->address);
        break;

    case OP_COMPARE:
        /*
         *  Compare this chunk to existing flash, and set a bit for the pages
         *  that don't match.
         */
        if (memcmp(&data->data[0], (u8 *)data->address, OQ_SU_CHUNK) != 0)
            MapSet(&pages, data->address);
        break;

    case OP_WRITE:
        /*
         *  Write this chunk to flash.
         */
        if (memcmp(&data->data[0], (u8 *)data->address, OQ_SU_CHUNK) != 0)
            FlashProgram((u32 *)data->address,
                         (u32 *)&data->data[0],
                         OQ_SU_CHUNK);
        break;
    }
}


/*
 *  Called when storage found a software update information record.
 */
void
StoreCallbackSoftwareUpdateExecute(SuExec_t * exec)
{
    switch (Op)
    {
    case OP_SEARCH:
        /*
         *  We found a new software update header.  If it's more recent
         *  than the one we have (if we have one), over write it and start
         *  collecting chunks again.
         */
        if (exec->sequence == SuInfo.sequence)
            SuExec = *exec;
        break;

    case OP_COMPARE:
        break;

    case OP_WRITE:
        break;
    }
}

/**********************************************************************/
/**********************************************************************/

void
Main(void)
{
    /*
     *  Get the storage flash area and config area from the UICR.  If those
     *  addresses are not set, bug out now.
     */
    StoreStart = NRF_UICR->CUSTOMER[0];
    StoreEnd = NRF_UICR->CUSTOMER[1];
    ConfigStart = NRF_UICR->CUSTOMER[2];
trace(StoreStart);
trace(StoreEnd);
trace(ConfigStart);

    if ((StoreStart < 0x1000 || StoreStart > 0x7f000) ||
        (StoreEnd < (StoreStart + 0x1000) || StoreEnd > 0x80000))
            goto out;

    /*
     *  Read through the storage flash looking for a software update.
     *  If there is no software update, or this is not a complete update
     *  image, bug out.
     */
    Op = OP_SEARCH;
    StoreRead();
trace(SuInfo.sequence);
trace(MapIsClearAll(&chunks));
trace(SuInfo.version);
trace(SuExec.version);
    if (SuInfo.sequence == 0 ||
        !MapIsClearAll(&chunks) ||
        SuInfo.version != SuExec.version)
    {
        goto out;
    }

    /*
     *  We have a software update.  Now it might be possible that we have
     *  already programmed some or all of this update.  Read the update
     *  from storage again, comparing it to the existing flash,
     */
    Op = OP_COMPARE;
    MapClearAll(&pages);
    StoreRead();

trace(0x12340005);
    /*
     *  If the entire flash is equal to the software update, and nothing
     *  remains to be programmed, just run the App.
     */
    if (MapIsClearAll(&pages))
        goto out;
trace(0x12340007);

    /*
     *  Erase the pages that are to be programmed.
     */
    for (unsigned pg = 1*OQ_FLASH_PAGE; pg < StoreStart; pg += OQ_FLASH_PAGE)
        if (MapIsSet(&pages, pg))
            FlashErase((u32 *)pg);
trace(0x12340009);

    /*
     *  Program all chunks that are different than the existing flash.
     */
    Op = OP_WRITE;
    StoreRead();

    /*
     *  Set the flash protect bits for the sections of flash that we use,
     *  then go run the soft device.  It should never return, but if it
     *  does, spin forever.
     */
  out:
trace(0x12340000);
    Protect();
    FlushCache();
    GoSoftDevice();
    for (;;)
        ;
}

/**********************************************************************/
