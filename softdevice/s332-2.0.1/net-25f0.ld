/*
 *  s332 : ANT/BLE, nrf52
 *      Default configuration.
 */

MEMORY
{
    flash (rx) :    org = 0x00029000, len = 0x40000 - 0x29000
    ram (rwx) :     org = 0x200025f0, len = 0x10000 - 0x25f0
}
