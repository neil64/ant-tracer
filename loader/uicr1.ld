/*
 *  Linker script for the UICR configuration
 */


MEMORY
{
    mem (rwix) :    org = 0x10001014, len = 0x10
}

SECTIONS
{
    .text :
    {
        *(.text)
    } > mem
}
