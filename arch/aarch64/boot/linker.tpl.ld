/*
 * This file will be firstly configured by CMake to fill in `init_objects`,
 * then preprocessed by C compiler to fill in some definitions from `image.h`.
 */

#include "image.h"

/*
 * DWARF debug sections.
 * Symbols in the DWARF debugging sections are relative to
 * the beginning of the section so we begin them at 0.
 */
#define DWARF_DEBUG						\
	/* DWARF 1 */						\
	.debug          0 : { *(.debug) }			\
	.line           0 : { *(.line) }			\
	/* GNU DWARF 1 extensions */				\
	.debug_srcinfo  0 : { *(.debug_srcinfo) }		\
	.debug_sfnames  0 : { *(.debug_sfnames) }		\
	/* DWARF 1.1 and DWARF 2 */				\
	.debug_aranges  0 : { *(.debug_aranges) }		\
	.debug_pubnames 0 : { *(.debug_pubnames) }		\
	/* DWARF 2 */						\
	.debug_info     0 : { *(.debug_info			\
			.gnu.linkonce.wi.*) }			\
	.debug_abbrev   0 : { *(.debug_abbrev) }		\
	.debug_line     0 : { *(.debug_line) }			\
	.debug_frame    0 : { *(.debug_frame) }			\
	.debug_str      0 : { *(.debug_str) }			\
	.debug_loc      0 : { *(.debug_loc) }			\
	.debug_macinfo  0 : { *(.debug_macinfo) }		\
	.debug_pubtypes 0 : { *(.debug_pubtypes) }		\
	/* DWARF 3 */						\
	.debug_ranges	0 : { *(.debug_ranges) }		\
	/* SGI/MIPS DWARF 2 extensions */			\
	.debug_weaknames 0 : { *(.debug_weaknames) }		\
	.debug_funcnames 0 : { *(.debug_funcnames) }		\
	.debug_typenames 0 : { *(.debug_typenames) }		\
	.debug_varnames  0 : { *(.debug_varnames) }		\
	/* GNU DWARF 2 extensions */				\
	.debug_gnu_pubnames 0 : { *(.debug_gnu_pubnames) }	\
	.debug_gnu_pubtypes 0 : { *(.debug_gnu_pubtypes) }	\
	/* DWARF 4 */						\
	.debug_types	0 : { *(.debug_types) }			\
	/* DWARF 5 */						\
	.debug_addr	0 : { *(.debug_addr) }			\
	.debug_line_str	0 : { *(.debug_line_str) }		\
	.debug_loclists	0 : { *(.debug_loclists) }		\
	.debug_macro	0 : { *(.debug_macro) }			\
	.debug_names	0 : { *(.debug_names) }			\
	.debug_rnglists	0 : { *(.debug_rnglists) }		\
	.debug_str_offsets	0 : { *(.debug_str_offsets) }

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
