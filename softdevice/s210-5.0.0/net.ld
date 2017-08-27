/*
 *  s210 : ANT, nrf51
 */

MEMORY
{
    flash (rx) :    org = 0x0000d000, len = 0x40000 - 0xd000
    ram (rwx) :     org = 0x20000900, len = 0x10000 - 0x900
}
