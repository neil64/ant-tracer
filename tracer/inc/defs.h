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
 *  Project wide product specific definitions.
 */

#ifndef __DEFS_H__
#define __DEFS_H__

/**********************************************************************/
/*
 *  Puck specific constants and configuration.
 */

/*
 *  High frequency clock.  This clock runs periodically when needed, mostly
 *  to clock the CPU when its active, and to generate the carrier frequency
 *  for the radio.
 */
#define OQ_HFCLK            64000000

/*
 *  Low frequency clock.  This clock is used to time long running events.
 *  It's used to schedule radio packets, time application events, and keep
 *  time-of-day.  It's always enabled, and is extremely power efficient.
 */
#define OQ_LFCLK            32768

/*
 *  The configured tick rate.  The tick interrupt is used to run periodic
 *  events, such as sensor acquisition, and anything else that needs to
 *  run more than once per second, or need to run exactly on a timer beat.
 *  This rate must be a power of 2, and an integer multiple of the sensor
 *  sample rate (1 is ok).
 */
#define HZ                  16

/******************************/

/*
 *  OttoQ ANT stuff.
 */
#define ANT_LICENSE_KEY "3831-521d-7df9-24d8-eff3-467b-225f-a00e"   // Eval

/******************************/

/*
 *  Software update parameters.
 */
#define OQ_SU_CHUNK             64              //  Software update chunk size
#define OQ_SU_CHUNK_SHIFT       6

/******************************/

/*
 *  Flash space values.
 */
#define OQ_FLASH_START      0x0                 //  Flash start address
#define OQ_FLASH_BLOCK      512                 //  Flash block size
#define OQ_FLASH_PAGE       4096                //  Flash page size
#define OQ_FLASH_PAGES      128                 //  Flash pages on nRF52
#define OQ_FLASH_SIZE       (128*OQ_FLASH_PAGE) //  Flash size

/*
 *  Flash storage.
 */
#ifndef OQ_FLASH_STORE
#  define OQ_FLASH_STORE        (64*OQ_FLASH_PAGE)    //  Storage area start
#endif // OQ_FLASH_STORE

#ifndef OQ_FLASH_STORE_END
#  define OQ_FLASH_STORE_END    (126*OQ_FLASH_PAGE)   //  Storage area end
#endif // OQ_FLASH_STORE_END

#ifndef OQ_FLASH_CONFIG
#  define OQ_FLASH_CONFIG       (126*OQ_FLASH_PAGE)   //  Configuration page
#endif // OQ_FLASH_CONFIG

#define OQ_FLASH_BOOT_LOADER    (127*OQ_FLASH_PAGE)   //  Boot loader

/*
 *  Particulars of the app code.
 */
#define OQ_MAIN_STACK       4096                // App stack
#define OQ_INTR_STACK       128                 // Interrupt stack

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

#ifndef __ASM__

#include "types.h"

/******************************/
/*
 *      OQ functions.
 */

/****************/
// main/board.c

extern void     BoardLoop(void);
extern u32      BoardGetDeviceID(void);
extern void     SetupBoard(void);
extern void     Reboot(unsigned tod0);
extern void     TouchLED(unsigned n, unsigned cnt);
extern int      ButtonEvent(void);

//  Pin assignments
extern const int * PinAssign;

#define PIN_LED0            0
#define PIN_LED1            1
#define PIN_LED2            2
#define PIN_LED3            3

#define PIN_BUTTON0         4
#define PIN_BUTTON1         5
#define PIN_BUTTON2         6
#define PIN_BUTTON3         7

#define PIN_MODEM_UART_TX   16
#define PIN_MODEM_UART_RX   17
#define PIN_MODEM_UART_RTS  18
#define PIN_MODEM_UART_CTS  19
#define PIN_MODEM_POWER     20
#define PIN_MODEM_RESET     21

#define PIN_SET_1(p)       (NRF_P0->OUTSET = (1 << p))
#define PIN_SET_0(p)       (NRF_P0->OUTCLR = (1 << p))

/****************/
// main/main.c

extern const u32   Version;
extern const u8    VersionVersion;
extern const u8    VersionRelease;
extern const u16   VersionRevision;
extern const char  VersionDate[];

/****************/
// misc/b85.c

extern unsigned BinaryToBase85(char * to, const u8 * from, unsigned size);
extern unsigned Base85ToBinary(u8 * to, const char * from, unsigned size);

/****************/
// misc/crc.c

typedef struct
{
    u32 crc;
}
    CRC_t;

extern void     CRCInit(CRC_t * crc);
extern void     CRC(CRC_t * crc, u8 * data, unsigned len);
extern void     CRC8(CRC_t * crc, u32 data);
extern void     CRC16(CRC_t * crc, u32 data);
extern void     CRC32(CRC_t * crc, u32 data);

/****************/
// misc/rand.c

/*
 *  Fetch a random number based on `count' number of bytes from the
 *  local pool of random bytes.
 *
 *  `count' must be either 1,2,3 or 4 and will otherwise cause this
 *  function to return 0.
 *
 *  If the circular buffer has less than `count' entries - it will
 *  also return 0.
 */
extern unsigned RandGet(unsigned count);

/*
 *  Invoked by the `RandSuperLoop' to refill the buffer using the
 *  softdevice's app random vector pool.
 */
extern void     RandRefill(void);

/*
 *  Called from the main superloop.  Ensures that we have filled our local
 *  random byte buffer.
 */
extern void     RandStateMachine(void);

/****************/
// app/tracer.c

extern int      TraceSuperLoop(void);
extern void     TraceSetup(void);

/****************/
// Nordic error codes

#include "nrf_sdm.h"
#include "ant_error.h"

extern const char * NRFError(u16 code);
extern const char * NRFANTEvent(u16 code);
extern const char * NRFBLEEvent(u16 code);

/****************/

#endif // __ASM__

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

#ifndef __ASM__

/*
 *  INITFUNC() is used to create constructor functions.  The __attribute__
 *  tells the linker to make a table of function addresses, which we then
 *  call during startup.  The table is sorted lexically.  Each INITFUNC
 *  goes into a separate section named ".initx###" where the ### is the
 *  function "pri".  The linker sorts sections based on their name.
 */

#define INITFUNC(pri)                                                   \
        static void init ## pri (void);                                 \
        static void (*init ## pri ## ptr)(void)                         \
                __attribute__ ((section (".initx" # pri), used))        \
                = &init ## pri;                                         \
        static void init ## pri (void)

/**************************************************/

/*
 *  NRF Error Helper.
 *  Probably needs to move somewhere more appropriate.
 */
#define OQ_NRF_ERROR_CHECK(msg, error)                                  \
do                                                                      \
{                                                                       \
    if (error != NRF_SUCCESS)                                           \
        dprintf7("<n> "msg" error %s [%04x]\n", NRFError(error), error);\
    else                                                                \
        dprintf6("<n> "msg" success\n");                                \
} while (0)

/********************/

#endif // __ASM__

/**********************************************************************/

#endif // __DEFS_H__

/**********************************************************************/
