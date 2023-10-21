SECTIONS {
    . = 0x80200000;

    . = ALIGN(4);
    __text_start = .;
    .text      :
    {
      *(.text)
    }

    . = ALIGN(4);
    __rodata_start = .;

    .rodata : { *(.rodata) }

    . = ALIGN(4);
    .data : { *(.data) }

    . = ALIGN(4);
    __bss_start = .;
    .bss : { *(.bss) *(.COMMON) }
    __bss_end = .;
}
