/*
 *  Linker script for the boot loader.
 */


MEMORY
{
    /*
     *  Boot loader -- the last page in flash, and some RAM.
     */
    flash (rx) :    org = 0x0007f000, len = 0x01000
    ram (rwx) :     org = 0x20004000, len = 0x04000
}

SECTIONS
{

    /*
     *  Code sections -- link to Flash.
     */
    .text :
    {
        KEEP(*(.vectors))
        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        __etext = .;

        *(.rodata*)

        KEEP(*(.eh_frame*))
    } > flash

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > flash

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash
    __exidx_end = .;

    __etext = .;

    /**************************************************/

    /*
     *  Initialized data sections -- linked to RAM, but stored in Flash.
     */
    .data : AT (__etext)
    {
        __data_start__ = .;
        *(vtable)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > ram

    .fs_data :
    {
        PROVIDE(__start_fs_data = .);
        KEEP(*(.fs_data))
        PROVIDE(__stop_fs_data = .);
    } > ram

    /*
     *  Provide the addresses of the initialized data source in RAM.
     */
    __data_load_start = LOADADDR(.data);
    __data_load_end = LOADADDR(.data) + SIZEOF(.data);

    ASSERT(__data_load_start < __data_load_end, ".data must have something")

    /**************************************************/

    /*
     *  Uninitialized data -- linked to RAM.
     */
    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > ram

    ASSERT(__bss_start__ < __bss_end__, ".bss must have something")

    /**************************************************/

    /*
     *  The stack (there can be only one).
     */

    /*
     *  .stack_dummy section doesn't contains any symbols.  It is only
     *  used for linker to calculate size of stack sections, and assign
     *  values to stack symbols later
     */
    .stack_dummy (COPY):
    {
        *(.stack*)
    } > ram

    /*
     *  Set stack top to end of RAM.  The stack limit moves down by the
     *  size of stack_dummy section
     */
    __StackTop = ORIGIN(ram) + LENGTH(ram);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    __StackBottom = __StackLimit;
    PROVIDE(__stack = __StackTop);

    /**************************************************/

    /*
     *  The heap -- from the end of BSS to the bottom of the stack.
     */
    __heap_start__ = __bss_end__;
    __heap_end__ = __StackLimit;

    /*
     *  Make sure that the bss, heap and stack don't run into each other.
     */
    ASSERT(__StackLimit >= __bss_end__, "RAM overflow")
}
