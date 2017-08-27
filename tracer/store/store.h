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
 *  Data storage.
 */

#ifndef __STORE_H__
#define __STORE_H__

#include "types.h"
#include "store/config.h"

/**********************************************************************/

/*
 *  Run the flash storage state machine.
 */
extern int      StoreSuperLoop(void);

/*
 *  Soft device reported that the flash operation completed;  `ok' is set
 *  if the operation succeeded, false if error.
 */
extern void     StoreFlashed(int ok);

/*
 *  Flash read function to fetch Software update headers, chunks and other
 *  stuff stored off in our flash.
 */
extern void     StoreRead(unsigned ops);

/******************************/

/*
 *  Schedule the storage of the configuration data.  The data is extracted
 *  directly from the configuration.
 */
extern void     StoreConfiguration(bool force);
extern bool     StoreConfigurationIsDone(void);

/******************************/

typedef struct
{
    unsigned    sequence;           //  Sequence ID this record was stored with
    u32         version;            //  Software update version
    unsigned    start;              //  Software update start address
    unsigned    end;                //  Software update end address (+1)
}
    SuInfo_t;


typedef struct
{
    unsigned    sequence;           //  Sequence ID this record was stored with
    unsigned    address;
    u8          data[64];
}
    SuData_t;


typedef struct
{
    unsigned    sequence;           //  Sequence ID this record was stored with
    u32         version;            //  Software update version
}
    SuExec_t;


/*
 *  Schedule the storage of a software update header, or update chunk.
 *  The data is copied from the caller supplied data to an internal buffer.
 */
extern void     StoreSoftwareUpdateInfo(SuInfo_t * info);
extern void     StoreSoftwareUpdateChunk(SuData_t * data);
extern void     StoreSoftwareUpdateExecute(SuExec_t * exec);

/**********************************************************************/

/*
 *  System provided callback functions that are called from the flash storage
 *  manager when a record is read from the flash.  There are callbacks for
 *  each type of object.
 */
extern void     StoreCallbackConfiguration(Config_t *);

extern void     StoreCallbackSoftwareUpdateInfo(SuInfo_t * info);
extern void     StoreCallbackSoftwareUpdateChunk(SuData_t * data);
extern void     StoreCallbackSoftwareUpdateExecute(SuExec_t * exec);

/**********************************************************************/

#endif // __STORE_H__

/**********************************************************************/
