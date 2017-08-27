/*
 *  UICR set up for the main hex image.
 */


    .syntax unified
    .arch armv7e-m


/**********************************************************************/
/**********************************************************************/

    .section .text

    .long   0x40000             //  Storage flash start address
    .long   0x7e000             //  Storage flash end address (+1)
    .long   0x7e000             //  Config page start address


/**********************************************************************/
/**********************************************************************/

  .end
