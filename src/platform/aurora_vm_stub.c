/**
 * @file aurora_vm_stub.c
 * @brief Aurora VM Stub Implementation - provides minimal stubs for linking
 */

#include "../../include/platform/aurora_vm.h"
#include "../../include/platform/platform_util.h"

/* Define size_t for freestanding environment */
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

AuroraVM *aurora_vm_create(void) {
    AuroraVM *vm = (AuroraVM *)platform_malloc(sizeof(AuroraVM));
    if (!vm) {
        return (AuroraVM*)0;
    }
    
    platform_memset(vm, 0, sizeof(AuroraVM));
    return vm;
}

void aurora_vm_destroy(AuroraVM *vm) {
    if (!vm) {
        return;
    }
    
    platform_free(vm);
}

int aurora_vm_init(AuroraVM *vm) {
    if (!vm) {
        return -1;
    }
    
    return 0;
}

void aurora_vm_cleanup(AuroraVM *vm) {
    if (!vm) {
        return;
    }
}

int aurora_vm_run(AuroraVM *vm) {
    if (!vm) {
        return -1;
    }
    
    return 0;
}

int aurora_vm_step(AuroraVM *vm) {
    if (!vm) {
        return -1;
    }
    
    return 0;
}

int aurora_vm_load_program(AuroraVM *vm, const uint8_t *data, size_t size, uint32_t addr) {
    if (!vm || !data || size == 0) {
        return -1;
    }
    
    (void)addr;
    
    return 0;
}

int aurora_vm_syscall(AuroraVM *vm, uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    if (!vm) {
        return -1;
    }
    
    (void)syscall_num;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    
    return 0;
}
