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
 *  Debugging.
 */

#ifndef __OQ_DEBUG_H__
#define __OQ_DEBUG_H__

#include "types.h"

/**********************************************************************/

#if OQ_DEBUG

extern void     DebugPutChar(int c);
extern void     DebugPutString(const char * str, unsigned len);
extern void     DebugPutCharBlocked(int c);
extern void     DebugPutStringBlocked(const char * str, unsigned len);
extern int      DebugPutAvail(void);
extern int      DebugGetChar(void);
extern int      DebugGetCharBlocking(void);
extern void     DebugShutdown(void);

static inline int       dgetc(void)     { return DebugGetChar(); }
static inline void      dputc(int c)    { DebugPutChar(c); }
static inline void      dxputc(int c)   { DebugPutCharBlocked(c); }

extern int      snprintf(char * out, uint sz, const char * fmt, ...);
extern int      dprintf(const char * fmt, ...);
extern int      printf(const char * fmt, ...);
extern int      dbprintf(const char * fmt, ...);

extern const char   HexString[];

#else // OQ_DEBUG

static inline int   dgetc(void)                                 { return 0; }
static inline void  dputc(int c)                                {}
static inline void  dxputc(int c)                               {}
static inline int   sprintf(char * out, const char * fmt, ...)  { return 0; }
static inline int   snprintf(char * out, uint sz, const char * fmt, ...)
                                                                { return 0; }
static inline int   dprintf(const char * fmt, ...)              { return 0; }
static inline int   printf(const char * fmt, ...)               { return 0; }
static inline int   dbprintf(const char * fmt, ...)             { return 0; }
static inline void  OqDebugInit(void)                           {}

#undef OQ_DEBUG_MASK        //  Make sure the masked versions are out as well
#define OQ_DEBUG_MASK 0

#endif // OQ_DEBUG

/**********************************************************************/
/*
 *  The context specific debug printfs.
 *
 *  Each of the following are allocated as follows:
 *
 *      0   system like things (SPI, UART, etc)
 *      1   BLE
 *      2   Cellular modem state machine
 *      3   Relay App and protocol
 *      4
 *      5
 *      6
 *      7
 */

#if OQ_DEBUG_MASK & 0x0001
    #define dprintf0    dprintf
    #define dbprintf0   dbprintf
#else
    static inline int dprintf0(const char * fmt, ...)   { return 0; }
    static inline int dbprintf0(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0002
    #define dprintf1    dprintf
    #define dbprintf1   dbprintf
#else
    static inline int dprintf1(const char * fmt, ...)   { return 0; }
    static inline int dbprintf1(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0004
    #define dprintf2    dprintf
    #define dbprintf2   dbprintf
#else
    static inline int dprintf2(const char * fmt, ...)   { return 0; }
    static inline int dbprintf2(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0008
    #define dprintf3    dprintf
    #define dbprintf3   dbprintf
#else
    static inline int dprintf3(const char * fmt, ...)   { return 0; }
    static inline int dbprintf3(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0010
    #define dprintf4    dprintf
    #define dbprintf4   dbprintf
#else
    static inline int dprintf4(const char * fmt, ...)   { return 0; }
    static inline int dbprintf4(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0020
    #define dprintf5    dprintf
    #define dbprintf5   dbprintf
#else
    static inline int dprintf5(const char * fmt, ...)   { return 0; }
    static inline int dbprintf5(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0040
    #define dprintf6    dprintf
    #define dbprintf6   dbprintf
#else
    static inline int dprintf6(const char * fmt, ...)   { return 0; }
    static inline int dbprintf6(const char * fmt, ...)  { return 0; }
#endif

#if OQ_DEBUG_MASK & 0x0080
    #define dprintf7    dprintf
    #define dbprintf7   dbprintf
#else
    static inline int dprintf7(const char * fmt, ...)   { return 0; }
    static inline int dbprintf7(const char * fmt, ...)  { return 0; }
#endif

/**********************************************************************/
/*
 *  Command interpreter and debugger.
 */

typedef void    CMDFunc_f(int argc, char ** argv);

typedef struct Cmd_t Cmd_t;
struct Cmd_t
{
    CMDFunc_f *     func;           // Command implementation
    const char *    cmd;            // Command name
    const char *    cmd1;           // Command name (alternate)
    const char *    help0;          // Left part of help string
    const char *    help1;          // Right part of help string
    const char *    help9;          // Multi-line help string
};

extern void     CommandLoop(void);
extern int      StrcmpCmd(const char * cmd, const char * text);
extern u32      GetDecimal(const char *);
extern u32      GetHex(char * sp);
extern void     ToHex(char * str, u32 val, int digits);

/*
 *  COMMAND() is used to create a command decription and add it to the
 *  command list.  An object file section is created whose name is based
 *  on `sort';  later, the linker collects these sections together
 *  and sorts them based on the section name.  The result is a NULL
 *  terminated array of pointers to Cmd_t sorted by `sort', and appearing
 *  under the symbol "CommandList" (see .../main/linker-script).
 *
 *  By convention, `sort' is a three digit number: 000-999.
 *  (The symbols are sorted lexically, not numerically).
 *  Numbers must be unique within a source file, but need not be unique
 *  globally.
 */
#define COMMAND(sort)                                           \
    static const Cmd_t __cmd ## sort;                           \
    static const Cmd_t * __cmds ## sort                         \
            __attribute__ ((section (".cmds" # sort), used))    \
            = &__cmd ## sort;                                   \
    static const Cmd_t __cmd ## sort =

/*
 *  Array of pointers to Cmd_t's;  defined in .../main/linker-script.
 */
extern const Cmd_t * const  CommandList[];

/**********************************************************************/

extern void     TouchLED(unsigned n, unsigned cnt);

/**********************************************************************/

#endif // __OQ_DEBUG_H__

/**********************************************************************/
