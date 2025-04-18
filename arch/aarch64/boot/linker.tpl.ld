/*
 * This file will be firstly configured by CMake to fill in `init_objects`,
 * then preprocessed by C compiler to fill in some definitions from `image.h`.
 */

#include "image.h"

ENTRY(_start)

SECTIONS
{
    DWARF_DEBUG

    . = TEXT_OFFSET;

    img_start = .;

    init : {
        ${init_objects}(.text* .data* .rodata* .bss* .extable*)
    }

    . = ALIGN(4K);
    init_end = ABSOLUTE(.);

    .text KERNEL_VADDR + init_end : AT(init_end) {
    _text_start = .;
        *(.text*)
    }
    . = ALIGN(4K);
    _text_end = .;

    .data : {
        *(.data*)
    }
    . = ALIGN(4K);

    .init.serial : {
        *(.init.serial*)
    }
    . = ALIGN(4K);
    _serial_end = . - KERNEL_VADDR;

    .rodata : {
        *(.rodata*)
    }
    . = ALIGN(4K);
    _edata = . - KERNEL_VADDR;

    _bss_start = . - KERNEL_VADDR;
    .bss : {
        *(.bss*)
    }
    . = ALIGN(4K);
    _bss_end = . - KERNEL_VADDR;

    _extable_start = .;
    .extable : {
        *(.extable*)
    }
    . = ALIGN(4K);
    _extable_end = .;
    
    img_end = . - KERNEL_VADDR;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
