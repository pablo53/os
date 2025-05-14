SECTIONS
{
    . = 0x00000000; /* Start address */

    /* Place the text section */
    .text : {
        *(.text)
    }

    /* Place read-only data (if any) */
    .rodata : {
        *(.rodata)
    }

    /* Place data section */
    .data : {
        *(.data)
    }

    /* Place BSS (uninitialized data) */
    .bss : {
        *(.bss)
    }

    /* Avoid gaps: force all other sections here */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.rel.*)
        *(.rela.*)
        *(.plt)
        *(.got)
        *(.got.plt)
        *(.interp)
        *(.dynamic)
        *(.dynsym)
        *(.dynstr)
    }
}
