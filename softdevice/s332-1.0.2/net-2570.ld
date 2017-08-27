/*
 *  s332 : ANT/BLE, nrf52
 *      Default configuration.
 */

MEMORY
{
    flash (rx) :    org = 0x00027000, len = 0x40000 - 0x27000
    ram (rwx) :     org = 0x20002570, len = 0x10000 - 0x2570
}
