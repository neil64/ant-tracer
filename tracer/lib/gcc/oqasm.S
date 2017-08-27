/*
 *  OttoQ herding of the pieces we need from the GCC support library,
 *  libgcc.a
 */

/*
 *  Definitions to fill in for missing include files.
 */
#define BITS_PER_UNIT   8


/*
 *  Define to include each part of the library.
 */
#define L_udivmoddi4
#define L_aeabi_ldivmod
#define L_dvmd_tls


/*
 *  Some includes that we will add.
 */


/*
 *  Include the C portion of the library.
 */
#include "lib1funcs.S"
