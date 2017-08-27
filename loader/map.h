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
 *
 *               _.-,=_"""--,_
 *            .-" =/7"   _  .3#"=.
 *          ,#7  " "  ,//)#d#######=.
 *        ,/ "      # ,i-/###########=
 *       /         _)#sm###=#=# #######\
 *      /         (#/"_`;\//#=#\-#######\
 *     /         ,d####-_.._.)##P########\
 *    ,        ,"############\\##bi- `\| Y.
 *    |       .d##############b\##P'   V  |
 *    |\      '#################!",       |
 *    |C.       \###=############7        |
 *    '###.           )#########/         '
 *     \#(             \#######|         /
 *      \B             /#######7 /      /
 *       \             \######" /"     /
 *        `.            \###7'       ,'
 *          "-_          `"'      ,-'
 *             "-._           _.-"
 *                 """"---""""
 *
 *  The map's primary purpose is to track a bunch of chunks in a aggregate
 *  of data.  You can set, clear and check various bits to see how complete
 *  the aggregate is at any time.
 *
 *  Calling the `MapUpdate()' with an address will set the corresponding
 *  position in the map.  When the mask is setup - all tracking bits are
 *  cleared.
 */

#ifndef __MAP_H__
#define __MAP_H__

/**********************************************************************/

#define MAP_COMPLETE (-1)
#define MAP_INVALID  (-2)

typedef struct
{
    unsigned    start;              // start offset of the data being tracked
    unsigned    end;                // end address of the largest byte tracked
    unsigned    chunkSize;          // size of each chunk
    u8 *        bits;               // pointer to data
    unsigned    entryCount;         // number of bytes pointed to by `bits'
}
    Map_t;

/**********************************************************************/

/*
 *  Mask creation helper.
 *
 *  Allocate a byte array to associate with this mask.  The allocation has to
 *  be able to account for the range of address from start to end where each
 *  chunk has a size as specified by `cs' below.  This allows for the caller
 *  to create various masks to track different allocations, pages et al.
 *  `s' and `e' must be aligned to a multiple of `cs'.
 */
#define MAP_DEFINE(name, s, e, cs)                                    \
u8 name ## _alloc[(((((e) - (s)) + (cs) - 1) / (cs)) + 8 - 1) / 8];   \
Map_t name =                                                          \
{                                                                     \
    .start = (s),                                                     \
    .end = (e),                                                       \
    .chunkSize = (cs),                                                \
    .bits = &name ## _alloc[0],                                       \
    .entryCount = (((((e) - (s)) + (cs) - 1) / (cs)) + 8 - 1) / 8,    \
}

// TODO: Optimize cs -> shift so we can do away with idivs in the code.

/**********************************************************************/

/*
 *  Clear the entire map's allocation and set it all to 0s.  This is expected
 *  to be called before we wish to track one or more segments within this map's
 *  range of slots.
 */
extern void         MapClearAll(Map_t * map);

/*
 *  Set the entire map's allocation and set it all to 1s.  This is expected
 *  to be called before we wish to track one or more segments within this map's
 *  range of slots.
 */
extern void         MapSetAll(Map_t * map);

/***********/

/*
 *  Clear the bit that corresponds to the slot in the map for the provided
 *  `address'.  All invalid maps will result in no modification to the map.
 */
extern void         MapClear(Map_t * map, unsigned address);

/*
 *  Clear the bits that corresponds to the slots in the map for the provided
 *  `start' and `end' addresses.  If either address is invalid, this function
 *  will make no modifications to the map.
 */
extern void         MapClearRange(Map_t * map, unsigned s, unsigned e);

/*
 *  Return true if the slot at the corresponding address is ckear, false for all
 *  other cases.
 */
extern bool         MapIsClear(Map_t * map, unsigned address);

/*
 *  Return true if the entire map is clear, false otherwise.
 */
extern bool         MapIsClearAll(Map_t * map);

/***********/

/*
 *  Set the bit that corresponds to the slot in the map for the provided
 *  `address'.  All invalid maps will result in no modification to the map.
 */
extern void         MapSet(Map_t * map, unsigned address);

/*
 *  Set the bits that corresponds to the slots in the map for the provided
 *  `start' and `end' addresses.  If either address is invalid, this function
 *  will make no modifications to the map.
 */
extern void         MapSetRange(Map_t * map, unsigned start, unsigned end);

/*
 *  Return true if the slot at the corresponding address is set, false for all
 *  other cases.
 */
extern bool         MapIsSet(Map_t * map, unsigned address);

/*
 *  Return true if the entire map is set, false otherwise.
 */
extern bool         MapIsSetAll(Map_t * map);

/***********/

/*
 *  Returns the address of the first set bit in the map.  Returns
 *  MAP_COMPLETE (-1) if the map is clear
 *  MAP_INVALID  (-2) if the map or parameters are invalid
 */
extern int          MapFirstBitSet(Map_t * map);

/*
 *  Returns the address of the `N'th set bit in the map.  However, if `N' bits
 *  are not set, it will return the address of the last bit that was set.
 *  If no bits are set, or the parameters are incomplete, this follows the
 *  return pattern of the above `MapFirstBitSet()' method.
 */
extern int          MapNBitSet(Map_t * map, unsigned n);

/**********************************************************************/

#if defined(OQ_DEBUG)
/*
 *  Debug function to dump the relevant portions of the map.
 */
extern void         MapDump(Map_t * map);
#endif // defined(OQ_DEBUG)

/**********************************************************************/

#endif // __MAP_H__

/**********************************************************************/
