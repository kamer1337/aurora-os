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

int linux_vm_init(void) {
    if (g_linux_vm_initialized) {
        return 0;
    }
    
    /* Initialize Linux VM subsystem */
    g_linux_vm_count = 0;
    g_linux_vm_initialized = true;
    
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
            /* Exit process */
            return 0;
            
        case LINUX_SYSCALL_WRITE:
            /* Write to file descriptor */
            /* args[0] = fd, args[1] = buf, args[2] = count */
            /* TODO: Implement write syscall */
            return args[2]; /* Return bytes written */
            
        case LINUX_SYSCALL_READ:
            /* Read from file descriptor */
            /* args[0] = fd, args[1] = buf, args[2] = count */
            /* TODO: Implement read syscall */
            return 0;
            
        case LINUX_SYSCALL_GETPID:
            /* Get process ID */
            return 1; /* Return dummy PID */
            
        case LINUX_SYSCALL_BRK:
            /* Change data segment size */
            /* args[0] = new break address */
            /* TODO: Implement brk syscall */
            return args[0];
            
        default:
            /* Unimplemented syscall */
            return -1;
    }
}

bool linux_vm_is_available(void) {
    return g_linux_vm_initialized;
}

const char* linux_vm_get_version(void) {
    return LINUX_VM_VERSION;
}
