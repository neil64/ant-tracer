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
 *  Radio packet tracer version 2
 *
 *  -------->   MAIN   <--------
 *
 *       It all starts here
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
 *      0000 - ssss         Soft device storage;  4 KB - 8 KB depending on
 *                          the configured soft device and configuration
 *      ssss - aaaa         App storage
 *      aaaa - f000         Heap storage (if used)
 *      f000 - ffff         Stack space;  Approximately 1 KB for the soft
 *                          device, and the remainder for the App.  4 KB
 *                          is generous.
 *
 *  NRF Devices:
 *      NVIC        SD managed, shared
 *      SysTick      ---
 *      POWER       SD managed
 *      WDT         app
 *      PPI         shared
 *      CLOCK       SD managed
 *      RADIO       SD
 *      NFCT         ---
 *      GPIO        shared
 *      COMP         ---
 *      LPCOMP       ---
 *      SAADC       app (for battery measurement)
 *      QDEC         ---
 *      PWM          ---
 *      I2S          ---
 *      PDM          ---
 *      NVMC        SD, library access from app
 *      RNG         ???
 *      RTC0        SD (radio scheduing, etc)
 *      RTC1         ---  (maybe the nRF timer library)
 *      RTC2        app (timing & callouts)
 *      TIMER0      SD (radio scheduling)
 *      TIMER1       ---
 *      TIMER2       ---
 *      TIMER3       ---
 *      TIMER4       ---
 *      TEMP        SD, library access
 *      ECB, CCM    SD (AES encryption)
 *      AAR         SD (radio address resolution)
 *      SPIM0       app (SPI interface for the SPI flash (relay only))
 *      SPIM1        ---
 *      SPIM2        ---
 *      TWIS         ---
 *      TWIM0       app (sensor comms)
 *      TWIM1       app (sensor comms)
 *      UARTE       app (cell modem comms)
 *      SPIS         ---
 */

/**********************************************************************/

#include <stdbool.h>
#include "stdlib.h"
#include "defs.h"
#include "timer.h"
#include "store/store.h"
#include "store/config.h"
#include "cpu/atomic.h"
#include "debug/debug.h"
#include "debug/tachyon.h"
#include "version.h"
// #include "ant/oqant.h"
// #include "ant_parameters.h"
// #include "ant_interface.h"
#include "nrf_sdm.h"
#include "nrf_nvic.h"

/******************************************/

const u32   Version = VERSION_HEX;
const u8    VersionVersion = VERSION_VERSION;
const u8    VersionRelease = VERSION_RELEASE;
const u16   VersionRevision = VERSION_REVISION;
const char  VersionDate[] = VERSION_DATE;

/**********************************************************************/

/*
 *  Global nvic state instance, required by nrf_nvic.h
 */
nrf_nvic_state_t nrf_nvic_state;


/*
 *  Softdevice assert handler
 */
void
softdevice_assert_callback(uint32_t id, uint32_t pc, uint32_t info)
{
    for (;;)
    {
        // No implementation needed.
    }
}


/*
 *  Softdevice event interrupt handler.  This will fire when the SWI2
 *  interrupt is raised by the softdevice as a signal to the application to
 *  consume an event.  This event might be any one of: BLE, ANT or the SOC.
 *
 *  WARNING: DO NOT REMOVE THIS EMPTY FUNCTION!
 *
 *  The interrupt manager for the NRF will only clear the pending flag on
 *  this interrupt once the below handler is invoked, it does not have to
 *  explicitly clear the interrupt's pending flag as that is done for us.
 */
void
SWI2_EGU2_IRQHandler(void)
{
    /*
     *  Do nothing!  The main loop should wake up and process the super-loop
     *  as needed.
     */
}

/**********************************************************************/

static nrf_clock_lf_cfg_t   clockLFCfg =
{
    .source = NRF_CLOCK_LF_SRC_XTAL,
    .rc_ctiv = 0,
    .rc_temp_ctiv = 0,
    .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM
};

/********************/

/*
 *  Setup the soft device and apply the ANT license key.
 */
static void
sdConfig(void)
{
    /*
     *  Enable the soft device.
     */
    unsigned err = sd_softdevice_enable(&clockLFCfg,
                                        softdevice_assert_callback,
                                        ANT_LICENSE_KEY);
    OQ_NRF_ERROR_CHECK("soft device enable", err);

    /*
     *  Enable softdevice event interrupts to the application.
     */
    err = sd_nvic_EnableIRQ(SWI2_EGU2_IRQn);
    OQ_NRF_ERROR_CHECK("sd_nvic_EnableIRQ", err);

    /*
     *  Enable the dcdc regulator.
     */
    err = sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    OQ_NRF_ERROR_CHECK("sd_power_dcdc_mode_set", err);

#if defined(OQ_DEBUG) && defined(OQ_TACHYON)
    /*
     *  Make sure that the 64 MHz crystal is running continuously, so
     *  tha the tachyon logging has an accurate time base.  This will
     *  increase power consumption.
     */
    sd_clock_hfclk_request();
#endif // defined(OQ_DEBUG) && defined(OQ_TACHYON)
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

/*
 *  Handle App events, such as flash operations.
 */
static int
handleSocEvents(void)
{
    int work = 0;

    for (;;)
    {
        uint32_t ev;
        int err = sd_evt_get(&ev);
        if (err != NRF_SUCCESS)
            break;

        work++;

        if (ev == NRF_EVT_FLASH_OPERATION_SUCCESS)
            StoreFlashed(true);
        else if (ev == NRF_EVT_FLASH_OPERATION_ERROR)
        {
            dprintf("Warning - flash write failed!\n");
            StoreFlashed(false); // TODO: FIXME: This should be false!
        }
            // StoreFlashed(false);

        /*
        else
        {
            handle other SOC events
        }
        */
    }

    return work;
}

/**********************************************************************/

void
Main(void)
{
    /******  Part 0  --  early items  ******/

    /*
     *  Initialize all that must before before printf(), and should be
     *  before everything.
     */
    TachyonInit();

    /****************************************/

    u32 devID = BoardGetDeviceID();
    dprintf("\n\n---------------------\n");
    dprintf("    Radio packet trace version 2 -- " VERSION_STRING "\n");
    dprintf("    DeviceID: %08x\n", devID);

    /****************************************/

    /*
     *  Start the Soft Device.  (Note that soft device needs to start early
     *  in this process, because it manages many of the nRF52 devices.)
     */
    sdConfig();

    /*
     *  Determine which board we are running on.  Needed before we configure
     *  any GPIO pins.
     */
    SetupBoard();

    /*
     *  Setup timers and devices.
     */
    TimerInit();

    /*
     *  Set up the configuration.  First read the from the configuration
     *  page, which should always have the latest.
     */
    ConfigInitialize();

    /*
     *  Now run a mini super loop for the flash only, to allow it to read
     *  and possibly initialize the flash.  This will retrieve any
     *  configuration in the storage flash, if it exists, and is more recent
     *  than one from the config page.  (Note that items located in circular
     *  flash must not be processed yet, but can be remembered for later.)
     */
    while (StoreSuperLoop())
    {
        TempusMagnaCirculi();
        handleSocEvents();
    }

    /*
     *  Update the device id in the config based on the board.  This has to be
     *  done after the store loop is run once since we could have config entries
     *  in the circular flash.
     */
    Config.confDeviceID = devID;

    /**********************************************************************/
    //
    /******  Part 1  --  initialization function list  ******/

    /*
     *  Run the INIT functions (INITFUNC).  They are ordered lexically by
     *  the linker, so by using names like "init999", lower numbered
     *  functions are called before higher numbered functions.
     */
    {
        typedef void i_f(void);
        extern i_f * __sinit[];
        i_f ** ip = &__sinit[0];
        i_f * ifp;
        while ((ifp = *ip))
        {
            ifp();
            ip++;
        }
    }

    /**********************************************************************/
    //
    /******  Part 2  --  higher level initialization  ******/

    /*
     *  Start the tracer.
     */
    TraceSetup();

    /*
     *  Set up the ANT and/or BLE protocols.
     */
    // AntSetup();

    /**********************************************************************/
    /**********************************************************************/
    /**********************************************************************/

    /*
     *  The Super Loop!!                <--------------
     */
    int work = 1;
    for (;;)
    {
        /*
         *  If no work was done in the super loop the last time through,
         *  we can safely sleep until there is an interrupt.
         */
        if (!work)
        {
            /*
             *  Wait for something to happen.  This returns if any
             *  interrupts have occurred since the last call.  If no
             *  interrupts have happened, it will cause the CPU to sleep.
             */
            sd_app_evt_wait();
        }

        work = 0;

        /*
         *  Run the Tempus thingy.
         */
        TempusMagnaCirculi();

#ifdef OQ_COMMAND
        /*
         *  Debugger command processor.
         */
        CommandLoop();
#endif // OQ_COMMAND

#ifdef OQ_DEBUG
        /*
         *  Flash LEDs, and check buttons.
         */
        BoardLoop();
#endif // OQ_DEBUG

        /*
         *  Handle SOC events, such as flash operations.
         */
        work += handleSocEvents();

        /*
         *  Run the flash storage state machine.
         */
        work += StoreSuperLoop();

        /*
         *  Ensure that our larger random buffer is filled.
         */
        RandStateMachine();

        /*
         *  Run the tracer (main) state machine.
         */
        work += TraceSuperLoop();
    }
}

/**********************************************************************/
