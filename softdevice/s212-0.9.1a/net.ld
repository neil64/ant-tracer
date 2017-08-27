/*
 *  s212 : ANT, nrf52
 */

MEMORY
{
    flash (rx) :    org = 0x00012000, len = 0x40000 - 0x12000
    ram (rwx) :     org = 0x20000a80, len = 0x10000 - 0xa80
}
