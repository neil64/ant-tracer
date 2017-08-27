/*
 *  s110 : BLE, nrf51
 */

MEMORY
{
    flash (rx) :    org = 0x00018000, len = 0x40000 - 0x18000
    ram (rwx) :     org = 0x20002000, len = 0x10000 - 0x2000
}
