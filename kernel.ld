OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  . = 0x80000000;         /* 整个操作系统内存的起始位置 */

  .text : {
    *(.text .text.*)      /* 将所有 .o 文件内的 text 段内容归入 .elf 中的 .text 段 */
    . = ALIGN(0x1000);    /* 定义最后的字节对齐至 0x1000 的倍数，即 4096 字节的倍数，即 4KB    可以理解为将 . 的地址直接增加到 4096KB 的倍数的位置 */
    /*
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    */
    PROVIDE(etext = .);    /* 定义一个位置标签（指针）指向 .text 区域结束的位置 */
  }

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);
}
