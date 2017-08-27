/*
 *  s132 : BLE, nrf52
 */

MEMORY
{
    flash (rx) :    org = 0x0001c000, len = 0x40000 - 0x1c000
    ram (rwx) :     org = 0x200013c8, len = 0x10000 - 0x13c8
}
