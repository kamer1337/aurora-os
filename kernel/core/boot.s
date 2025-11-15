/* Aurora OS - Boot Assembly Code */
/* Multiboot header for GRUB bootloader */

.set ALIGN,    1<<0                 /* Align loaded modules on page boundaries */
.set MEMINFO,  1<<1                 /* Provide memory map */
.set FLAGS,    ALIGN | MEMINFO      /* Multiboot flags */
.set MAGIC,    0x1BADB002           /* Magic number for multiboot */
.set CHECKSUM, -(MAGIC + FLAGS)     /* Checksum for multiboot */

/* Multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack setup */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KB stack
stack_top:

/* Entry point */
.section .text
.global _start
.type _start, @function
_start:
    /* Setup stack pointer */
    mov $stack_top, %esp
    
    /* Reset EFLAGS */
    pushl $0
    popf
    
    /* Call kernel main function */
    call kernel_main
    
    /* If kernel_main returns, halt */
    cli
1:  hlt
    jmp 1b

/* Set size of _start symbol */
.size _start, . - _start
