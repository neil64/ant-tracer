/*
 *  Linker script for the UICR configuration
 */


MEMORY
{
    mem (rwix) :    org = 0x10001080, len = 0x80
}

SECTIONS
{
    .text :
    {
        *(.text)
    } > mem
}
