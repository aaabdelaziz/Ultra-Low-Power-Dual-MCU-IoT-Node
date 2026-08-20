/*
 * STM32L476RGTX_FLASH.ld
 *
 * Linker script for STM32L476RG: 1 MB Flash starting at the Cortex-M4's
 * default boot address, 128 KB SRAM (96 KB SRAM1 contiguous with 32 KB
 * SRAM2, which is memory-mapped immediately after SRAM1 on this device).
 *
 * Hand-written (no vendor project template shipped one in this repo);
 * follows the standard GNU-ARM-Embedded layout used by CubeIDE-generated
 * scripts, trimmed to what this firmware actually needs (no heap, since
 * this codebase uses no dynamic allocation - see _Min_Heap_Size below).
 */

ENTRY(Reset_Handler)

/* End of RAM = top of the stack, since the stack grows downward from
 * here. Keeping a small reserved heap/stack allowance lets the linker
 * fail the build if .data+.bss ever grow enough to collide with it,
 * instead of silently corrupting memory at runtime. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x0;    /* no malloc/free anywhere in this firmware */
_Min_Stack_Size = 0x400;  /* 1 KB reserved for the main stack */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  /* Cortex-M vector table: must be the very first thing in Flash so the
   * core finds the initial stack pointer and Reset_Handler at boot. */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* Load address (_sidata) stays in FLASH; the AT> clause below is what
   * makes .data have a different LMA (Flash) than VMA (RAM). Reset_Handler
   * in startup_stm32l476xx.s copies _sidata..VMA-length into RAM using
   * exactly these _sdata/_edata/_sidata symbols before main() runs. */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Reserve _Min_Heap_Size + _Min_Stack_Size of RAM above .bss so the
   * build fails at link time (region RAM overflowed) instead of the
   * stack silently colliding with .data/.bss at runtime. */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM
}
