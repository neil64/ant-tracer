/*
 *  s120 : BLE, nrf51
 */

MEMORY
{
    flash (rx) :    org = 0x0001d000, len = 0x40000 - 0x1d000
    ram (rwx) :     org = 0x20002800, len = 0x10000 - 0x2800
}
