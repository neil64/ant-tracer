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
 *  Map module to keep track of chunks in a group.
 */

#ifndef LOADER
#  include "defs.h"
#  include "stdlib.h"
#else
#  include "bl.h"
#endif // !LOADER

#include "map.h"

#if defined(OQ_DEBUG)
#  include "debug/debug.h"
#endif


#if 1
/*
 *  Clear the entire map's allocation and set it all to 0s.  This is expected
 *  to be called before we wish to track one or more segments within this map's
 *  range of slots.
 */
void
MapClearAll(Map_t * map)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return;

    memset(map->bits, 0x00, map->entryCount);
}
#endif // 0


#if 0
/*
 *  Set the entire map's allocation and set it all to 1s.  This is expected
 *  to be called before we wish to track one or more segments within this map's
 *  range of slots.
 */
void
MapSetAll(Map_t * map)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return;

    memset(map->bits, 0xff, map->entryCount);
}
#endif // 0

/***********/

#if 1
/*
 *  Clear the bit that corresponds to the slot in the map for the provided
 *  `address'.  All invalid maps will result in no modification to the map.
 */
void
MapClear(Map_t * map, unsigned address)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return;

    if (address < map->start || address >= map->end)
        return;

    unsigned offset = address - map->start;
    unsigned slot = offset / map->chunkSize;
    unsigned byteIdx = slot / BITS_PER_BYTE;
    unsigned bitOffset = slot % BITS_PER_BYTE;

    map->bits[byteIdx] &= ~(1 << bitOffset);
}
#endif // 0


#if 0
/*
 *  Clear the bits that corresponds to the slots in the map for the provided
 *  `start' and `end' addresses.  If either address is invalid, this function
 *  will make no modifications to the map.
 */
void
MapClearRange(Map_t * map, unsigned s, unsigned e)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return;

    if (s < map->start || e >= map->end || s >= e)
        return;

    /*
     *  Find the byte where the start address corresponds to and find the bit
     *  which marks its start.  Do the same for the end address.
     */
    unsigned sOffset = s - map->start;
    unsigned eOffset = e - map->start;

    unsigned sSlot = sOffset / map->chunkSize;
    unsigned sByte = sSlot / BITS_PER_BYTE;
    unsigned sBit = sSlot % BITS_PER_BYTE;

    unsigned eSlot = (eOffset + map->chunkSize - 1) / map->chunkSize;
    unsigned eByte = eSlot / BITS_PER_BYTE;
    unsigned eBit = eSlot % BITS_PER_BYTE;

    /*
     *  If both the start and end reside on the same byte we just clear
     *  the bits needed in place.
     */
    if (sByte == eByte)
    {
        map->bits[sByte] &= ~(((1 << (eBit-sBit)) - 1) << sBit);
    }
    else
    {
        /*
         *  Clear the bits in the first byte from sBit -> MSB.
         */
        map->bits[sByte++] &= ((1 << sBit) - 1);

        /*
         *  Clear the bytes from sByte + 1 -> eByte.
         */
        memset(&map->bits[sByte], 0x00, eByte-sByte);

        /*
         *  Clear the bits in the last byte up to eBit.
         */
        map->bits[eByte] &= ~((1 << eBit) - 1);
    }
}
#endif // 0


#if 0
/*
 *  Return true if the slot at the corresponding address is clear, false for
 *  all other cases.
 */
bool
MapIsClear(Map_t * map, unsigned address)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return false;

    if (address < map->start || address >= map->end)
        return false;

    /*
     *  Figure out the slot for the chunk at this address.
     */
    unsigned offset = address - map->start;
    unsigned slot = offset / map->chunkSize;
    unsigned byteIdx = slot / BITS_PER_BYTE;
    unsigned bitOffset = slot % BITS_PER_BYTE;

    /*
     *  We cannot just blindly return !MapIsSet(...) as invalid maps and
     *  addresses will return false.
     */
    return !(map->bits[byteIdx] & (1 << bitOffset));
}
#endif // 0


#if 1
/*
 *  Return true if the entire map is clear, false otherwise.  All invalid map's
 *  will also return false.
 */
bool
MapIsClearAll(Map_t * map)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return false;

    unsigned range = map->end - map->start;
    unsigned slot = range / map->chunkSize;
    unsigned byteOffset = slot / BITS_PER_BYTE;
    unsigned bitOffset = slot % BITS_PER_BYTE;

    u8 mask = 0xff;
    unsigned i = 0;
    for (i = 0; i < map->entryCount; i++)
    {
        if (i == byteOffset)
        {
            mask <<= (BITS_PER_BYTE - bitOffset);
            mask >>= (BITS_PER_BYTE - bitOffset);
        }
        if ((map->bits[i] & mask) != 0)
            return false;
    }
    return true;
}
#endif // 0

/***********/

#if 1
/*
 *  Set the bit that corresponds to the slot in the map for the provided
 *  `address'.  All invalid maps will result in no modification to the map.
 */
void
MapSet(Map_t * map, unsigned address)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return;

    if (address < map->start || address >= map->end)
        return;

    unsigned offset = address - map->start;
    unsigned slot = offset / map->chunkSize;
    unsigned byteIdx = slot / BITS_PER_BYTE;
    unsigned bitOffset = slot % BITS_PER_BYTE;

    map->bits[byteIdx] |= (1 << bitOffset);
}
#endif // 0


#if 1
/*
 *  Set the bits that corresponds to the slots in the map for the provided
 *  `start' and `end' addresses.  If either address is invalid, this function
 *  will make no modifications to the map.
 */
void
MapSetRange(Map_t * map, unsigned s, unsigned e)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return;

    if (s < map->start || e >= map->end || s >= e)
        return;

    /*
     *  Find the byte where the start address corresponds to and find the bit
     *  which marks its start.  Do the same for the end address.
     */
    unsigned sOffset = s - map->start;
    unsigned eOffset = e - map->start;

    unsigned sSlot = sOffset / map->chunkSize;
    unsigned sByte = sSlot / BITS_PER_BYTE;
    unsigned sBit = sSlot % BITS_PER_BYTE;

    unsigned eSlot = (eOffset + map->chunkSize - 1) / map->chunkSize;
    unsigned eByte = eSlot / BITS_PER_BYTE;
    unsigned eBit = eSlot % BITS_PER_BYTE;

    /*
     *  If both the start and end reside on the same byte we just set
     *  the bits needed in place.
     */
    if (sByte == eByte)
    {
        map->bits[sByte] |= (((1 << (eBit-sBit)) - 1) << sBit);
    }
    else
    {
        /*
         *  Set the bits in the first byte from sBit -> MSB. Should this be (sBit-1) ?
         */
        map->bits[sByte++] |= ~((1 << sBit) - 1);

        /*
         *  Set the bytes from sByte + 1 -> eByte.
         */
        memset(&map->bits[sByte], 0xff, eByte-sByte);

        /*
         *  Set the bits in the last byte up to eBit.
         */
        map->bits[eByte] |= ((1 << eBit) - 1);
    }
}
#endif // 0


#if 1
/*
 *  Return true if the slot at the corresponding address is set, false for all
 *  other cases.
 */
bool
MapIsSet(Map_t * map, unsigned address)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return false;

    if (address < map->start || address >= map->end)
        return false;

    /*
     *  Figure out the slot for the chunk at this address.
     */
    unsigned offset = address - map->start;
    unsigned slot = offset / map->chunkSize;
    unsigned byteIdx = slot / BITS_PER_BYTE;
    unsigned bitOffset = slot % BITS_PER_BYTE;

    return (map->bits[byteIdx] & (1 << bitOffset));
}
#endif // 0


#if 0
/*
 *  Return true if the entire map is set, false otherwise.  All invalid map's
 *  will also return false.
 */
bool
MapIsSetAll(Map_t * map)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return false;

    unsigned range = map->end - map->start;
    unsigned slot = range / map->chunkSize;
    unsigned byteOffset = slot / BITS_PER_BYTE;
    unsigned bitOffset = slot % BITS_PER_BYTE;

    u8 mask = 0xff;
    unsigned i = 0;
    for (i = 0; i < map->entryCount; i++)
    {
        if (i == byteOffset)
        {
            mask <<= bitOffset;
            mask >>= bitOffset;
        }

        if ((map->bits[i] & mask) != mask)
            return false;
    }
    return true;
}
#endif // 0

/***********/

#if 0
/*
 *  Returns the address of the `N'th set bit in the map.  However, if `N' bits
 *  are not set, it will return the address of the last bit that was set.
 *  If no bits are set, or the parameters are incomplete, this follows the
 *  return pattern of the above `MapFirstBitSet()' method.
 */
int
MapNBitSet(Map_t * map, unsigned n)
{
    if (!map || map->chunkSize == 0 || map->entryCount == 0)
        return MAP_INVALID;

    for (unsigned i = 0; i < map->entryCount; i++)
    {
        u8 c = map->bits[i];
        unsigned count = 0;

        while (c)
        {
            /*
             *  Find the next set bit in the byte.
             */
            while (!(c & 0x01))
            {
                c >>= 1;
                count++;
            }

            if (n == 0)
            {
                unsigned chunkIdx = (i * BITS_PER_BYTE) + count;
                return (map->start + (chunkIdx * map->chunkSize));
            }

            n--;
            c >>= 1;
            count++;
        }
    }
    return MAP_COMPLETE;
}
#endif // 0


#if 0
/*
 *  Returns the address of the first set bit in the map.  Returns
 *  MAP_COMPLETE (-1) if the map is clear
 *  MAP_INVALID  (-2) if the map or parameters are invalid
 */
int
MapFirstBitSet(Map_t * map)
{
    return MapNBitSet(map, 0);
}
#endif // 0

/***********/

#if defined(OQ_DEBUG) || defined(OQ_TESTING)
/*
 *  Debug function to dump the relevant portions of the map.
 */
void
MapDump(Map_t * map)
{
    if (!map || map->chunkSize == 0)
        return;

dprintf("\nmap with count: %d bytes starting at addr: %08x\n", map->entryCount, map->start);
    unsigned i = 0;
    for (i = 0; i < map->entryCount; i++)
    {
        dprintf("%02x", map->bits[i]);
        if (i % 16 == 15)
            dprintf("\n");
    }
    dprintf("\n");
}
#endif // defined(OQ_DEBUG) || defined(OQ_TESTING)

/**********************************************************************/
