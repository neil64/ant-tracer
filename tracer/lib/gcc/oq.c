/*
 *  OttoQ herding of the pieces we need from the GCC support library,
 *  libgcc.a
 */

/*
 *  Definitions to fill in for missing include files.
 */
typedef unsigned    size_t;
typedef long long   DItype;

#define BITS_PER_UNIT   8
#define MIN_UNITS_PER_WORD   4

#ifndef __ARM_EABI__
#  define __ARM_EABI__
#endif

/*
 *  Define to include each part of the library.
 */
#define L_udivmoddi4


/*
 *  Some includes that we will add.
 */


/*
 *  Include the C portion of the library.
 */
#include "libgcc2.c"
