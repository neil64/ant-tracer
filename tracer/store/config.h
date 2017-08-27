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
 *  Persistent configuration.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/**********************************************************************/

/*
 *  The number of 32-bit words in the configuration.
 */
#define CONFIG_WORDS    32
#define CONFIG_SHORTS   (CONFIG_WORDS * 2)
#define CONFIG_BYTES    (CONFIG_SHORTS * 2)

/*
 *  Relay configuration structure.
 *
 *  WARNING:  Care *must* be taken to ensure that the layout of this structure
 *            does not change over time, so that stored configuration remains
 *            compatible with future software.
 */
typedef union
{
    struct
    {
        u16     confSequence;       //  Configuration sequence number

        u8      __res0[2];

        u16     confNodeID;         //  16-bit "lot network" assigned ID
        u16     confNetID;          //  "Lot network" assigned ID
        u32     confDeviceID;       //  Puck/Relay ID (factory assigned)

        u32     __res1[3];

        u8      confFrequency;      //  Tracer frequency [1, 80]
        u8      __res4[3];

        u32     __res5[22];

        u32     printMask;          //  Debugging printf() mask

        u16     bootCount;          //  Number of times we have (re)booted

        u16     __res6;

        u32     confCRC;            //  CRC of this configuration, when stored
    };

    u32 confArray[CONFIG_WORDS];    //  Word mapped storage
    u16 confShorts[CONFIG_SHORTS];  //  Short mapped storage
    u8  confBytes[CONFIG_BYTES];    //  Byte mapped storage
}
    Config_t;

extern Config_t Config;


/*
 *  Schedule a commit of the current configuration.  The actual write will
 *  happen some time in the next minute or so, unless the `force' flag is
 *  set, in which case the write will happen as soon as possible.
 */
extern void     ConfigSave(bool force);

/*
 *  Return true if a previously requested configuration write is complete.
 */
extern bool     ConfigIsDone(void);

/*
 *  Locate the most recent configuration from flash storage and set up
 *  `Config'.
 */
extern void     ConfigInitialize(void);

/*
 *  Return the CRC of the given configuration structure.
 */
extern u32      ConfigCRC(Config_t * conf);

/**********************************************************************/

#endif // __CONFIG_H__

/**********************************************************************/
