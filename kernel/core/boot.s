/* Aurora OS - Boot Assembly Code */
/* Multiboot header for GRUB bootloader with video mode support */

.set ALIGN,    1<<0                 /* Align loaded modules on page boundaries */
.set MEMINFO,  1<<1                 /* Provide memory map */
.set VIDEO,    1<<2                 /* Request video mode */
.set FLAGS,    ALIGN | MEMINFO | VIDEO  /* Multiboot flags */
.set MAGIC,    0x1BADB002           /* Magic number for multiboot */
.set CHECKSUM, -(MAGIC + FLAGS)     /* Checksum for multiboot */

/* Video mode preferences */
.set MODE_TYPE, 0                   /* 0 = linear framebuffer mode */
.set WIDTH,     1920                /* Preferred width */
.set HEIGHT,    1080                /* Preferred height */
.set DEPTH,     32                  /* Preferred depth (bits per pixel) */

/* Multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
/* Video mode fields (only valid if bit 2 of FLAGS is set) */
.long MODE_TYPE
.long WIDTH
.long HEIGHT
.long DEPTH

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
    
    /* Push multiboot info pointer (from EBX) onto stack */
    pushl %ebx
    
    /* Push multiboot magic number (from EAX) onto stack */
    pushl %eax
    
    /* Call kernel main function with multiboot parameters */
    call kernel_main
    
    /* If kernel_main returns, halt */
    cli
1:  hlt
    jmp 1b

/* Set size of _start symbol */
.size _start, . - _start
