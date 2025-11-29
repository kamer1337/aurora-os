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
.long 0                             /* header_addr - not used */
.long 0                             /* load_addr - not used */
.long 0                             /* load_end_addr - not used */
.long 0                             /* bss_end_addr - not used */
.long 0                             /* entry_addr - not used */
.long MODE_TYPE
.long WIDTH
.long HEIGHT
.long DEPTH

/* Stack setup */
.section .bss
.align 16
stack_bottom:
.skip 65536 # 64 KB stack for 64-bit mode
stack_top:

/* Saved multiboot parameters (in .data for persistence) */
.section .data
.align 8
.global saved_multiboot_magic
saved_multiboot_magic:
.quad 0
.global saved_multiboot_info
saved_multiboot_info:
.quad 0

/* Page tables for identity mapping (needed for long mode) */
.section .bss
.align 4096
pml4_table:
.skip 4096
pdpt_table:
.skip 4096
pd_table:
.skip 4096
pt_table:
.skip 4096

/* GDT for 64-bit long mode */
.section .data
.align 16
gdt64:
    .quad 0                          /* Null descriptor */
gdt64_code:
    .quad 0x00209A0000000000         /* Code segment: L=1, D=0, DPL=0, P=1 */
gdt64_data:
    .quad 0x0000920000000000         /* Data segment: DPL=0, P=1 */
gdt64_end:

gdt64_pointer:
    .word gdt64_end - gdt64 - 1      /* GDT limit */
    .quad gdt64                      /* GDT base address */

/* Entry point - GRUB starts us in 32-bit protected mode */
.section .text
.code32
.global _start
.type _start, @function
_start:
    /* Disable interrupts */
    cli
    
    /* Save multiboot parameters (EAX=magic, EBX=info pointer) */
    movl %eax, saved_multiboot_magic
    movl %ebx, saved_multiboot_info
    
    /* Setup initial stack (32-bit) */
    movl $stack_top, %esp
    
    /* Check if CPUID is supported */
    pushfl
    popl %eax
    movl %eax, %ecx
    xorl $0x200000, %eax             /* Flip ID bit */
    pushl %eax
    popfl
    pushfl
    popl %eax
    pushl %ecx                       /* Restore original EFLAGS */
    popfl
    xorl %ecx, %eax
    jz .no_long_mode                 /* If ID bit unchanged, no CPUID */
    
    /* Check for extended CPUID functions */
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax
    jb .no_long_mode
    
    /* Check for long mode support */
    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx           /* Check LM bit */
    jz .no_long_mode
    
    /* Set up identity paging for first 2MB */
    /* Clear page tables (each table is 4KB = 4096 bytes = 1024 dwords) */
    movl $pml4_table, %edi
    xorl %eax, %eax
    movl $1024, %ecx                 /* 1024 dwords = 4096 bytes */
    rep stosl
    
    movl $pdpt_table, %edi
    movl $1024, %ecx
    rep stosl
    
    movl $pd_table, %edi
    movl $1024, %ecx
    rep stosl
    
    /* Set up PML4 -> PDPT */
    movl $pml4_table, %edi
    movl $pdpt_table, %eax
    orl $0x03, %eax                  /* Present + Writable */
    movl %eax, (%edi)
    
    /* Set up PDPT -> PD */
    movl $pdpt_table, %edi
    movl $pd_table, %eax
    orl $0x03, %eax                  /* Present + Writable */
    movl %eax, (%edi)
    
    /* Set up PD with 2MB pages for first 4GB (simple identity mapping) */
    movl $pd_table, %edi
    movl $0x00000083, %eax           /* Present + Writable + PS (2MB page) */
    movl $512, %ecx                  /* 512 entries = 1GB */
.fill_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax             /* Next 2MB page */
    addl $8, %edi
    loop .fill_pd
    
    /* Enable PAE */
    movl %cr4, %eax
    orl $(1 << 5), %eax              /* PAE bit */
    movl %eax, %cr4
    
    /* Load PML4 into CR3 */
    movl $pml4_table, %eax
    movl %eax, %cr3
    
    /* Enable long mode via EFER MSR */
    movl $0xC0000080, %ecx           /* EFER MSR */
    rdmsr
    orl $(1 << 8), %eax              /* LME bit */
    wrmsr
    
    /* Enable paging (this activates long mode) */
    movl %cr0, %eax
    orl $(1 << 31), %eax             /* PG bit */
    movl %eax, %cr0
    
    /* Load 64-bit GDT */
    lgdt gdt64_pointer
    
    /* Far jump to 64-bit code segment */
    ljmp $0x08, $.long_mode_start

.no_long_mode:
    /* CPU doesn't support 64-bit mode - halt */
    movl $0xB8000, %edi
    movl $0x4F4E4F45, (%edi)         /* "NO" in red */
    movl $0x4F204F36, 4(%edi)        /* "64" in red */
    cli
.halt32:
    hlt
    jmp .halt32

/* 64-bit code starts here */
.code64
.long_mode_start:
    /* Reload segment registers with 64-bit data segment */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    /* Setup 64-bit stack */
    movq $stack_top, %rsp
    
    /* Load saved multiboot parameters into registers for kernel_main */
    /* In System V AMD64 ABI: first arg in RDI, second in RSI */
    movq saved_multiboot_magic, %rdi
    movq saved_multiboot_info, %rsi
    
    /* Call kernel main function */
    call kernel_main
    
    /* If kernel_main returns, halt */
    cli
.halt64:
    hlt
    jmp .halt64

/* Set size of _start symbol */
.size _start, . - _start

/* Mark stack as non-executable for security */
.section .note.GNU-stack, "", @progbits
