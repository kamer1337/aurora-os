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
    
    /* TODO: Parse kernel header to find entry point */
    /* For now, use default entry point */
    vm->kernel_entry = LINUX_VM_KERNEL_BASE;
    
    return 0;
}

int linux_vm_load_initrd(LinuxVM* vm, const uint8_t* initrd_data, uint32_t size) {
    if (!vm || !initrd_data || size == 0) {
        return -1;
    }
    
    /* Set initrd parameters */
    vm->has_initrd = true;
    vm->initrd_addr = LINUX_VM_KERNEL_BASE + vm->kernel_size + 0x1000; /* Align to 4KB */
    vm->initrd_size = size;
    
    /* TODO: Load initrd into VM memory */
    /* This would copy the initrd data to the VM's memory space */
    
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
    
    if (!vm->kernel_image) {
        return -1; /* No kernel loaded */
    }
    
    /* TODO: Set up Linux boot protocol */
    /* This would involve:
     * 1. Setting up boot parameters
     * 2. Loading kernel into VM memory
     * 3. Setting up initial register state
     * 4. Starting VM execution at kernel entry point
     */
    
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
                /* Write to console buffer */
                uint32_t bytes_to_write = count;
                if (g_console_buffer_pos + count >= LINUX_CONSOLE_BUFFER_SIZE) {
                    bytes_to_write = LINUX_CONSOLE_BUFFER_SIZE - g_console_buffer_pos - 1;
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
                /* Allocate at current break */
                uint32_t aligned_brk = (g_current_brk + 0xFFF) & ~0xFFF; /* Page align */
                if (aligned_brk + length >= LINUX_HEAP_MAX) {
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
