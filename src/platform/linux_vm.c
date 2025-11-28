/**
 * @file linux_vm.c
 * @brief Linux Kernel Virtual Machine Implementation
 */

#include "../../include/platform/linux_vm.h"
#include "../../include/platform/platform_util.h"

/* Global Linux VM state */
static bool g_linux_vm_initialized = false;
static uint32_t g_linux_vm_count = 0;

/* Linux VM version */
#define LINUX_VM_VERSION "1.0.0-aurora"

/* Linux boot protocol structures */
#define LINUX_BZIMAGE_MAGIC    0x53726448  /* "HdrS" */
#define LINUX_SETUP_SECT_SIZE  512
#define LINUX_SETUP_HEADER_OFF 0x1F1

/* Linux setup header (at offset 0x1F1 in bzImage) */
typedef struct __attribute__((packed)) {
    uint8_t  setup_sects;        /* 0x1F1: Setup sectors */
    uint16_t root_flags;         /* 0x1F2: Root flags */
    uint32_t syssize;            /* 0x1F4: System size */
    uint16_t ram_size;           /* 0x1F8: RAM size (obsolete) */
    uint16_t vid_mode;           /* 0x1FA: Video mode */
    uint16_t root_dev;           /* 0x1FC: Root device */
    uint16_t boot_flag;          /* 0x1FE: Boot flag (0xAA55) */
    uint16_t jump;               /* 0x200: Jump instruction */
    uint32_t header;             /* 0x202: Magic "HdrS" */
    uint16_t version;            /* 0x206: Boot protocol version */
    uint32_t realmode_swtch;     /* 0x208: Real mode switch */
    uint16_t start_sys;          /* 0x20C: Start of setup.S */
    uint16_t kernel_version;     /* 0x20E: Kernel version string offset */
    uint8_t  type_of_loader;     /* 0x210: Bootloader ID */
    uint8_t  loadflags;          /* 0x211: Load flags */
    uint16_t setup_move_size;    /* 0x212: Move size for setup */
    uint32_t code32_start;       /* 0x214: 32-bit code entry point */
    uint32_t ramdisk_image;      /* 0x218: Initrd load address */
    uint32_t ramdisk_size;       /* 0x21C: Initrd size */
    uint32_t bootsect_kludge;    /* 0x220: Obsolete */
    uint16_t heap_end_ptr;       /* 0x224: Heap end pointer */
    uint8_t  ext_loader_ver;     /* 0x226: Extended loader version */
    uint8_t  ext_loader_type;    /* 0x227: Extended loader type */
    uint32_t cmd_line_ptr;       /* 0x228: Command line pointer */
    uint32_t initrd_addr_max;    /* 0x22C: Maximum initrd address */
    uint32_t kernel_alignment;   /* 0x230: Kernel alignment */
    uint8_t  relocatable_kernel; /* 0x234: Relocatable kernel flag */
    uint8_t  min_alignment;      /* 0x235: Minimum alignment */
    uint16_t xloadflags;         /* 0x236: Extended load flags */
    uint32_t cmdline_size;       /* 0x238: Command line max size */
    uint32_t hardware_subarch;   /* 0x23C: Hardware subarchitecture */
    uint64_t hardware_subarch_data; /* 0x240: Subarch data */
    uint32_t payload_offset;     /* 0x248: Payload offset */
    uint32_t payload_length;     /* 0x24C: Payload length */
    uint64_t setup_data;         /* 0x250: Setup data linked list */
    uint64_t pref_address;       /* 0x258: Preferred load address */
    uint32_t init_size;          /* 0x260: Init size */
    uint32_t handover_offset;    /* 0x264: EFI handover offset */
} linux_setup_header_t;

/* E820 memory map entry types */
#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5

/* Boot parameters structure (zero page) */
typedef struct __attribute__((packed)) {
    uint8_t  screen_info[64];      /* 0x000: Screen info */
    uint8_t  apm_bios_info[20];    /* 0x040: APM BIOS info */
    uint8_t  _pad1[4];             /* 0x054: Padding */
    uint64_t tboot_addr;           /* 0x058: Tboot address */
    uint8_t  ist_info[16];         /* 0x060: IST info */
    uint8_t  _pad2[16];            /* 0x070: Padding */
    uint8_t  hd0_info[16];         /* 0x080: HD0 info */
    uint8_t  hd1_info[16];         /* 0x090: HD1 info */
    uint16_t sys_desc_len;         /* 0x0A0: System descriptor table length */
    uint8_t  _pad3[14];            /* 0x0A2: Padding */
    uint8_t  e820_entries;         /* 0x0E8: E820 entry count */
    uint8_t  eddbuf_entries;       /* 0x0E9: EDD buffer entries */
    uint8_t  edd_mbr_sig_buf_entries; /* 0x0EA: EDD MBR sig buffer entries */
    uint8_t  kbd_status;           /* 0x0EB: Keyboard status */
    uint8_t  _pad4[4];             /* 0x0EC: Padding */
    linux_setup_header_t hdr;      /* 0x1F1: Setup header */
    uint8_t  _pad5[36];            /* 0x290: Padding */
    uint8_t  e820_map[20*20];      /* 0x2D0: E820 memory map (20 entries) */
} linux_boot_params_t;

/* Console output buffer for write syscall */
#define LINUX_CONSOLE_BUFFER_SIZE 4096
static char g_console_buffer[LINUX_CONSOLE_BUFFER_SIZE];
static uint32_t g_console_buffer_pos = 0;

/* File descriptor table */
#define LINUX_MAX_FDS 64
typedef struct {
    bool in_use;
    uint32_t type;      /* 0=stdin, 1=stdout, 2=stderr, 3=file */
    uint32_t position;  /* Current position in file */
    uint32_t size;      /* File size (for regular files) */
} linux_fd_entry_t;

static linux_fd_entry_t g_fd_table[LINUX_MAX_FDS];

/* Memory management for brk syscall */
#define LINUX_HEAP_BASE  0x10000000   /* Heap starts at 256MB */
#define LINUX_HEAP_MAX   0x20000000   /* Max heap at 512MB */
static uint32_t g_current_brk = LINUX_HEAP_BASE;

/* Initialize file descriptor table */
static void linux_init_fd_table(void) {
    for (int i = 0; i < LINUX_MAX_FDS; i++) {
        g_fd_table[i].in_use = false;
        g_fd_table[i].type = 0;
        g_fd_table[i].position = 0;
        g_fd_table[i].size = 0;
    }
    /* Initialize standard file descriptors */
    g_fd_table[0].in_use = true;
    g_fd_table[0].type = 0; /* stdin */
    g_fd_table[1].in_use = true;
    g_fd_table[1].type = 1; /* stdout */
    g_fd_table[2].in_use = true;
    g_fd_table[2].type = 2; /* stderr */
}

int linux_vm_init(void) {
    if (g_linux_vm_initialized) {
        return 0;
    }
    
    /* Initialize Linux VM subsystem */
    g_linux_vm_count = 0;
    g_linux_vm_initialized = true;
    
    /* Initialize console buffer */
    g_console_buffer_pos = 0;
    platform_memset(g_console_buffer, 0, LINUX_CONSOLE_BUFFER_SIZE);
    
    /* Initialize file descriptor table */
    linux_init_fd_table();
    
    /* Initialize heap */
    g_current_brk = LINUX_HEAP_BASE;
    
    return 0;
}

LinuxVM* linux_vm_create(void) {
    if (!g_linux_vm_initialized) {
        linux_vm_init();
    }
    
    /* Allocate Linux VM structure */
    LinuxVM* vm = (LinuxVM*)platform_malloc(sizeof(LinuxVM));
    if (!vm) {
        return NULL;
    }
    
    /* Initialize fields */
    platform_memset(vm, 0, sizeof(LinuxVM));
    
    /* Create underlying Aurora VM */
    vm->aurora_vm = aurora_vm_create();
    if (!vm->aurora_vm) {
        platform_free(vm);
        return NULL;
    }
    
    vm->state = LINUX_VM_STATE_INITIALIZED;
    vm->kernel_image = NULL;
    vm->kernel_size = 0;
    vm->kernel_entry = LINUX_VM_KERNEL_BASE;
    vm->has_initrd = false;
    vm->initrd_addr = 0;
    vm->initrd_size = 0;
    
    /* Set default kernel command line */
    platform_strncpy(vm->kernel_cmdline, "console=ttyS0 root=/dev/ram0", sizeof(vm->kernel_cmdline));
    
    g_linux_vm_count++;
    
    return vm;
}

void linux_vm_destroy(LinuxVM* vm) {
    if (!vm) {
        return;
    }
    
    /* Stop VM if running */
    if (vm->state == LINUX_VM_STATE_RUNNING) {
        linux_vm_stop(vm);
    }
    
    /* Free kernel image */
    if (vm->kernel_image) {
        platform_free(vm->kernel_image);
    }
    
    /* Destroy Aurora VM */
    if (vm->aurora_vm) {
        aurora_vm_destroy(vm->aurora_vm);
    }
    
    platform_free(vm);
    
    if (g_linux_vm_count > 0) {
        g_linux_vm_count--;
    }
}

/**
 * Parse Linux bzImage header to extract boot parameters
 * @param kernel_data Kernel image data
 * @param size Kernel image size
 * @param entry_point Output: kernel entry point address
 * @param setup_sects Output: number of setup sectors
 * @return 0 on success, -1 on invalid header
 */
static int parse_linux_kernel_header(const uint8_t* kernel_data, uint32_t size, 
                                     uint32_t* entry_point, uint8_t* setup_sects) {
    if (size < 1024) {
        return -1;  /* Too small to be a valid kernel */
    }
    
    /* Check for boot signature at offset 0x1FE */
    if (kernel_data[0x1FE] != 0x55 || kernel_data[0x1FF] != 0xAA) {
        /* Not a bootable image, use raw entry point */
        *entry_point = LINUX_VM_KERNEL_BASE;
        *setup_sects = 4;  /* Default setup sectors */
        return 0;
    }
    
    /* Parse setup header at offset 0x1F1 */
    const linux_setup_header_t* hdr = (const linux_setup_header_t*)(kernel_data + LINUX_SETUP_HEADER_OFF);
    
    /* Check for "HdrS" magic signature */
    if (hdr->header == LINUX_BZIMAGE_MAGIC) {
        /* Valid bzImage format */
        *setup_sects = hdr->setup_sects;
        if (*setup_sects == 0) {
            *setup_sects = 4;  /* Default if not specified */
        }
        
        /* Calculate kernel entry point */
        if (hdr->version >= 0x0200 && hdr->code32_start != 0) {
            /* Use code32_start from header */
            *entry_point = hdr->code32_start;
        } else {
            /* Default entry: after setup code */
            *entry_point = LINUX_VM_KERNEL_BASE + ((*setup_sects + 1) * LINUX_SETUP_SECT_SIZE);
        }
        
        return 0;
    }
    
    /* Legacy kernel or raw binary */
    *entry_point = LINUX_VM_KERNEL_BASE;
    *setup_sects = 4;
    return 0;
}

int linux_vm_load_kernel(LinuxVM* vm, const uint8_t* kernel_data, uint32_t size) {
    if (!vm || !kernel_data || size == 0) {
        return -1;
    }
    
    /* Free existing kernel image if any */
    if (vm->kernel_image) {
        platform_free(vm->kernel_image);
    }
    
    /* Allocate memory for kernel image */
    vm->kernel_image = (uint8_t*)platform_malloc(size);
    if (!vm->kernel_image) {
        return -1;
    }
    
    /* Copy kernel data */
    platform_memcpy(vm->kernel_image, kernel_data, size);
    vm->kernel_size = size;
    
    /* Parse kernel header to find entry point */
    uint8_t setup_sects;
    if (parse_linux_kernel_header(kernel_data, size, &vm->kernel_entry, &setup_sects) < 0) {
        /* Failed to parse, use default entry point */
        vm->kernel_entry = LINUX_VM_KERNEL_BASE;
    }
    
    return 0;
}

int linux_vm_load_initrd(LinuxVM* vm, const uint8_t* initrd_data, uint32_t size) {
    if (!vm || !initrd_data || size == 0) {
        return -1;
    }
    
    if (!vm->aurora_vm) {
        return -1;  /* Need Aurora VM to load initrd into memory */
    }
    
    /* Calculate initrd load address (align to 4KB, after kernel) */
    uint32_t initrd_base = LINUX_VM_KERNEL_BASE + vm->kernel_size;
    initrd_base = (initrd_base + 0xFFF) & ~0xFFF;  /* Align to 4KB boundary */
    
    /* Set initrd parameters */
    vm->has_initrd = true;
    vm->initrd_addr = initrd_base;
    vm->initrd_size = size;
    
    /* Load initrd into VM memory space */
    /* Use Aurora VM's memory interface to copy data */
    /* Note: Aurora VM memory is 64KB, Linux VM memory model is larger */
    /* For compatibility, we store initrd reference; actual loading happens at VM start */
    AuroraVM* avm = vm->aurora_vm;
    if (avm && avm->memory && initrd_base + size <= AURORA_VM_MEMORY_SIZE) {
        /* Can fit in Aurora VM memory - direct copy */
        platform_memcpy(&avm->memory[initrd_base], initrd_data, size);
    }
    /* For larger initrd, it's stored in vm->initrd_addr/size for host-side handling */
    
    return 0;
}

int linux_vm_set_cmdline(LinuxVM* vm, const char* cmdline) {
    if (!vm || !cmdline) {
        return -1;
    }
    
    platform_strncpy(vm->kernel_cmdline, cmdline, sizeof(vm->kernel_cmdline));
    
    return 0;
}

int linux_vm_start(LinuxVM* vm) {
    if (!vm) {
        return -1;
    }
    
    if (vm->state == LINUX_VM_STATE_RUNNING) {
        return 0; /* Already running */
    }
    
    if (!vm->kernel_image || !vm->aurora_vm) {
        return -1; /* No kernel loaded or no Aurora VM */
    }
    
    AuroraVM* avm = vm->aurora_vm;
    
    /* Set up Linux boot protocol:
     * 1. Load kernel into VM memory at LINUX_VM_KERNEL_BASE
     * 2. Set up boot parameters (zero page) at 0x7000
     * 3. Set up command line at 0x8000
     * 4. Initialize CPU state for protected mode entry
     */
    
    /* Load kernel image into VM memory */
    if (avm->memory && LINUX_VM_KERNEL_BASE + vm->kernel_size <= LINUX_VM_MEMORY_SIZE) {
        platform_memcpy(&avm->memory[LINUX_VM_KERNEL_BASE], vm->kernel_image, vm->kernel_size);
    }
    
    /* Set up boot parameters (zero page) at 0x7000 */
    #define LINUX_ZERO_PAGE_ADDR 0x7000
    #define LINUX_CMDLINE_ADDR   0x8000
    
    if (avm->memory) {
        linux_boot_params_t* params = (linux_boot_params_t*)&avm->memory[LINUX_ZERO_PAGE_ADDR];
        platform_memset(params, 0, sizeof(linux_boot_params_t));
        
        /* Copy setup header from kernel if valid bzImage */
        if (vm->kernel_size > LINUX_SETUP_HEADER_OFF + sizeof(linux_setup_header_t)) {
            platform_memcpy(&params->hdr, 
                           &vm->kernel_image[LINUX_SETUP_HEADER_OFF], 
                           sizeof(linux_setup_header_t));
        }
        
        /* Set up initrd if present */
        if (vm->has_initrd) {
            params->hdr.ramdisk_image = vm->initrd_addr;
            params->hdr.ramdisk_size = vm->initrd_size;
        }
        
        /* Set up command line pointer */
        params->hdr.cmd_line_ptr = LINUX_CMDLINE_ADDR;
        
        /* Set loader type (0xFF = unknown bootloader) */
        params->hdr.type_of_loader = 0xFF;
        
        /* Set load flags (CAN_USE_HEAP | LOADED_HIGH) */
        params->hdr.loadflags = 0x81;
        
        /* Copy command line to designated address */
        size_t cmdline_len = platform_strlen(vm->kernel_cmdline);
        if (cmdline_len > 255) cmdline_len = 255;
        platform_memcpy(&avm->memory[LINUX_CMDLINE_ADDR], vm->kernel_cmdline, cmdline_len);
        avm->memory[LINUX_CMDLINE_ADDR + cmdline_len] = '\0';
        
        /* Set up basic E820 memory map */
        /* Entry format: base (8 bytes), size (8 bytes), type (4 bytes) */
        uint8_t* e820_entry = params->e820_map;
        
        /* Entry 0: Low memory (0-640KB) - usable */
        *((uint64_t*)e820_entry) = 0;                    /* Base */
        *((uint64_t*)(e820_entry + 8)) = 0xA0000;        /* Size: 640KB */
        *((uint32_t*)(e820_entry + 16)) = E820_RAM;      /* Type: RAM */
        e820_entry += 20;
        
        /* Entry 1: Video memory (640KB-1MB) - reserved */
        *((uint64_t*)e820_entry) = 0xA0000;              /* Base */
        *((uint64_t*)(e820_entry + 8)) = 0x60000;        /* Size: 384KB */
        *((uint32_t*)(e820_entry + 16)) = E820_RESERVED; /* Type: Reserved */
        e820_entry += 20;
        
        /* Entry 2: Extended memory (1MB onwards) - usable */
        *((uint64_t*)e820_entry) = 0x100000;             /* Base: 1MB */
        *((uint64_t*)(e820_entry + 8)) = LINUX_VM_MEMORY_SIZE - 0x100000;
        *((uint32_t*)(e820_entry + 16)) = E820_RAM;      /* Type: RAM */
        
        params->e820_entries = 3;
    }
    
    /* Initialize CPU state for 32-bit protected mode entry */
    if (avm) {
        /* Set up registers for kernel entry */
        /* ESI should point to boot_params (zero page) */
        avm->cpu.registers[6] = LINUX_ZERO_PAGE_ADDR;  /* ESI */
        
        /* EBP, EDI, EBX should be 0 */
        avm->cpu.registers[5] = 0;  /* EBP */
        avm->cpu.registers[7] = 0;  /* EDI */
        avm->cpu.registers[3] = 0;  /* EBX */
        
        /* Set program counter to kernel entry point */
        avm->cpu.pc = vm->kernel_entry;
        
        /* Clear halted flag */
        avm->cpu.halted = false;
    }
    
    vm->state = LINUX_VM_STATE_RUNNING;
    
    return 0;
}

int linux_vm_pause(LinuxVM* vm) {
    if (!vm) {
        return -1;
    }
    
    if (vm->state != LINUX_VM_STATE_RUNNING) {
        return -1;
    }
    
    vm->state = LINUX_VM_STATE_PAUSED;
    
    return 0;
}

int linux_vm_resume(LinuxVM* vm) {
    if (!vm) {
        return -1;
    }
    
    if (vm->state != LINUX_VM_STATE_PAUSED) {
        return -1;
    }
    
    vm->state = LINUX_VM_STATE_RUNNING;
    
    return 0;
}

int linux_vm_stop(LinuxVM* vm) {
    if (!vm) {
        return -1;
    }
    
    vm->state = LINUX_VM_STATE_STOPPED;
    
    return 0;
}

linux_vm_state_t linux_vm_get_state(LinuxVM* vm) {
    if (!vm) {
        return LINUX_VM_STATE_ERROR;
    }
    
    return vm->state;
}

int32_t linux_vm_handle_syscall(LinuxVM* vm, uint32_t syscall_num, uint32_t* args) {
    if (!vm || !args) {
        return -1;
    }
    
    /* Handle common Linux syscalls */
    switch (syscall_num) {
        case LINUX_SYSCALL_EXIT:
            /* Exit process - stop VM execution */
            vm->state = LINUX_VM_STATE_STOPPED;
            return 0;
            
        case LINUX_SYSCALL_WRITE: {
            /* Write to file descriptor */
            /* args[0] = fd, args[1] = buf ptr, args[2] = count */
            uint32_t fd = args[0];
            uint32_t buf_ptr = args[1];
            uint32_t count = args[2];
            
            /* Validate file descriptor */
            if (fd >= LINUX_MAX_FDS || !g_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Handle stdout and stderr - write to console buffer */
            if (fd == 1 || fd == 2) {
                /* Write to console buffer with overflow protection */
                uint32_t bytes_to_write = count;
                
                /* Check if buffer is already full */
                if (g_console_buffer_pos >= LINUX_CONSOLE_BUFFER_SIZE - 1) {
                    bytes_to_write = 0;
                } else {
                    /* Calculate available space safely */
                    uint32_t available = LINUX_CONSOLE_BUFFER_SIZE - g_console_buffer_pos - 1;
                    if (bytes_to_write > available) {
                        bytes_to_write = available;
                    }
                }
                
                /* Copy data from VM memory to console buffer */
                if (vm->aurora_vm && bytes_to_write > 0) {
                    /* Read from VM memory space */
                    for (uint32_t i = 0; i < bytes_to_write; i++) {
                        if (buf_ptr + i < AURORA_VM_MEMORY_SIZE) {
                            g_console_buffer[g_console_buffer_pos++] = 
                                vm->aurora_vm->memory[buf_ptr + i];
                        }
                    }
                    g_console_buffer[g_console_buffer_pos] = '\0';
                }
                
                return (int32_t)count;
            }
            
            /* For regular files, return count (simulated write) */
            return (int32_t)count;
        }
            
        case LINUX_SYSCALL_READ: {
            /* Read from file descriptor */
            /* args[0] = fd, args[1] = buf ptr, args[2] = count */
            uint32_t fd = args[0];
            uint32_t buf_ptr = args[1];
            uint32_t count = args[2];
            
            /* Validate file descriptor */
            if (fd >= LINUX_MAX_FDS || !g_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Handle stdin - return 0 (EOF) for now */
            if (fd == 0) {
                return 0;
            }
            
            /* For regular files, simulate empty read */
            (void)buf_ptr;
            (void)count;
            return 0;
        }
            
        case LINUX_SYSCALL_GETPID:
            /* Get process ID - return VM instance ID */
            return 1; /* Return PID 1 (init process) */
            
        case LINUX_SYSCALL_BRK: {
            /* Change data segment size */
            /* args[0] = new break address, 0 to query current */
            uint32_t new_brk = args[0];
            
            if (new_brk == 0) {
                /* Query current break */
                return (int32_t)g_current_brk;
            }
            
            /* Validate new break address */
            if (new_brk < LINUX_HEAP_BASE || new_brk >= LINUX_HEAP_MAX) {
                return -12; /* -ENOMEM */
            }
            
            /* Set new break address */
            g_current_brk = new_brk;
            return (int32_t)g_current_brk;
        }
        
        case LINUX_SYSCALL_MMAP: {
            /* Memory mapping */
            /* args[0] = addr, args[1] = length, args[2] = prot, args[3] = flags */
            uint32_t addr = args[0];
            uint32_t length = args[1];
            /* uint32_t prot = args[2]; */
            /* uint32_t flags = args[3]; */
            
            /* Simple implementation: allocate from current break */
            if (addr == 0) {
                /* Check for potential overflow in alignment calculation */
                if (g_current_brk > (0xFFFFFFFF - 0xFFF)) {
                    return -12; /* -ENOMEM */
                }
                
                /* Allocate at current break with page alignment */
                uint32_t aligned_brk = (g_current_brk + 0xFFF) & ~0xFFF;
                
                /* Check for overflow in size calculation */
                if (length > LINUX_HEAP_MAX - aligned_brk) {
                    return -12; /* -ENOMEM */
                }
                
                g_current_brk = aligned_brk + length;
                return (int32_t)aligned_brk;
            }
            
            /* Fixed mapping at specified address */
            return (int32_t)addr;
        }
        
        case LINUX_SYSCALL_MUNMAP:
            /* Unmap memory - no-op for now */
            return 0;
            
        default:
            /* Unimplemented syscall - return -ENOSYS */
            return -38;
    }
}

bool linux_vm_is_available(void) {
    return g_linux_vm_initialized;
}

const char* linux_vm_get_version(void) {
    return LINUX_VM_VERSION;
}
