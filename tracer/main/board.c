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
 *  Board configuration and setup.
 */


#include <stdbool.h>
#include "stdlib.h"

#include "defs.h"
#include "store/config.h"
#include "timer.h"
#include "debug/debug.h"

/********************************************************************/

/*
 *  Define the lanes that exist in the OttoQ base relay board.  Each
 *  stacked connector supports three swizzled lanes which can be accessed
 *  by the CPU.  Each lane is six GPIO signals wide and has dedicated
 *  power and ground pins as needed.  The following structure defines the
 *  lane to pin mapping for the puck2i CPU board.  (Note that only two
 *  lanes are connected to the puck2i.)
 */
#define LANE_WIDTH 6

#define L0P0 11
#define L0P1 19
#define L0P2 23
#define L0P3 26
#define L0P4 28
#define L0P5 30

#define L1P0 17
#define L1P1 22
#define L1P2 25
#define L1P3 27
#define L1P4 29
#define L1P5 31

#define L2P0 -1
#define L2P1 -1
#define L2P2 -1
#define L2P3 -1
#define L2P4 -1
#define L2P5 -1

/********************************************************************/

int IsPuck2;            //  True if this is a Puck2 (otherwise a PCA10040)
const int * PinAssign;  //  Pin assignments

static int maxLED;
static int maxButton;


/*
 *  Static list of developer boards for the PCA10040.  We blindly assume
 *  that if the board running this firmware is not one of these below, that
 *  the board under question is a PuckV2.
 */
static const u32 pcaBoards[] =
{
    0xee9136cf,         //  682588270   (Neil.0)
    0xd9bf549b,         //  682846100   (Neil.1)

    0x0caf5e1a,         //  682076784   (Shaba.0)
    0x5700cd15,         //  682908516   (Shaba.1)

    0x96693ea8,         //  682642155
    0x819271e4,         //  682953012
    0x417e990e,         //  682679854
    0xb18be151,         //  682116613

    0
};


/*
 *  Define the pin mapping for PCA10040.  This board has 4 LEDs and the
 *  I2C interface arbitrarily bound to SDA 30 and SCL 31 for all 4 sensors.
 */
static const int pinsPCA[32] =
{
    [PIN_LED0] = 17,
    [PIN_LED1] = 18,
    [PIN_LED2] = 19,
    [PIN_LED3] = 20,

    [PIN_BUTTON0] = 13,
    [PIN_BUTTON1] = 14,
    [PIN_BUTTON2] = 15,
    [PIN_BUTTON3] = 16,

    /*
     *  Map the appropriate pins for the UART connection to the modem.
     */
    [PIN_MODEM_UART_TX] = -1,
    [PIN_MODEM_UART_RX] = -1,
    [PIN_MODEM_UART_RTS] = -1,
    [PIN_MODEM_UART_CTS] = -1,
    [PIN_MODEM_POWER] = -1,
    [PIN_MODEM_RESET] = -1,
};


/*
 *  Define the pin mapping for the PuckV2 board in Relay mode.  This board has
 *  1 LED, potentially a modem connected via a UART and an SPI interface to a
 *  SPI flash chip (25LQ080B).
 */
static const int pinsRelay2[32] =
{
    [PIN_LED0] = 8,
    [PIN_LED1] = -1,
    [PIN_LED2] = -1,
    [PIN_LED3] = -1,

    [PIN_BUTTON0] = -1,
    [PIN_BUTTON1] = -1,
    [PIN_BUTTON2] = -1,
    [PIN_BUTTON3] = -1,

    /*
     *  Map the appropriate pins for the UART connection to the modem.  This is
     *  done expecting the modem to reside on `Lane0' of this stack.
     */
    [PIN_MODEM_UART_RX] = L0P3,
    [PIN_MODEM_UART_TX] = L0P5,
    [PIN_MODEM_UART_CTS] = L0P2,
    [PIN_MODEM_UART_RTS] = L0P4,
    [PIN_MODEM_POWER] = L0P0,
    [PIN_MODEM_RESET] = L0P1,
};


/*
 *  Temporarily determine what board we are using based on the device ID
 *  loaded into the FICR by Nordic.
 */
static void
whichBoard(void)
{
    NRF_FICR_Type * ficr = NRF_FICR;
    u32 devID = ficr->DEVICEID[0];

    IsPuck2 = true;
    PinAssign = &pinsRelay2[0];
    maxLED = 1;
    maxButton = 0;
    for (const u32 * ip = &pcaBoards[0]; *ip; ip++)
    {
        if (*ip == devID)
        {
            IsPuck2 = false;
            PinAssign = &pinsPCA[0];
            maxLED = 4;
            maxButton = 4;
            break;
        }
    }
}

/**********************************************************************/


/*
 *  Reboot timer and associated reboot implementation.
 */
static TempusCallout_t rebootTimer;

static void
kickTheBucket(TempusCallout_t * timer)
{
    static int ktbCounter = 0;

    /*
     *  Alert the watchdog timer that we are about to do a safe reset.  It
     *  should clear its channels so that if the bootloader takes long to
     *  program a potential image, we dont pull the plug prematurely.
     */
    // WdtAboutToReboot();

    /*
     *  If the config is in the middle of writing an entry, try again later.
     */
    if (ConfigIsDone() || ktbCounter >= 3)  //  XXX - is 3 good enough?
    {
#if defined(OQ_DEBUG)
        /*
         *  WARNING: Issuing any other calls to SEGGER_RTT_Read* or Write* will
         *  cause INIT() to get called before the system goes down.  The call
         *  to `DebugShutdown' has to be RIGHT before the call to the system
         *  reset.
         */
        DebugShutdown();
#endif // defined(OQ_DEBUG)

        NVIC_SystemReset();
    }
    else
    {
        /*
         *  Config is not done, make sure it has a chance to save now.
         */
        ConfigSave(true);

        ktbCounter++;
        TempusCallout(timer, Future(3));    //  Try again soon...
    }
}


/*
 *  Reboot the system at the specified system time `tod0'.  The reboot time
 *  is forced to be at least `MinResetDelay' seconds in the future.
 *  Subsequent calls will modify the reboot time.  If a `tod0' of 0 is
 *  given, the reboot is cancelled.
 */
void
Reboot(unsigned tod0)
{
    /*
     *  Calculate the soonest we can do the reboot.
     */
    unsigned MinResetDelay = Future(3);

    /*
     *  Are we cancelling?
     */
    if (tod0 == 0)
    {
        TempusCallout(&rebootTimer, 0);
        return;
    }

    /*
     *  Enforce the minimum reboot delay.
     */
    if (tod0 < MinResetDelay)
        tod0 = MinResetDelay;

    /*
     *  Set a Tempus timer to implement the reboot.
     */
    TempusCalloutFunc(&rebootTimer, tod0, &kickTheBucket, 0, 0);
}

/******************************/

static void
rebootCommand(int argc, char ** argv)
{
    unsigned t = Future(0);
    if (argv[1])
        t = Future(GetDecimal(argv[1]));

    Reboot(t);
}

COMMAND(066)
{
    rebootCommand, "REBOOT", 0,
    "reboot [seconds]", "reboot in `seconds' from now",
    "A `reboot' with no argument will reboot as soon as possible (typically\n"
    "about 3 seconds from now.  A `reboot 0' will cancel a pending reboot."
};

/**********************************************************************/

#ifdef OQ_DEBUG

static unsigned ledCnt[4];
static int button[4];
static int buttonCnt[4];

/*
 *  Turn on the `n'th LED for `cnt' ticks.
 */
void
TouchLED(unsigned n, unsigned cnt)
{
    if (n >= maxLED)
        return;

    if (cnt > 40)
        cnt = 40;
    ledCnt[n] = cnt;
}


int
ButtonEvent(void)
{
    for (int i = 0; i < maxButton; i++)
        if (button[i] == 1)
        {
            button[i] = 2;
            return i+1;
        }

    return 0;
}

#endif // OQ_DEBUG

/**********************************************************************/

#ifdef OQ_DEBUG

void
BoardLoop(void)
{
    static Timer_t tmr;

    if (TimerTimeout(&tmr))
    {
        /*
         *  40 times per second, we blink an LED and check for button presses.
         */
        TimerSetMs(&tmr, 25);

        NRF_GPIO_Type * p0 = NRF_P0;

        for (int i = 0; i < maxLED; i++)
        {
            unsigned x = 1 << PinAssign[PIN_LED0 + i];
            if (ledCnt[i] > 0)
            {
                p0->OUTCLR = x;
                ledCnt[i]--;
            }
            else
                p0->OUTSET = x;
            x <<= 1;
        }

        for (int i = 0; i < maxButton; i++)
        {
            unsigned x = 1 << PinAssign[PIN_BUTTON0 + i];
            int b = !(p0->IN & x);
            if (b && !button[i])
            {
                if (buttonCnt[i] < 0)
                    buttonCnt[i] = 0;
                buttonCnt[i]++;
                if (buttonCnt[i] >= 2)
                    button[i] = 1;
            }
            else if (!b && button[i])
            {
                if (buttonCnt[i] > 0)
                    buttonCnt[i] = 0;
                buttonCnt[i]--;
                if (buttonCnt[i] <= 2)
                    button[i] = 0;
            }

            x <<= 1;
        }
    }


#if 0

    {
        static Tempus_t pus;
        if (TempusTimeout(&pus))
        {
            TempusSet(&pus, Future(2));
            TouchLED(3, 1);
dprintf("tick (%d, %d)\n", pus.time, GetTODZero());
        }
    }

    int x = ButtonEvent();
    if (x > 0)
    {
        x--;
        dprintf("button event %d\n", x);
        TouchLED(x, 4);
    }

#endif

}

#endif // OQ_DEBUG

/**********************************************************************/

/*
 *  Fetch the appropriate device-ID for this board.  A valid device-ID in
 *  UICR[4] will be returned as the ID.  If this is not present, the Nordic
 *  assigned ID in the FICR will be used.
 */
u32
BoardGetDeviceID(void)
{
    NRF_UICR_Type * uicr = NRF_UICR;
    u32 devID = uicr->CUSTOMER[3];
    if (devID == 0xffffffff)
    {
        NRF_FICR_Type * ficr = NRF_FICR;
        devID = ficr->DEVICEID[0];

#if OQ_DEBUG
        dprintf("Alert! No device ID issued for this device!\n");
        dprintf("       Using ID in nrf FICR!\n");
#endif // OQ_DEBUG
    }

    return devID;
}


/*
 *  Setup the PinAssign table to point at the correct pin definitions,
 *  and setup any of the LED / button pins.
 */
void
SetupBoard(void)
{
    /*
     *  Load the correct board based on if we are a PuckV2 or a NRF dev kit.
     */
    whichBoard();
    if (IsPuck2)
        dprintf("  (Puck2 hardware!)\n");
    else
        dprintf("  (Nordic PCA10040 hardware)\n");


    NRF_GPIO_Type * p0 = NRF_P0;

    //  LEDs (Output, high drive low)
    p0->PIN_CNF[PinAssign[PIN_LED0]] = 0x00000103;
    if (PinAssign[PIN_LED1] >= 0)
    {
        p0->PIN_CNF[PinAssign[PIN_LED1]] = 0x00000103;
        p0->PIN_CNF[PinAssign[PIN_LED2]] = 0x00000103;
        p0->PIN_CNF[PinAssign[PIN_LED3]] = 0x00000103;
    }

    //  Buttons (Input, pull up)
    if (PinAssign[PIN_BUTTON0] >= 0)
    {
        p0->PIN_CNF[PinAssign[PIN_BUTTON0]] = 0x0000000c;
        p0->PIN_CNF[PinAssign[PIN_BUTTON1]] = 0x0000000c;
        p0->PIN_CNF[PinAssign[PIN_BUTTON2]] = 0x0000000c;
        p0->PIN_CNF[PinAssign[PIN_BUTTON3]] = 0x0000000c;
    }

    //  Modem power on/off (Output, no-pull, open collector)
    if (PinAssign[PIN_MODEM_POWER] >= 0)
        p0->PIN_CNF[PinAssign[PIN_MODEM_POWER]] = 0x00000603;

    //  Modem reset (Output, no-pull, open collector)
    if (PinAssign[PIN_MODEM_RESET] >= 0)
        p0->PIN_CNF[PinAssign[PIN_MODEM_RESET]] = 0x00000603;
}

/**********************************************************************/
