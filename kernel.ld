OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

PHDRS
{
  text PT_LOAD FLAGS(5); /* read + execute */
  data PT_LOAD FLAGS(6); /* read + write */
}

SECTIONS
{
  /* OpenSBI occupies the machine-mode firmware area from 0x80000000. */
  . = 0x80200000;    /* 0x80000000 开始的一段是 OpenSBI 的固件的物理地址，为了防止重叠把操作系统的内存开始地址推后 */

  .text : {     /* 输出节的名字 */
    KEEP(*(.text.entry))
    *(.text .text.*)
    . = ALIGN(0x1000);
    /*
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    */
    PROVIDE(KERNEL_DATA = .);
  } :text    /* 表示将上述内容归为 PHDRS 的 text 段，给对应权限 */

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  } :text

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  } :data

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  } :data

  . = ALIGN(4096);
  PROVIDE(ALLOC_BEGIN = .);
  /* QEMU RAM is 130 MiB from 0x80000000; leave the first 2 MiB for OpenSBI. */
  PROVIDE(ALLOC_END = 0x80200000 + 128M);
}

/*
lab2 中一页是 4096 Byte = 4 KB
内核以页为单位分配内存

这里 lab2 的可分配内存从内核内存结束开始，一直到 (起始内存 + 128M)
可分配内存从 ALLOC_BEGIN 开始一直到 ALLOC_END
*/