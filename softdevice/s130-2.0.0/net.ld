/*
 *  s130 : BLE, nrf51
 */

MEMORY
{
    flash (rx) :    org = 0x0001b000, len = 0x40000 - 0x1b000
    ram (rwx) :     org = 0x200013c8, len = 0x10000 - 0x13c8
}
