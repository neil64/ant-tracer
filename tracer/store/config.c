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
 *  Configuration management.
 */


#include <stdbool.h>
#include "defs.h"
#include "assert.h"
#include "stdlib.h"
#include "store/config.h"
#include "store/store.h"
#include "debug/debug.h"


/*
 *  The running configuration.
 *
 *  It is initialized at compile time to the "factory" defaults, and will
 *  be overwritten with the current running configuration once the system
 *  is finished booting.
 *
 */
Config_t Config;

STATIC_ASSERT((sizeof Config == CONFIG_BYTES), config_size_not_expected);

/**********************************************************************/

/*
 *  Setup the factory defaults for the configuration structure.
 */
void
ConfigDefault(void)
{
    /*
     *  Clear the configuration, preserving the configuration sequence
     *  number and reboot count.
     */
    unsigned seq = Config.confSequence;
    unsigned boots = Config.bootCount;

    memset(&Config, 0, sizeof Config);

    Config.confSequence = seq + 1;
    Config.bootCount = boots;

    /*
     *  Set the (non-zero) defaults.
     *
     *  Note that the ActiveListen must be quicker than the MSG_TTL time in
     *  order to allow a puck a chance to get the message before it expires.
     */
    Config.confFrequency = 0;

    /*
     *  Upgrade a few things for development.
     */
#if OQ_DEVEL
    NRF_FICR_Type * ficr = NRF_FICR;
    Config.confDeviceID = ficr->DEVICEID[0];
#endif // OQ_DEVEL
}


/*
 *  Schedule a commit of the current configuration to flash.  The actual
 *  write will happen some time in the next minute or so, unless the `force'
 *  flag is set, in which case the write will happen as soon as possible.
 *
 *  If space remains, a new copy of the configuration is appended to the
 *  configuration page, with an updated sequence number.  If there is not
 *  space remaining, a copy of the configuration is written to the circular
 *  flash storage (just in case), then the configuration page is erased,
 *  and finally a new copy of the config is saved to the now clear config
 *  page.
 */
void
ConfigSave(bool force)
{
    StoreConfiguration(force);
}


/*
 *  Return true if a previously requested configuration write is complete.
 */
bool
ConfigIsDone(void)
{
    return StoreConfigurationIsDone();
}


/*
 *  Calculate and return the CRC of the given configuration structure.
 */
u32
ConfigCRC(Config_t * cf)
{
    CRC_t crc;
    CRCInit(&crc);
    CRC(&crc,
        (u8 *)&cf->confArray[0],
        sizeof cf->confArray - sizeof cf->confCRC);
    return crc.crc;
}

/******************************/

/*
 *  Locate the most recent configuration from flash storage and set up
 *  `Config'.
 */
void
ConfigInitialize(void)
{
    /*
     *  Default config in case we have nothing.
     */
    ConfigDefault();

    /*
     *  Load the latest config instance from the config page.
     */
    Config_t * cs = (Config_t *)OQ_FLASH_CONFIG;
    Config_t * ce = (Config_t *)(OQ_FLASH_CONFIG + OQ_FLASH_PAGE);
    for (Config_t * cp = cs; cp < ce; cp++)
        if (cp->confArray[0] != 0xffffffff  &&
            cp->confCRC == ConfigCRC(cp)  &&
            cp->confSequence > Config.confSequence)
        {
            memcpy(&Config.confArray[0],
                   &cp->confArray[0],
                   sizeof Config.confArray);
        }
}

/******************************/

/*
 *  Call back from the storage management code when a configuration record
 *  is read from storage flash.
 */
void
StoreCallbackConfiguration(Config_t * cf)
{
    //  Check the CRC for paranoia sake!
    if (cf->confSequence > Config.confSequence &&
        cf->confCRC == ConfigCRC(cf))
    {
        /*
         *  Update the RAM copy of the config, and schedule it to be
         *  written back to the config flash page.
         */
        memcpy(&Config.confArray[0],
               &cf->confArray[0],
               sizeof Config.confArray);

        StoreConfiguration(true);
    }
}

/******************************/

#if defined(OQ_COMMAND) && defined(OQ_DEBUG)

static void
configCmd(int argc, char ** argv)
{
    if (argc >= 2)
    {
        const char * arg = argv[1];

        if (StrcmpCmd("DEFault", arg) <= 1)
            ConfigDefault();
        else if (StrcmpCmd("SAVE", arg) <= 1)
            ConfigSave(true);
        else if (StrcmpCmd("NODEid", arg) <= 1)
        {
            if (argc < 3)
                dprintf("Unable to set nodeID! None specified!\n");
            else
            {
                Config.confNodeID = GetHex(argv[2]);
                ConfigSave(false);
            }
        }
        else if (StrcmpCmd("NETid", arg) <= 1)
        {
            if (argc < 3)
                dprintf("Unable to set netID! None specified!\n");
            else
            {
                Config.confNetID = GetHex(argv[2]);
                ConfigSave(false);
            }
        }
        else if (StrcmpCmd("DEVid", arg) <= 1)
        {
            if (argc < 3)
                dprintf("Unable to set deviceID! None specified!\n");
            else
            {
                Config.confDeviceID = GetHex(argv[2]);
                ConfigSave(false);
            }
        }
        else if (StrcmpCmd("FREQuency", arg) <= 1)
        {
            int freq = GetDecimal(argv[2]);
            if (argc < 3 || freq < 2 || freq > 80)
                dprintf("frequency must be between 2 and 80\n");
            else
            {
                Config.confFrequency = freq;
                ConfigSave(false);
            }
        }
    }
    else
    {
        dprintf("Relay configuration (sequence %d)\n", Config.confSequence);
        dprintf("    node-ID          %04x\n", Config.confNodeID);
        dprintf("    net-ID           %04x\n", Config.confNetID);
        dprintf("    device-ID        %08x\n", Config.confDeviceID);
        dprintf("    frequency        %d MHz\n", Config.confFrequency + 2400);
        dprintf("\n");
        dprintf("Active parameters:\n");
        // dprintf("    node-ID        %04x\n", App.nodeID);
        // dprintf("    net-ID         %04x\n", App.netID);
        // dprintf("    reset reason   %02x\n", App.resetReason);
        dprintf("    reboot count   %d\n", Config.bootCount);
    }
}

COMMAND(610)
{
    configCmd, "CONFiguration", 0,
    "CONFiguration", "Device config management and querying",
    "Prints the current config structure if no arguments are specified.\n"
    "DEFault     - sets the factory default configuration.\n"
    "SAVE        - force saves the config as it is.\n"
    "NODEid <id> - set the nodeID for this device.\n"
    "NETid <id>  - set the netID for this device.\n"
    "DEVid <id>  - set the deviceID for this device.\n"
};

#endif // defined(OQ_COMMAND) && defined(OQ_DEBUG)

/**********************************************************************/
