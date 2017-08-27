/*
 *  Linker script for the UICR configuration
 */


MEMORY
{
    mem (rwix) :    org = 0x1000120c, len = 0x10
}

SECTIONS
{
    .text :
    {
        *(.text)
    } > mem
}
