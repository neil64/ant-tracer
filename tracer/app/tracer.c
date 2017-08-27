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
 *  The ANT packet tracer.
 */

#include "types.h"
#include "defs.h"
#include "timer.h"
#include "store/config.h"
#include "cpu/atomic.h"
#include "debug/debug.h"
#include "debug/tachyon.h"
#include "stdlib.h"

/**********************************************************************/

/*
 *  Trace Frequency.
 */
#define TRACE_DEFAULT_FREQUENCY     79

/*
 *  The "correlator" packet address.  (Found empirically.)
 */
#define ANT_NET_BASE    0xda
#define ANT_NET_PREFIX  0xa4

/**********************************************************************/

#define PACKET_MEMORY   (32*1024)
#define PACKETS         (PACKET_MEMORY / sizeof (packet_t))

typedef struct
{
    u32     time;               //  Time stamp
    u8      data[13];           //  payload
    u8      __res0;
    s8      rssi;               //  RSSI
    u8      crcOk;              //  Good CRC
}
    packet_t;

static int          packetWr;
static int          packetRd;
static Atomic_t     packetCnt;
static int          packetCrcError;

// static packet_t __attribute__ ((aligned(16)))    packets[PACKETS];
static packet_t     packets[PACKETS];

/**********************************************************************/

/*
 *  Clear the memory watch unit (MWU) region watch.
 */
static inline u32
peripheralRegionEnClear(void)
{
    u32 v = NRF_MWU->REGIONEN;
    NRF_MWU->REGIONENCLR = v;

    // while (NRF_MWU->REGIONEN != 0)
    //     ;
    asm volatile ("dsb; nop; nop; nop");
    asm volatile ("nop; nop; nop; nop");

    return v;
}


/*
 *  Restore the memory watch unit's region watch.
 */
static inline void
peripheralRegionEnSet(u32 v)
{
    asm volatile ("dsb; nop; nop; nop");
    asm volatile ("nop; nop; nop; nop");

    NRF_MWU->REGIONENSET = v;
    // while (NRF_MWU->REGIONEN != v)
    //     ;
}

/**********************************************************************/

static void
radioInterrupt(void)
{
    NRF_RADIO_Type * radio = NRF_RADIO;

    /*
     *  Get access to the radio peripheral.
     */
    u32 prot = peripheralRegionEnClear();

    /*
     *  Clear our event.
     */
    radio->EVENTS_END = 0;

    /*
     *  Store any status.
     */
    packet_t * pkt = &packets[packetWr];

    bool crcOk = (radio->CRCSTATUS != 0);
    pkt->crcOk = crcOk;
    pkt->time = TachyonGet();
    pkt->rssi = -((int)radio->RSSISAMPLE);

    /*
     *  Count CRC errors.
     */
    if (!crcOk)
        packetCrcError++;

    /*
     *  If there is still space in the packet buffer, update the packet
     *  write pointer and set the new DMA address.  (We fill all but the
     *  last slot in the packet buffer, so the radio has somewhere to write
     *  a new packet when the buffer is full.)
     */
    if (crcOk && AtomicGet(&packetCnt) < ARRAY_SIZE(packets) - 1)
    {
        packetWr++;
        if (packetWr >= ARRAY_SIZE(packets))
            packetWr = 0;
        radio->PACKETPTR = (u32)&packets[packetWr].data[0];
        AtomicAddAndReturn(&packetCnt, 1);
    }

    /*
     *  Set up the CRC.  CRC includes the address field, CRC is 2 bytes.
     */
    radio->CRCPOLY = 0x1021;
    radio->CRCCNF = 2;
    radio->CRCINIT = 0xffff;

    /*
     *  Start the receiver again.
     */
    radio->TASKS_START = 1;

    /*
     *  Done.
     */
    peripheralRegionEnSet(prot);
}


void
SWI0_EGU0_IRQHandler(void)
{
    NRF_EGU0->EVENTS_TRIGGERED[0] = 0;
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    NVIC_ClearPendingIRQ(SWI0_EGU0_IRQn);

    radioInterrupt();
}


static void
radioStart(void)
{
    NRF_RADIO_Type * radio = NRF_RADIO;

    /*
     *  Get access to the radio peripheral.
     */
    u32 prot = peripheralRegionEnClear();

    /*
     *  Power cycle the radio.
     */
    radio->POWER = 0;
    radio->POWER = 1;

    /*
     *  1 MBs Nordic proprietary mode (ANT mode).
     */
    radio->MODE = 0;

    /*
     *  Frequency is set from the config.
     */
    radio->FREQUENCY = Config.confFrequency;

    /*
     *  TX power is +4dBm, but we should never transmit.
     */
    radio->TXPOWER = 0x04;

    /*
     *  Set up the BASE and PREFIX address registers to the given network
     *  address, and set to use it on transmit and receive.
     */
    radio->PREFIX0 = ANT_NET_PREFIX;
    radio->BASE0 = ANT_NET_BASE << 24;
    radio->TXADDRESS = 0;
    radio->RXADDRESSES = 1;

    /*
     *  LENGTH, S0, S1 fields all zero (unused in ANT), 8-bit preamble.
     */
    radio->PCNF0 = 0;

    /*
     *  Big endian on air, no whitening, base addr len = 1,
     *  13 byte static length for standard ANT packets.  (We could allow more
     *  than 13 for the `maxsz', but this would overflow the packet buffer.
     *  That would be useful for receiving extended ANT packets, but I did
     *  not see the need for our application.)
     */
    unsigned sz = 13;
    unsigned maxsz = 13;
    radio->PCNF1 = 0x01010000 | (sz << 8) | (maxsz << 0);

    /*
     *  Set up the CRC.  CRC includes the address field, CRC is 2 bytes.
     *  (0x1021 is the CCITT CRC polynomial, used in Bluetooth and many other
     *   things.  It's a very common polynomial.)
     */
    radio->CRCCNF = 2;
    radio->CRCINIT = 0xffff;
    radio->CRCPOLY = 0x11021;

    /*
     *  Start the DMA at the first packet slot.
     */
    radio->PACKETPTR = (u32)&packets[0].data[0];

    /*
     *  Normal ramp up, and TX 1's between packets.
     */
    radio->MODECNF0 = 0;

    /*
     *  Shortcut signals, and no direct interrupt.  Clear the events
     *  that we will use.
     */
    radio->SHORTS = RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
    radio->EVENTS_END = 0;

    /*
     *  Start the radio, and wait for the READY,
     */
    radio->EVENTS_READY = 0;
    radio->TASKS_RXEN = 1;
    while (!radio->EVENTS_READY)
        ;

    /*
     *  Start the receiver.
     */
    radio->TASKS_START = 1;

    /*
     *  Done.
     */
    peripheralRegionEnSet(prot);
}


#if 0
static void
radioStop(void)
{
    NRF_RADIO_Type * radio = NRF_RADIO;

    /*
     *  Get access to the radio peripheral.
     */
    u32 prot = peripheralRegionEnClear();

    /*
     *  Power off the radio.
     */
    radio->POWER = 0;

    /*
     *  Done.
     */
    peripheralRegionEnSet(prot);
}
#endif


static void
setupInterrupts(void)
{
    /*
     *  Disable the interrupt (just in case), and enable access to
     *  all peripherals (the ones that the soft device protected).
     */
    NVIC_DisableIRQ(SWI0_EGU0_IRQn);
    u32 prot = peripheralRegionEnClear();

    /*
     *  Route the radio events we are interested in via the PPI and EGU.
     */
    NRF_RADIO->EVENTS_END = 0;
    NRF_PPI->CH[0].EEP = (u32)&NRF_RADIO->EVENTS_END;
    NRF_PPI->CH[0].TEP = (u32)&NRF_EGU0->TASKS_TRIGGER[0];
    NRF_PPI->CHENSET = 0x1;
    NRF_EGU0->INTENSET = 0x1;

    /*
     *  Enable interrupts.
     */
    NVIC_SetPriority(SWI0_EGU0_IRQn, 7);
    NVIC_ClearPendingIRQ(SWI0_EGU0_IRQn);
    NVIC_EnableIRQ(SWI0_EGU0_IRQn);

    peripheralRegionEnSet(prot);
}

/**********************************************************************/

void
PacketDecode(unsigned addr, unsigned mod, unsigned net, unsigned node,
             unsigned aflag,
             u8 * payload)
{

    /*
     *  Custom packet decode goes here.
     */

}

/**********************************************************************/

static void
prTime(unsigned time)
{
    if (time < 10000)
        dprintf("%12d", time);
    else if (time < 1000000)
        dprintf("%8d,%03d", time / 1000,
                            time % 1000);
    else
        dprintf("%4d,%03d,%03d", time / 1000000,
                                 (time / 1000) % 1000,
                                 time % 1000);
}


/*
 *  Run the tracer super loop, looking for packets and reporting them.
 */
int
TraceSuperLoop(void)
{
    int work = 0;
    static unsigned lastTime;

    if (AtomicGet(&packetCnt) > 0)
    {
        /*
         *  Grab the next packet.
         */
        AtomicSubAndReturn(&packetCnt, 1);
        packet_t * pkt = &packets[packetRd];
        packetRd++;
        if (packetRd >= ARRAY_SIZE(packets))
            packetRd = 0;

        /*
         *  Time Stamp.
         */
        unsigned time = TACHY2US(pkt->time);
        unsigned elapsed = time - lastTime;
        lastTime = time;

        prTime(time);
        dprintf("(");
        prTime(elapsed);
        dprintf(")  ");

        /*
         *  RSSI.
         */
        dprintf("{%3ddB} ", pkt->rssi);

        /*
         *  Extract our addresses.
         */
        u32 addr = OqGet32(&pkt->data[0]);
        unsigned txType = (addr >> 24) & 0x0f;
        unsigned txTypeX = (addr >> 28) & 0x0f;
        unsigned devType = (addr >> 16) & 0xff;
        unsigned devNumber = (addr >> 0) & 0xffff;

        /*
         *  Print the generic ANT address fields.
         *
         *  (This could be printed differently, depending on how the
         *   application uses the addresses.)
         */
        dprintf("%02x.%02x.%04x  ", (txTypeX << 4) | txType,
                                    devType,
                                    devNumber);

        /*
         *  ANT flag.  This is the extra "flags" byte that makes up the 13th
         *  bytes of an ANT packet.  It's not documented by the ANT guys in
         *  any document that I could find.
         *
         *  Here is my best guess of what it contains:
         *
         *      bit-7       burst (packet is part of a burst transfer)
         *      bit-6       burst response
         *      bit-5       burst last (the final packet of a burst)
         *      bit-4       burst sequence counter (only 1 bit needed)
         *      bit-3       initial of a broadcast (or burst)
         *      bit-2       (no idea)
         *      bit-1       (no idea;  always 1)
         *      bit-0       (no idea)
         */
        unsigned aflag = pkt->data[4];
        char * c0 = "";
        char * c1 = "\e[0m";
        char * c2 = "   ";
        if (aflag & 0x80)
        {
            c0 = "\e[35m";
            if (aflag & 0x40)
                c2 = "<==";     //  c2 = "\xe2\x87\x92";
            else
                c2 = "==>";     //  c2 = "\xe2\x87\x90";
        }
        else if (aflag == 0x0a)
            c2 = "-->";         //  c2 = "\xe2\x86\x92";
        else if (aflag == 0x02)
            c2 = "<--";         //  c2 = "\xe2\x86\x90";
        dprintf("%s%s[%02x]%s  ", c0, c2, aflag, c1);

        /*
         *  Payload
         */
        dprintf("%02x %02x %02x %02x  %02x %02x %02x %02x  ",
            pkt->data[5],
            pkt->data[6],
            pkt->data[7],
            pkt->data[8],
            pkt->data[9],
            pkt->data[10],
            pkt->data[11],
            pkt->data[12]);

        /*
         *  Custom decode.
         */
        PacketDecode(addr, txType, txTypeX, devType, devNumber, &pkt->data[5]);

        /*
         *  Finish up.
         */
        dprintf("\n");
    }

    /*
     *  Return an indication of how much work we did.
     */
    return work;
}

/**********************************************************************/

void
TraceSetup(void)
{
    if (Config.confFrequency == 0)
    {
        Config.confFrequency = TRACE_DEFAULT_FREQUENCY;
        ConfigSave(false);
    }

    setupInterrupts();
    radioStart();
}

/**********************************************************************/

static void
captureCmd(int argc, char ** argv)
{
}

COMMAND(180)
{
    captureCmd, "CAPture", 0,
    "CAPture ...", "Packet capture",
    "   capture [options ...]\n"
    "       Begin a packet capture with the given options.\n"
};

/**********************************************************************/

static void
frequencyChangeCmd(int argc, char ** argv)
{
    int nf = -1;
    if (argc >= 2)
        nf = GetDecimal(argv[1]);

    if (nf > 0 && nf <= 80)
    {
        Config.confFrequency = nf;
        ConfigSave(false);

        radioStart();
    }

    dprintf("Current trace frequency: %d\n", Config.confFrequency);
}

COMMAND(181)
{
    frequencyChangeCmd, "FREQuency", 0,
    "FREQuency ...", "Change tracer frequency",
    "   frequency <value>\n"
    "       Change the frequency for the radio between 0 - 80.\n"
};

/**********************************************************************/

#if defined(OQ_DEBUG) && defined(OQ_COMMAND)

/**********************************************************************/
/*
 *  Debugging stuff.
 */

static void
antMsgPrint(u8 * msg, int sz)
{
    if (!msg)
        return;
    dprintf("[");
    while (sz-- > 0)
        dprintf("%C", *msg++);
    dprintf("]");
}


static void
antRx(int chan, ANT_MESSAGE * msg)
{
    dprintf("RX{%2d,%02x,%02x} ",
            msg->ANT_MESSAGE_ucSize,
            msg->ANT_MESSAGE_ucMesgID,
            msg->ANT_MESSAGE_ucChannel);
    if (msg->ANT_MESSAGE_ucMesgID == MESG_BROADCAST_DATA_ID)
    {
        dprintf("bcast: ");
        antMsgPrint(&msg->ANT_MESSAGE_aucPayload[0], 8);
        dprintf("\n");
    }
    else if (msg->ANT_MESSAGE_ucMesgID == MESG_ACKNOWLEDGED_DATA_ID)
    {
        dprintf("ack'd: ");
        antMsgPrint(&msg->ANT_MESSAGE_aucPayload[0], 8);
        dprintf("\n");
    }
    else if (msg->ANT_MESSAGE_ucMesgID == MESG_BURST_DATA_ID)
    {
        dprintf("burst:");
        antMsgPrint(&msg->ANT_MESSAGE_aucPayload[0], 8);
        dprintf("\n");
    }
    else
    {
        dprintf("unknown\n");
    }
}


void
DebugAntRxPrint(int chan, ANT_MESSAGE * msg)
                __attribute__ ((alias ("antRx")));
                // __attribute__ ((weak, alias ("antRx")));

#endif // defined(OQ_DEBUG) && defined(OQ_COMMAND)

/**********************************************************************/
