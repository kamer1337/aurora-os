/**
 * @file aurora_vm.c
 * @brief Aurora OS Virtual Machine Implementation
 */

#include "../../include/platform/aurora_vm.h"
#include "../../include/platform/platform_util.h"

/* Freestanding string comparison */
static int vm_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* Simple number to string helper for disassembly */
static int vm_int_to_str(int value, char *buffer, int base, int is_signed) {
    char temp[12];
    int i = 0;
    int neg = 0;
    unsigned int uval;
    
    /* Handle signed negative values, with special case for INT_MIN */
    if (is_signed && value < 0) {
        neg = 1;
        /* Cast to unsigned first to avoid UB when negating INT_MIN */
        uval = (unsigned int)(-(value + 1)) + 1;
    } else {
        uval = (unsigned int)value;
    }
    
    if (uval == 0) {
        temp[i++] = '0';
    } else {
        while (uval > 0) {
            int digit = uval % base;
            temp[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            uval /= base;
        }
    }
    
    int pos = 0;
    if (neg) buffer[pos++] = '-';
    while (i > 0) buffer[pos++] = temp[--i];
    buffer[pos] = '\0';
    return pos;
}

/* Simple format helper: copy string and return length */
static int vm_strcpy_ret_len(char *dest, const char *src, size_t max_len) {
    int i = 0;
    while (src[i] && (size_t)i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return i;
}

/* Format R-type instruction: "OPCODE rd, rs1, rs2" */
static int vm_format_r_type(char *buffer, size_t size, const char *op, int rd, int rs1, int rs2) {
    int pos = 0;
    pos += vm_strcpy_ret_len(buffer + pos, op, size - pos);
    buffer[pos++] = ' ';
    buffer[pos++] = 'r';
    pos += vm_int_to_str(rd, buffer + pos, 10, 0);
    buffer[pos++] = ',';
    buffer[pos++] = ' ';
    buffer[pos++] = 'r';
    pos += vm_int_to_str(rs1, buffer + pos, 10, 0);
    buffer[pos++] = ',';
    buffer[pos++] = ' ';
    buffer[pos++] = 'r';
    pos += vm_int_to_str(rs2, buffer + pos, 10, 0);
    buffer[pos] = '\0';
    return pos;
}

/* Format 2-register instruction: "OPCODE rd, rs" */
static int vm_format_2reg(char *buffer, size_t size, const char *op, int rd, int rs) {
    int pos = 0;
    pos += vm_strcpy_ret_len(buffer + pos, op, size - pos);
    buffer[pos++] = ' ';
    buffer[pos++] = 'r';
    pos += vm_int_to_str(rd, buffer + pos, 10, 0);
    buffer[pos++] = ',';
    buffer[pos++] = ' ';
    buffer[pos++] = 'r';
    pos += vm_int_to_str(rs, buffer + pos, 10, 0);
    buffer[pos] = '\0';
    return pos;
}

/* Format I-type instruction: "OPCODE rd, imm" */
static int vm_format_i_type(char *buffer, size_t size, const char *op, int rd, int imm) {
    int pos = 0;
    pos += vm_strcpy_ret_len(buffer + pos, op, size - pos);
    buffer[pos++] = ' ';
    buffer[pos++] = 'r';
    pos += vm_int_to_str(rd, buffer + pos, 10, 0);
    buffer[pos++] = ',';
    buffer[pos++] = ' ';
    pos += vm_int_to_str(imm, buffer + pos, 10, 1);
    buffer[pos] = '\0';
    return pos;
}

/* Format unknown instruction: "UNKNOWN (0xXXXXXXXX)" */
static int vm_format_unknown(char *buffer, size_t size, uint32_t instruction) {
    int pos = 0;
    pos += vm_strcpy_ret_len(buffer + pos, "UNKNOWN (0x", size - pos);
    pos += vm_int_to_str((int)instruction, buffer + pos, 16, 0);
    buffer[pos++] = ')';
    buffer[pos] = '\0';
    return pos;
}

/* Format J-type instruction: "OPCODE 0xaddr" */
static int vm_format_j_type(char *buffer, size_t size, const char *op, int addr) {
    int pos = 0;
    pos += vm_strcpy_ret_len(buffer + pos, op, size - pos);
    buffer[pos++] = ' ';
    buffer[pos++] = '0';
    buffer[pos++] = 'x';
    pos += vm_int_to_str(addr, buffer + pos, 16, 0);
    buffer[pos] = '\0';
    return pos;
}

/* ===== Internal Helper Functions ===== */

/**
 * Decode instruction opcode
 */
static inline aurora_opcode_t decode_opcode(uint32_t instruction) {
    return (aurora_opcode_t)((instruction >> 24) & 0xFF);
}

/**
 * Decode R-type instruction
 */
static inline void decode_r_type(uint32_t instruction, uint8_t *rd, uint8_t *rs1, uint8_t *rs2) {
    *rd = (instruction >> 16) & 0x0F;
    *rs1 = (instruction >> 8) & 0x0F;
    *rs2 = instruction & 0x0F;
}

/**
 * Decode I-type instruction
 */
static inline void decode_i_type(uint32_t instruction, uint8_t *rd, int16_t *imm) {
    *rd = (instruction >> 16) & 0x0F;
    *imm = (int16_t)(instruction & 0xFFFF);
}

/**
 * Decode J-type instruction
 */
static inline void decode_j_type(uint32_t instruction, int32_t *imm) {
    *imm = (int32_t)(instruction & 0x00FFFFFF);
    if (*imm & 0x00800000) {
        *imm |= 0xFF000000;  /* Sign extend */
    }
}

/**
 * Set CPU flags based on result
 */
static void set_flags(aurora_cpu_t *cpu, uint32_t result, bool carry, bool overflow) {
    cpu->flags = 0;
    if (result == 0) cpu->flags |= AURORA_FLAG_ZERO;
    if (result & 0x80000000) cpu->flags |= AURORA_FLAG_NEGATIVE;
    if (carry) cpu->flags |= AURORA_FLAG_CARRY;
    if (overflow) cpu->flags |= AURORA_FLAG_OVERFLOW;
}

/**
 * Check if address is valid for access
 */
static bool check_memory_access(const AuroraVM *vm, uint32_t addr, size_t size, uint8_t required_prot) {
    if (addr + size > AURORA_VM_MEMORY_SIZE) return false;
    
    uint32_t start_page = addr / AURORA_VM_PAGE_SIZE;
    uint32_t end_page = (addr + size - 1) / AURORA_VM_PAGE_SIZE;
    
    for (uint32_t page = start_page; page <= end_page; page++) {
        if (!(vm->pages[page].protection & AURORA_PAGE_PRESENT)) return false;
        if ((vm->pages[page].protection & required_prot) != required_prot) return false;
    }
    
    return true;
}

/**
 * Allocate from heap
 */
static uint32_t heap_alloc(aurora_heap_t *heap, uint32_t size) {
    /* Simple bump allocator */
    if (heap->used + size > heap->size) return 0;
    
    uint32_t addr = heap->base + heap->used;
    heap->used += (size + 3) & ~3;  /* Align to 4 bytes */
    return addr;
}

/**
 * Free memory (simplified - just for API compatibility)
 */
static void heap_free(aurora_heap_t *heap, uint32_t addr) {
    /* In a real implementation, this would maintain a free list */
    /* For simplicity, we just validate the address is in heap range */
    (void)heap;
    (void)addr;
}

/**
 * Find a free file descriptor
 */
static int find_free_fd(aurora_filesystem_t *fs) {
    /* Start from 1 to avoid confusion with stdin/stdout/stderr */
    for (uint32_t i = 1; i < AURORA_VM_MAX_FILES; i++) {
        if (!fs->files[i].open) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * Find an open file by descriptor
 */
static aurora_file_t *get_file(aurora_filesystem_t *fs, int fd) {
    if (fd < 0 || fd >= (int)AURORA_VM_MAX_FILES) return NULL;
    if (!fs->files[fd].open) return NULL;
    return &fs->files[fd];
}

/**
 * Find a file by path
 */
static aurora_file_t *find_file_by_path(aurora_filesystem_t *fs, const char *path) {
    for (uint32_t i = 0; i < AURORA_VM_MAX_FILES; i++) {
        if (fs->files[i].open && vm_strcmp(fs->files[i].path, path) == 0) {
            return &fs->files[i];
        }
    }
    return NULL;
}

/* ===== System Call Implementation ===== */

static int handle_syscall(AuroraVM *vm) {
    uint32_t syscall_num = vm->cpu.registers[0];
    
    switch (syscall_num) {
        case AURORA_SYSCALL_EXIT: {
            vm->exit_code = vm->cpu.registers[1];
            vm->cpu.halted = true;
            return 0;
        }
        
        case AURORA_SYSCALL_PRINT: {
            uint32_t addr = vm->cpu.registers[1];
            uint32_t len = vm->cpu.registers[2];
            
            if (!check_memory_access(vm, addr, len, AURORA_PAGE_READ)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return -1;
            }
            
            /* In freestanding environment, we just validate the data is accessible */
            /* Output would be handled by platform-specific console driver */
            /* For now, mark as successful */
            
            vm->cpu.registers[0] = len;
            return 0;
        }
        
        case AURORA_SYSCALL_READ: {
            uint32_t addr = vm->cpu.registers[1];
            uint32_t max_len = vm->cpu.registers[2];
            
            if (!check_memory_access(vm, addr, max_len, AURORA_PAGE_WRITE)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return -1;
            }
            
            /* In freestanding environment, read would come from keyboard driver */
            /* For now, return 0 bytes read (no input available) */
            vm->cpu.registers[0] = 0;
            return 0;
        }
        
        case AURORA_SYSCALL_GET_TIME: {
            vm->cpu.registers[0] = (uint32_t)vm->timer.ticks;
            return 0;
        }
        
        case AURORA_SYSCALL_SLEEP: {
            uint32_t milliseconds = vm->cpu.registers[1];
            uint64_t ticks = (milliseconds * vm->timer.frequency) / 1000;
            vm->timer.ticks += ticks;
            vm->debugger.cycle_count += ticks;
            return 0;
        }
        
        case AURORA_SYSCALL_ALLOC: {
            uint32_t size = vm->cpu.registers[1];
            uint32_t addr = heap_alloc(&vm->heap, size);
            vm->cpu.registers[0] = addr;
            return 0;
        }
        
        case AURORA_SYSCALL_FREE: {
            uint32_t addr = vm->cpu.registers[1];
            heap_free(&vm->heap, addr);
            vm->cpu.registers[0] = 0;
            return 0;
        }
        
        case AURORA_SYSCALL_PIXEL: {
            uint32_t x = vm->cpu.registers[1];
            uint32_t y = vm->cpu.registers[2];
            uint32_t color = vm->cpu.registers[3];
            
            if (x < AURORA_VM_DISPLAY_WIDTH && y < AURORA_VM_DISPLAY_HEIGHT) {
                aurora_vm_display_set_pixel(vm, x, y, color);
                vm->cpu.registers[0] = 0;
            } else {
                vm->cpu.registers[0] = (uint32_t)-1;
            }
            return 0;
        }
        
        case AURORA_SYSCALL_OPEN: {
            uint32_t path_addr = vm->cpu.registers[1];
            uint32_t mode = vm->cpu.registers[2];
            
            /* Read path from memory */
            char path[AURORA_VM_MAX_FILENAME];
            uint32_t i;
            for (i = 0; i < AURORA_VM_MAX_FILENAME - 1; i++) {
                if (!check_memory_access(vm, path_addr + i, 1, AURORA_PAGE_READ)) {
                    vm->cpu.registers[0] = (uint32_t)-1;
                    return -1;
                }
                path[i] = (char)vm->memory[path_addr + i];
                if (path[i] == '\0') break;
            }
            path[AURORA_VM_MAX_FILENAME - 1] = '\0';
            
            /* Check if file already exists */
            aurora_file_t *existing = find_file_by_path(&vm->filesystem, path);
            
            if (existing && existing->open) {
                /* File already open, return error */
                vm->cpu.registers[0] = (uint32_t)-1;
                return 0;
            }
            
            /* Find free file descriptor */
            int fd = find_free_fd(&vm->filesystem);
            if (fd < 0) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return 0;
            }
            
            /* Allocate storage for the file if it doesn't exist */
            aurora_file_t *file = &vm->filesystem.files[fd];
            if (!existing) {
                /* New file - allocate storage space */
                if (vm->filesystem.storage_used + AURORA_VM_MAX_FILE_SIZE > vm->storage.size) {
                    vm->cpu.registers[0] = (uint32_t)-1;
                    return 0;
                }
                file->storage_offset = vm->filesystem.storage_used;
                vm->filesystem.storage_used += AURORA_VM_MAX_FILE_SIZE;
                file->size = 0;
            } else {
                /* Existing file - reuse storage */
                file->storage_offset = existing->storage_offset;
                file->size = existing->size;
            }
            
            /* Initialize file descriptor */
            platform_strncpy(file->path, path, AURORA_VM_MAX_FILENAME - 1);
            file->path[AURORA_VM_MAX_FILENAME - 1] = '\0';
            file->offset = 0;
            file->mode = (uint8_t)mode;
            file->open = true;
            
            vm->cpu.registers[0] = (uint32_t)fd;
            return 0;
        }
        
        case AURORA_SYSCALL_CLOSE: {
            int fd = (int)vm->cpu.registers[1];
            aurora_file_t *file = get_file(&vm->filesystem, fd);
            
            if (!file) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return 0;
            }
            
            file->open = false;
            vm->cpu.registers[0] = 0;
            return 0;
        }
        
        case AURORA_SYSCALL_READ_FILE: {
            int fd = (int)vm->cpu.registers[1];
            uint32_t buf_addr = vm->cpu.registers[2];
            uint32_t count = vm->cpu.registers[3];
            
            aurora_file_t *file = get_file(&vm->filesystem, fd);
            if (!file || (file->mode != 0 && file->mode != 2)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return 0;
            }
            
            /* Check memory access */
            if (!check_memory_access(vm, buf_addr, count, AURORA_PAGE_WRITE)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return -1;
            }
            
            /* Calculate how much we can read */
            uint32_t available = file->size - file->offset;
            if (available > count) available = count;
            
            /* Copy from storage to VM memory */
            if (available > 0) {
                platform_memcpy(&vm->memory[buf_addr], 
                       &vm->storage.data[file->storage_offset + file->offset],
                       available);
                file->offset += available;
            }
            
            vm->cpu.registers[0] = available;
            return 0;
        }
        
        case AURORA_SYSCALL_WRITE_FILE: {
            int fd = (int)vm->cpu.registers[1];
            uint32_t buf_addr = vm->cpu.registers[2];
            uint32_t count = vm->cpu.registers[3];
            
            aurora_file_t *file = get_file(&vm->filesystem, fd);
            if (!file || (file->mode != 1 && file->mode != 2)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return 0;
            }
            
            /* Check memory access */
            if (!check_memory_access(vm, buf_addr, count, AURORA_PAGE_READ)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return -1;
            }
            
            /* Calculate how much we can write */
            uint32_t space_available = AURORA_VM_MAX_FILE_SIZE - file->offset;
            if (count > space_available) count = space_available;
            
            /* Copy from VM memory to storage */
            if (count > 0) {
                platform_memcpy(&vm->storage.data[file->storage_offset + file->offset],
                       &vm->memory[buf_addr],
                       count);
                file->offset += count;
                if (file->offset > file->size) {
                    file->size = file->offset;
                }
            }
            
            vm->cpu.registers[0] = count;
            return 0;
        }
        
        /* Network syscalls */
        case AURORA_SYSCALL_NET_SEND: {
            uint32_t addr = vm->cpu.registers[1];
            uint32_t len = vm->cpu.registers[2];
            
            if (!check_memory_access(vm, addr, len, AURORA_PAGE_READ)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return -1;
            }
            
            int sent = aurora_vm_net_send(vm, &vm->memory[addr], len);
            vm->cpu.registers[0] = sent;
            return 0;
        }
        
        case AURORA_SYSCALL_NET_RECV: {
            uint32_t addr = vm->cpu.registers[1];
            uint32_t max_len = vm->cpu.registers[2];
            
            if (!check_memory_access(vm, addr, max_len, AURORA_PAGE_WRITE)) {
                vm->cpu.registers[0] = (uint32_t)-1;
                return -1;
            }
            
            int received = aurora_vm_net_recv(vm, &vm->memory[addr], max_len);
            vm->cpu.registers[0] = received;
            return 0;
        }
        
        case AURORA_SYSCALL_NET_CONNECT:
        case AURORA_SYSCALL_NET_LISTEN:
            /* Network connection syscalls - placeholder */
            vm->network.connected = true;
            vm->cpu.registers[0] = 0;
            return 0;
        
        /* Thread syscalls */
        case AURORA_SYSCALL_THREAD_CREATE: {
            uint32_t entry_point = vm->cpu.registers[1];
            uint32_t arg = vm->cpu.registers[2];
            int tid = aurora_vm_thread_create(vm, entry_point, arg);
            vm->cpu.registers[0] = tid;
            return 0;
        }
        
        case AURORA_SYSCALL_THREAD_EXIT:
            vm->scheduler.threads[vm->scheduler.current].active = false;
            aurora_vm_thread_yield(vm);
            return 0;
        
        case AURORA_SYSCALL_THREAD_JOIN: {
            uint32_t tid = vm->cpu.registers[1];
            if (tid < AURORA_VM_MAX_THREADS && vm->scheduler.threads[tid].active) {
                vm->scheduler.threads[vm->scheduler.current].waiting = true;
                vm->scheduler.threads[vm->scheduler.current].wait_target = tid;
                aurora_vm_thread_yield(vm);
            }
            vm->cpu.registers[0] = 0;
            return 0;
        }
        
        case AURORA_SYSCALL_MUTEX_LOCK: {
            /* Mutex lock: r1 = mutex_addr (in VM memory) */
            uint32_t mutex_addr = vm->cpu.registers[1];
            
            /* Validate address bounds */
            if (mutex_addr + sizeof(aurora_mutex_t) > AURORA_VM_MEMORY_SIZE) {
                vm->cpu.registers[0] = (uint32_t)-1;  /* Invalid address */
                return 0;
            }
            
            aurora_mutex_t* mutex = (aurora_mutex_t*)&vm->memory[mutex_addr];
            
            if (!mutex->locked) {
                /* Mutex is free, acquire it */
                mutex->locked = true;
                mutex->owner = vm->scheduler.current;
                vm->cpu.registers[0] = 0;  /* Success */
            } else if (mutex->owner == vm->scheduler.current) {
                /* Already owned by this thread - error (not recursive) */
                vm->cpu.registers[0] = (uint32_t)-2;  /* Deadlock would occur */
            } else {
                /* Mutex is locked by another thread, wait */
                vm->scheduler.threads[vm->scheduler.current].waiting = true;
                vm->scheduler.threads[vm->scheduler.current].wait_target = mutex_addr;
                aurora_vm_thread_yield(vm);
                vm->cpu.registers[0] = 0;  /* Will retry after yield */
            }
            return 0;
        }
        
        case AURORA_SYSCALL_MUTEX_UNLOCK: {
            /* Mutex unlock: r1 = mutex_addr */
            uint32_t mutex_addr = vm->cpu.registers[1];
            
            /* Validate address bounds */
            if (mutex_addr + sizeof(aurora_mutex_t) > AURORA_VM_MEMORY_SIZE) {
                vm->cpu.registers[0] = (uint32_t)-1;  /* Invalid address */
                return 0;
            }
            
            aurora_mutex_t* mutex = (aurora_mutex_t*)&vm->memory[mutex_addr];
            
            if (!mutex->locked) {
                /* Mutex not locked */
                vm->cpu.registers[0] = (uint32_t)-1;  /* Error: not locked */
            } else if (mutex->owner != vm->scheduler.current) {
                /* Not owned by this thread */
                vm->cpu.registers[0] = (uint32_t)-2;  /* Error: not owner */
            } else {
                /* Unlock the mutex */
                mutex->locked = false;
                mutex->owner = 0;
                
                /* Wake up any threads waiting on this mutex */
                for (uint32_t i = 0; i < AURORA_VM_MAX_THREADS; i++) {
                    if (vm->scheduler.threads[i].active &&
                        vm->scheduler.threads[i].waiting &&
                        vm->scheduler.threads[i].wait_target == mutex_addr) {
                        vm->scheduler.threads[i].waiting = false;
                        vm->scheduler.threads[i].wait_target = 0;
                        break;  /* Only wake one waiter */
                    }
                }
                
                vm->cpu.registers[0] = 0;  /* Success */
            }
            return 0;
        }
        
        case AURORA_SYSCALL_SEM_WAIT: {
            /* Semaphore wait (P operation): r1 = sem_addr */
            uint32_t sem_addr = vm->cpu.registers[1];
            
            /* Validate address bounds */
            if (sem_addr + sizeof(aurora_semaphore_t) > AURORA_VM_MEMORY_SIZE) {
                vm->cpu.registers[0] = (uint32_t)-1;  /* Invalid address */
                return 0;
            }
            
            aurora_semaphore_t* sem = (aurora_semaphore_t*)&vm->memory[sem_addr];
            
            if (sem->value > 0) {
                /* Semaphore available, decrement and continue */
                sem->value--;
                vm->cpu.registers[0] = 0;  /* Success */
            } else {
                /* Semaphore not available, add to wait queue */
                if (sem->wait_count < AURORA_VM_MAX_THREADS) {
                    sem->waiting[sem->wait_count++] = vm->scheduler.current;
                }
                vm->scheduler.threads[vm->scheduler.current].waiting = true;
                vm->scheduler.threads[vm->scheduler.current].wait_target = sem_addr;
                aurora_vm_thread_yield(vm);
                vm->cpu.registers[0] = 0;  /* Will check again after wake */
            }
            return 0;
        }
        
        case AURORA_SYSCALL_SEM_POST: {
            /* Semaphore post (V operation): r1 = sem_addr */
            uint32_t sem_addr = vm->cpu.registers[1];
            
            /* Validate address bounds */
            if (sem_addr + sizeof(aurora_semaphore_t) > AURORA_VM_MEMORY_SIZE) {
                vm->cpu.registers[0] = (uint32_t)-1;  /* Invalid address */
                return 0;
            }
            
            aurora_semaphore_t* sem = (aurora_semaphore_t*)&vm->memory[sem_addr];
            
            /* Increment semaphore value */
            sem->value++;
            
            /* Wake up first waiting thread if any */
            if (sem->wait_count > 0) {
                uint32_t tid = sem->waiting[0];
                
                /* Remove from wait queue (shift remaining entries) */
                for (uint32_t i = 0; i < sem->wait_count - 1; i++) {
                    sem->waiting[i] = sem->waiting[i + 1];
                }
                sem->wait_count--;
                
                /* Wake the thread */
                if (tid < AURORA_VM_MAX_THREADS && vm->scheduler.threads[tid].waiting) {
                    vm->scheduler.threads[tid].waiting = false;
                    vm->scheduler.threads[tid].wait_target = 0;
                }
            }
            
            vm->cpu.registers[0] = 0;  /* Success */
            return 0;
        }
        
        default:
            /* Unknown syscall */
            vm->cpu.registers[0] = (uint32_t)-1;
            return -1;
    }
}

/* ===== Instruction Execution ===== */

static int execute_instruction(AuroraVM *vm, uint32_t instruction) {
    aurora_opcode_t opcode = decode_opcode(instruction);
    uint8_t rd, rs1, rs2;
    int16_t imm16;
    int32_t imm32;
    uint32_t result, operand1, operand2, addr;
    uint64_t result64;
    bool carry, overflow;
    
    /* Update cycle count */
    vm->debugger.cycle_count++;
    
    switch (opcode) {
        /* Arithmetic operations */
        case AURORA_OP_ADD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1];
            operand2 = vm->cpu.registers[rs2];
            result = operand1 + operand2;
            carry = result < operand1;
            overflow = ((operand1 ^ result) & (operand2 ^ result) & 0x80000000) != 0;
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, carry, overflow);
            break;
            
        case AURORA_OP_SUB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1];
            operand2 = vm->cpu.registers[rs2];
            result = operand1 - operand2;
            carry = operand1 < operand2;
            overflow = ((operand1 ^ operand2) & (operand1 ^ result) & 0x80000000) != 0;
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, carry, overflow);
            break;
            
        case AURORA_OP_MUL:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result64 = (uint64_t)vm->cpu.registers[rs1] * (uint64_t)vm->cpu.registers[rs2];
            result = (uint32_t)result64;
            vm->cpu.registers[rd] = result;
            carry = (result64 >> 32) != 0;
            set_flags(&vm->cpu, result, carry, false);
            break;
            
        case AURORA_OP_DIV:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            if (vm->cpu.registers[rs2] == 0) {
                /* Division by zero */
                vm->cpu.registers[rd] = 0;
                return -1;
            }
            result = vm->cpu.registers[rs1] / vm->cpu.registers[rs2];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_MOD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            if (vm->cpu.registers[rs2] == 0) {
                /* Modulo by zero */
                vm->cpu.registers[rd] = 0;
                return -1;
            }
            result = vm->cpu.registers[rs1] % vm->cpu.registers[rs2];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_NEG:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = -(int32_t)vm->cpu.registers[rs1];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        /* Logical operations */
        case AURORA_OP_AND:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = vm->cpu.registers[rs1] & vm->cpu.registers[rs2];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_OR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = vm->cpu.registers[rs1] | vm->cpu.registers[rs2];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_XOR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = vm->cpu.registers[rs1] ^ vm->cpu.registers[rs2];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_NOT:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = ~vm->cpu.registers[rs1];
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_SHL:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = vm->cpu.registers[rs1] << (vm->cpu.registers[rs2] & 0x1F);
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_SHR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = vm->cpu.registers[rs1] >> (vm->cpu.registers[rs2] & 0x1F);
            vm->cpu.registers[rd] = result;
            set_flags(&vm->cpu, result, false, false);
            break;
            
        /* Memory operations */
        case AURORA_OP_LOAD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1] + vm->cpu.registers[rs2];
            if (!check_memory_access(vm, operand1, 4, AURORA_PAGE_READ)) return -1;
            vm->cpu.registers[rd] = *(uint32_t *)&vm->memory[operand1];
            break;
            
        case AURORA_OP_STORE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1] + vm->cpu.registers[rs2];
            if (!check_memory_access(vm, operand1, 4, AURORA_PAGE_WRITE)) return -1;
            *(uint32_t *)&vm->memory[operand1] = vm->cpu.registers[rd];
            break;
            
        case AURORA_OP_LOADI:
            decode_i_type(instruction, &rd, &imm16);
            vm->cpu.registers[rd] = (uint32_t)imm16;
            break;
            
        case AURORA_OP_LOADB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1] + vm->cpu.registers[rs2];
            if (!check_memory_access(vm, operand1, 1, AURORA_PAGE_READ)) return -1;
            vm->cpu.registers[rd] = vm->memory[operand1];
            break;
            
        case AURORA_OP_STOREB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1] + vm->cpu.registers[rs2];
            if (!check_memory_access(vm, operand1, 1, AURORA_PAGE_WRITE)) return -1;
            vm->memory[operand1] = (uint8_t)vm->cpu.registers[rd];
            break;
            
        case AURORA_OP_MOVE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            vm->cpu.registers[rd] = vm->cpu.registers[rs1];
            break;
            
        /* Comparison operations */
        case AURORA_OP_CMP:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            operand1 = vm->cpu.registers[rs1];
            operand2 = vm->cpu.registers[rs2];
            result = operand1 - operand2;
            carry = operand1 < operand2;
            overflow = ((operand1 ^ operand2) & (operand1 ^ result) & 0x80000000) != 0;
            set_flags(&vm->cpu, result, carry, overflow);
            break;
            
        case AURORA_OP_TEST:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            result = vm->cpu.registers[rs1] & vm->cpu.registers[rs2];
            set_flags(&vm->cpu, result, false, false);
            break;
            
        case AURORA_OP_SLT:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            vm->cpu.registers[rd] = ((int32_t)vm->cpu.registers[rs1] < (int32_t)vm->cpu.registers[rs2]) ? 1 : 0;
            break;
            
        case AURORA_OP_SLE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            vm->cpu.registers[rd] = ((int32_t)vm->cpu.registers[rs1] <= (int32_t)vm->cpu.registers[rs2]) ? 1 : 0;
            break;
            
        case AURORA_OP_SEQ:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            vm->cpu.registers[rd] = (vm->cpu.registers[rs1] == vm->cpu.registers[rs2]) ? 1 : 0;
            break;
            
        case AURORA_OP_SNE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            vm->cpu.registers[rd] = (vm->cpu.registers[rs1] != vm->cpu.registers[rs2]) ? 1 : 0;
            break;
            
        /* Control flow operations */
        case AURORA_OP_JMP:
            decode_j_type(instruction, &imm32);
            vm->cpu.pc = (uint32_t)imm32;
            return 0;  /* Don't increment PC */
            
        case AURORA_OP_JZ:
            decode_j_type(instruction, &imm32);
            if (vm->cpu.flags & AURORA_FLAG_ZERO) {
                vm->cpu.pc = (uint32_t)imm32;
                return 0;
            }
            break;
            
        case AURORA_OP_JNZ:
            decode_j_type(instruction, &imm32);
            if (!(vm->cpu.flags & AURORA_FLAG_ZERO)) {
                vm->cpu.pc = (uint32_t)imm32;
                return 0;
            }
            break;
            
        case AURORA_OP_JC:
            decode_j_type(instruction, &imm32);
            if (vm->cpu.flags & AURORA_FLAG_CARRY) {
                vm->cpu.pc = (uint32_t)imm32;
                return 0;
            }
            break;
            
        case AURORA_OP_JNC:
            decode_j_type(instruction, &imm32);
            if (!(vm->cpu.flags & AURORA_FLAG_CARRY)) {
                vm->cpu.pc = (uint32_t)imm32;
                return 0;
            }
            break;
            
        case AURORA_OP_CALL:
            decode_j_type(instruction, &imm32);
            /* Push return address */
            vm->cpu.sp -= 4;
            if (!check_memory_access(vm, vm->cpu.sp, 4, AURORA_PAGE_WRITE)) return -1;
            *(uint32_t *)&vm->memory[vm->cpu.sp] = vm->cpu.pc + 4;
            vm->cpu.pc = (uint32_t)imm32;
            return 0;
            
        case AURORA_OP_RET:
            /* Pop return address */
            if (!check_memory_access(vm, vm->cpu.sp, 4, AURORA_PAGE_READ)) return -1;
            vm->cpu.pc = *(uint32_t *)&vm->memory[vm->cpu.sp];
            vm->cpu.sp += 4;
            return 0;
            
        /* System operations */
        case AURORA_OP_SYSCALL:
            return handle_syscall(vm);
            
        case AURORA_OP_HALT:
            vm->cpu.halted = true;
            return 1;
        
        /* Floating-point operations (IEEE 754 single-precision) */
        case AURORA_OP_FADD: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            float f1, f2, result;
            platform_memcpy(&f1, &vm->cpu.registers[rs1], sizeof(float));
            platform_memcpy(&f2, &vm->cpu.registers[rs2], sizeof(float));
            result = f1 + f2;
            platform_memcpy(&vm->cpu.registers[rd], &result, sizeof(float));
            break;
        }
        
        case AURORA_OP_FSUB: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            float f1, f2, result;
            platform_memcpy(&f1, &vm->cpu.registers[rs1], sizeof(float));
            platform_memcpy(&f2, &vm->cpu.registers[rs2], sizeof(float));
            result = f1 - f2;
            platform_memcpy(&vm->cpu.registers[rd], &result, sizeof(float));
            break;
        }
        
        case AURORA_OP_FMUL: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            float f1, f2, result;
            platform_memcpy(&f1, &vm->cpu.registers[rs1], sizeof(float));
            platform_memcpy(&f2, &vm->cpu.registers[rs2], sizeof(float));
            result = f1 * f2;
            platform_memcpy(&vm->cpu.registers[rd], &result, sizeof(float));
            break;
        }
        
        case AURORA_OP_FDIV: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            float f1, f2, result;
            platform_memcpy(&f1, &vm->cpu.registers[rs1], sizeof(float));
            platform_memcpy(&f2, &vm->cpu.registers[rs2], sizeof(float));
            result = f1 / f2;
            platform_memcpy(&vm->cpu.registers[rd], &result, sizeof(float));
            break;
        }
        
        case AURORA_OP_FCMP: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            float f1, f2;
            platform_memcpy(&f1, &vm->cpu.registers[rs1], sizeof(float));
            platform_memcpy(&f2, &vm->cpu.registers[rs2], sizeof(float));
            /* Set flags based on comparison */
            vm->cpu.flags = 0;
            if (f1 == f2) vm->cpu.flags |= AURORA_FLAG_ZERO;
            if (f1 < f2) vm->cpu.flags |= AURORA_FLAG_NEGATIVE;
            break;
        }
        
        case AURORA_OP_FCVT: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            (void)rs2;  /* Unused for this operation */
            float result = (float)(int32_t)vm->cpu.registers[rs1];
            platform_memcpy(&vm->cpu.registers[rd], &result, sizeof(float));
            break;
        }
        
        case AURORA_OP_ICVT: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            (void)rs2;  /* Unused for this operation */
            float f;
            platform_memcpy(&f, &vm->cpu.registers[rs1], sizeof(float));
            vm->cpu.registers[rd] = (uint32_t)(int32_t)f;
            break;
        }
        
        case AURORA_OP_FMOV: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            (void)rs2;  /* Unused for this operation */
            vm->cpu.registers[rd] = vm->cpu.registers[rs1];
            break;
        }
        
        /* SIMD/Vector operations (4x8-bit packed operations) */
        case AURORA_OP_VADD: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            uint32_t v1 = vm->cpu.registers[rs1];
            uint32_t v2 = vm->cpu.registers[rs2];
            uint32_t result = 0;
            /* Add each byte independently */
            for (int i = 0; i < 4; i++) {
                uint8_t b1 = (v1 >> (i * 8)) & 0xFF;
                uint8_t b2 = (v2 >> (i * 8)) & 0xFF;
                uint8_t sum = b1 + b2;
                result |= ((uint32_t)sum << (i * 8));
            }
            vm->cpu.registers[rd] = result;
            break;
        }
        
        case AURORA_OP_VSUB: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            uint32_t v1 = vm->cpu.registers[rs1];
            uint32_t v2 = vm->cpu.registers[rs2];
            uint32_t result = 0;
            /* Subtract each byte independently */
            for (int i = 0; i < 4; i++) {
                uint8_t b1 = (v1 >> (i * 8)) & 0xFF;
                uint8_t b2 = (v2 >> (i * 8)) & 0xFF;
                uint8_t diff = b1 - b2;
                result |= ((uint32_t)diff << (i * 8));
            }
            vm->cpu.registers[rd] = result;
            break;
        }
        
        case AURORA_OP_VMUL: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            uint32_t v1 = vm->cpu.registers[rs1];
            uint32_t v2 = vm->cpu.registers[rs2];
            uint32_t result = 0;
            /* Multiply each byte independently (with truncation) */
            for (int i = 0; i < 4; i++) {
                uint8_t b1 = (v1 >> (i * 8)) & 0xFF;
                uint8_t b2 = (v2 >> (i * 8)) & 0xFF;
                uint8_t prod = (uint8_t)((b1 * b2) & 0xFF);
                result |= ((uint32_t)prod << (i * 8));
            }
            vm->cpu.registers[rd] = result;
            break;
        }
        
        case AURORA_OP_VDOT: {
            decode_r_type(instruction, &rd, &rs1, &rs2);
            uint32_t v1 = vm->cpu.registers[rs1];
            uint32_t v2 = vm->cpu.registers[rs2];
            uint32_t sum = 0;
            /* Compute dot product of 4 bytes */
            for (int i = 0; i < 4; i++) {
                uint8_t b1 = (v1 >> (i * 8)) & 0xFF;
                uint8_t b2 = (v2 >> (i * 8)) & 0xFF;
                sum += b1 * b2;
            }
            vm->cpu.registers[rd] = sum;
            break;
        }
        
        /* Atomic operations */
        case AURORA_OP_XCHG:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            addr = vm->cpu.registers[rs1];
            if (check_memory_access(vm, addr, 4, AURORA_PAGE_READ | AURORA_PAGE_WRITE)) {
                uint32_t temp;
                platform_memcpy(&temp, &vm->memory[addr], 4);
                vm->cpu.registers[rd] = temp;
                platform_memcpy(&vm->memory[addr], &vm->cpu.registers[rs2], 4);
            } else {
                return -1;
            }
            break;
        
        case AURORA_OP_CAS:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            addr = vm->cpu.registers[rs1];
            if (check_memory_access(vm, addr, 4, AURORA_PAGE_READ | AURORA_PAGE_WRITE)) {
                uint32_t current;
                platform_memcpy(&current, &vm->memory[addr], 4);
                if (current == vm->cpu.registers[rd]) {
                    platform_memcpy(&vm->memory[addr], &vm->cpu.registers[rs2], 4);
                    vm->cpu.registers[rd] = 1;  /* Success */
                } else {
                    vm->cpu.registers[rd] = 0;  /* Failed */
                }
            } else {
                return -1;
            }
            break;
        
        case AURORA_OP_FADD_ATOMIC:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            addr = vm->cpu.registers[rs1];
            if (check_memory_access(vm, addr, 4, AURORA_PAGE_READ | AURORA_PAGE_WRITE)) {
                uint32_t old_value;
                platform_memcpy(&old_value, &vm->memory[addr], 4);
                vm->cpu.registers[rd] = old_value;
                uint32_t new_value = old_value + vm->cpu.registers[rs2];
                platform_memcpy(&vm->memory[addr], &new_value, 4);
            } else {
                return -1;
            }
            break;
        
        case AURORA_OP_LOCK:
            /* Lock prefix - just a hint for atomic operations */
            break;
            
        default:
            /* Invalid opcode */
            return -1;
    }
    
    return 0;
}

/* ===== VM API Implementation ===== */

AuroraVM *aurora_vm_create(void) {
    AuroraVM *vm = (AuroraVM *)platform_malloc(sizeof(AuroraVM));
    if (!vm) return NULL;
    
    /* Allocate storage */
    vm->storage.data = (uint8_t *)platform_malloc(AURORA_VM_STORAGE_SIZE);
    if (!vm->storage.data) {
        platform_free(vm);
        return NULL;
    }
    vm->storage.size = AURORA_VM_STORAGE_SIZE;
    
    return vm;
}

int aurora_vm_init(AuroraVM *vm) {
    if (!vm) return -1;
    
    /* Initialize CPU */
    platform_memset(&vm->cpu, 0, sizeof(aurora_cpu_t));
    vm->cpu.pc = 0;
    vm->cpu.sp = AURORA_VM_MEMORY_SIZE - 4;  /* Stack grows downward */
    vm->cpu.fp = vm->cpu.sp;
    vm->cpu.halted = false;
    
    /* Initialize memory - all pages invalid by default */
    platform_memset(vm->memory, 0, AURORA_VM_MEMORY_SIZE);
    platform_memset(vm->pages, 0, sizeof(vm->pages));
    
    /* Set up code section (first 16KB - read/execute) */
    for (uint32_t i = 0; i < 64; i++) {
        vm->pages[i].protection = AURORA_PAGE_READ | AURORA_PAGE_EXEC | AURORA_PAGE_PRESENT;
    }
    
    /* Set up heap section (next 32KB - read/write) */
    vm->heap.base = 16 * 1024;
    vm->heap.size = AURORA_VM_HEAP_SIZE;
    vm->heap.used = 0;
    for (uint32_t i = 64; i < 192; i++) {
        vm->pages[i].protection = AURORA_PAGE_READ | AURORA_PAGE_WRITE | AURORA_PAGE_PRESENT;
    }
    
    /* Set up stack section (last 8KB - read/write) */
    for (uint32_t i = 224; i < 256; i++) {
        vm->pages[i].protection = AURORA_PAGE_READ | AURORA_PAGE_WRITE | AURORA_PAGE_PRESENT;
    }
    
    /* Initialize devices */
    platform_memset(&vm->display, 0, sizeof(aurora_display_t));
    platform_memset(&vm->keyboard, 0, sizeof(aurora_keyboard_t));
    platform_memset(&vm->mouse, 0, sizeof(aurora_mouse_t));
    
    vm->timer.ticks = 0;
    vm->timer.frequency = AURORA_VM_TIMER_FREQ;
    
    platform_memset(vm->storage.data, 0, vm->storage.size);
    
    /* Initialize network device */
    platform_memset(&vm->network, 0, sizeof(aurora_network_t));
    vm->network.connected = false;
    
    /* Initialize file system */
    platform_memset(&vm->filesystem, 0, sizeof(aurora_filesystem_t));
    vm->filesystem.storage_used = 0;
    for (uint32_t i = 0; i < AURORA_VM_MAX_FILES; i++) {
        vm->filesystem.files[i].open = false;
    }
    
    /* Initialize interrupt controller */
    platform_memset(&vm->irq_ctrl, 0, sizeof(aurora_irq_ctrl_t));
    vm->irq_ctrl.enabled = false;
    
    /* Initialize scheduler */
    platform_memset(&vm->scheduler, 0, sizeof(aurora_scheduler_t));
    vm->scheduler.current = 0;
    vm->scheduler.count = 1;  /* Main thread */
    vm->scheduler.threads[0].id = 0;
    vm->scheduler.threads[0].active = true;
    vm->scheduler.threads[0].waiting = false;
    
    /* Initialize JIT compiler */
    platform_memset(&vm->jit, 0, sizeof(aurora_jit_t));
    vm->jit.enabled = AURORA_VM_JIT_ENABLED;
    vm->jit.cache_size = AURORA_VM_JIT_CACHE_SIZE;
    vm->jit.cache_used = 0;
    vm->jit.num_blocks = 0;
    
    /* Allocate JIT cache if JIT is enabled */
    if (vm->jit.enabled) {
        vm->jit.cache = (uint8_t *)platform_malloc(vm->jit.cache_size);
        if (vm->jit.cache) {
            platform_memset(vm->jit.cache, 0, vm->jit.cache_size);
        }
    } else {
        vm->jit.cache = NULL;
    }
    
    /* Initialize GDB server */
    platform_memset(&vm->gdb, 0, sizeof(aurora_gdb_server_t));
    vm->gdb.enabled = false;
    vm->gdb.connected = false;
    vm->gdb.socket_fd = -1;
    
    /* Initialize debugger */
    vm->debugger.enabled = false;
    vm->debugger.single_step = false;
    vm->debugger.num_breakpoints = 0;
    vm->debugger.instruction_count = 0;
    vm->debugger.cycle_count = 0;
    
    /* Runtime state */
    vm->running = false;
    vm->exit_code = 0;
    
    return 0;
}

void aurora_vm_destroy(AuroraVM *vm) {
    if (!vm) return;
    
    if (vm->storage.data) {
        platform_free(vm->storage.data);
    }
    
    if (vm->jit.cache) {
        platform_free(vm->jit.cache);
    }
    
    platform_free(vm);
}

int aurora_vm_load_program(AuroraVM *vm, const uint8_t *program, size_t size, uint32_t addr) {
    if (!vm || !program) return -1;
    if (addr + size > AURORA_VM_MEMORY_SIZE) return -1;
    
    /* Check that target pages are present and writable (or executable for loading) */
    uint32_t start_page = addr / AURORA_VM_PAGE_SIZE;
    uint32_t end_page = (addr + size - 1) / AURORA_VM_PAGE_SIZE;
    
    for (uint32_t page = start_page; page <= end_page; page++) {
        if (!(vm->pages[page].protection & AURORA_PAGE_PRESENT)) return -1;
    }
    
    platform_memcpy(&vm->memory[addr], program, size);
    return 0;
}

int aurora_vm_run(AuroraVM *vm) {
    if (!vm) return -1;
    
    vm->running = true;
    vm->cpu.halted = false;
    
    while (vm->running && !vm->cpu.halted) {
        int result = aurora_vm_step(vm);
        if (result < 0) {
            /* Error occurred */
            vm->running = false;
            return -1;
        }
    }
    
    return vm->exit_code;
}

int aurora_vm_step(AuroraVM *vm) {
    if (!vm || vm->cpu.halted) return 1;
    
    /* Check debugger breakpoints */
    if (vm->debugger.enabled) {
        for (uint32_t i = 0; i < vm->debugger.num_breakpoints; i++) {
            if (vm->debugger.breakpoints[i] == vm->cpu.pc) {
                /* Hit breakpoint */
                return 2;
            }
        }
    }
    
    /* Fetch instruction */
    if (!check_memory_access(vm, vm->cpu.pc, 4, AURORA_PAGE_READ | AURORA_PAGE_EXEC)) {
        return -1;
    }
    
    uint32_t instruction = *(uint32_t *)&vm->memory[vm->cpu.pc];
    
    /* Save PC for potential rollback */
    uint32_t old_pc = vm->cpu.pc;
    
    /* Execute instruction */
    int result = execute_instruction(vm, instruction);
    
    /* Increment instruction counter */
    vm->debugger.instruction_count++;
    
    /* Advance timer */
    vm->timer.ticks++;
    
    /* If PC wasn't changed by instruction, increment it */
    if (vm->cpu.pc == old_pc) {
        vm->cpu.pc += 4;
    }
    
    if (result < 0) {
        /* Execution error */
        return -1;
    }
    
    if (vm->cpu.halted) {
        return 1;
    }
    
    /* Check for pending interrupts and dispatch them */
    if (vm->irq_ctrl.enabled) {
        for (uint32_t i = 0; i < AURORA_VM_MAX_INTERRUPTS; i++) {
            if (vm->irq_ctrl.interrupts[i].pending && 
                vm->irq_ctrl.interrupts[i].enabled &&
                vm->irq_ctrl.interrupts[i].handler != 0) {
                /* Dispatch interrupt - save state and jump to handler */
                vm->cpu.sp -= 4;
                if (check_memory_access(vm, vm->cpu.sp, 4, AURORA_PAGE_WRITE)) {
                    platform_memcpy(&vm->memory[vm->cpu.sp], &vm->cpu.pc, 4);
                    vm->cpu.pc = vm->irq_ctrl.interrupts[i].handler;
                    vm->irq_ctrl.interrupts[i].pending = false;
                    vm->irq_ctrl.active &= ~(1 << i);
                    break;  /* Handle one interrupt per step */
                }
            }
        }
    }
    
    /* Check single-step mode */
    if (vm->debugger.enabled && vm->debugger.single_step) {
        return 2;  /* Paused for debugging */
    }
    
    return 0;
}

void aurora_vm_reset(AuroraVM *vm) {
    if (!vm) return;
    aurora_vm_init(vm);
}

uint32_t aurora_vm_get_register(const AuroraVM *vm, uint32_t reg) {
    if (!vm || reg >= AURORA_VM_NUM_REGISTERS) return 0;
    return vm->cpu.registers[reg];
}

void aurora_vm_set_register(AuroraVM *vm, uint32_t reg, uint32_t value) {
    if (!vm || reg >= AURORA_VM_NUM_REGISTERS) return;
    vm->cpu.registers[reg] = value;
}

int aurora_vm_read_memory(const AuroraVM *vm, uint32_t addr, size_t size, void *buffer) {
    if (!vm || !buffer) return -1;
    
    /* Check if this is an MMIO read */
    if (addr >= AURORA_VM_MMIO_BASE && addr < AURORA_VM_MMIO_BASE + AURORA_VM_MMIO_SIZE) {
        /* MMIO read - dispatch to device handlers */
        /* For now, we return zeros for MMIO reads */
        /* Devices are typically accessed via syscalls */
        platform_memset(buffer, 0, size);
        return (int)size;
    }
    
    if (!check_memory_access(vm, addr, size, AURORA_PAGE_READ)) return -1;
    
    platform_memcpy(buffer, &vm->memory[addr], size);
    return (int)size;
}

int aurora_vm_write_memory(AuroraVM *vm, uint32_t addr, size_t size, const void *buffer) {
    if (!vm || !buffer) return -1;
    
    /* Check if this is an MMIO write */
    if (addr >= AURORA_VM_MMIO_BASE && addr < AURORA_VM_MMIO_BASE + AURORA_VM_MMIO_SIZE) {
        /* MMIO write - dispatch to device handlers */
        /* For now, we accept MMIO writes but don't process them */
        /* Devices are typically accessed via syscalls */
        return (int)size;
    }
    
    if (!check_memory_access(vm, addr, size, AURORA_PAGE_WRITE)) return -1;
    
    platform_memcpy(&vm->memory[addr], buffer, size);
    return (int)size;
}

int aurora_vm_set_page_protection(AuroraVM *vm, uint32_t page, uint8_t protection) {
    if (!vm || page >= AURORA_VM_NUM_PAGES) return -1;
    vm->pages[page].protection = protection;
    return 0;
}

uint8_t aurora_vm_get_page_protection(const AuroraVM *vm, uint32_t page) {
    if (!vm || page >= AURORA_VM_NUM_PAGES) return 0;
    return vm->pages[page].protection;
}

/* ===== Debugger API Implementation ===== */

void aurora_vm_debugger_enable(AuroraVM *vm, bool enabled) {
    if (!vm) return;
    vm->debugger.enabled = enabled;
}

void aurora_vm_debugger_set_single_step(AuroraVM *vm, bool enabled) {
    if (!vm) return;
    vm->debugger.single_step = enabled;
}

int aurora_vm_debugger_add_breakpoint(AuroraVM *vm, uint32_t addr) {
    if (!vm) return -1;
    if (vm->debugger.num_breakpoints >= AURORA_VM_MAX_BREAKPOINTS) return -1;
    
    vm->debugger.breakpoints[vm->debugger.num_breakpoints++] = addr;
    return 0;
}

int aurora_vm_debugger_remove_breakpoint(AuroraVM *vm, uint32_t addr) {
    if (!vm) return -1;
    
    for (uint32_t i = 0; i < vm->debugger.num_breakpoints; i++) {
        if (vm->debugger.breakpoints[i] == addr) {
            /* Shift remaining breakpoints */
            for (uint32_t j = i; j < vm->debugger.num_breakpoints - 1; j++) {
                vm->debugger.breakpoints[j] = vm->debugger.breakpoints[j + 1];
            }
            vm->debugger.num_breakpoints--;
            return 0;
        }
    }
    
    return -1;
}

void aurora_vm_debugger_clear_breakpoints(AuroraVM *vm) {
    if (!vm) return;
    vm->debugger.num_breakpoints = 0;
}

uint64_t aurora_vm_debugger_get_instruction_count(const AuroraVM *vm) {
    if (!vm) return 0;
    return vm->debugger.instruction_count;
}

uint64_t aurora_vm_debugger_get_cycle_count(const AuroraVM *vm) {
    if (!vm) return 0;
    return vm->debugger.cycle_count;
}

int aurora_vm_disassemble(uint32_t instruction, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    aurora_opcode_t opcode = decode_opcode(instruction);
    uint8_t rd, rs1, rs2;
    int16_t imm16;
    int32_t imm32;
    int written = 0;
    
    switch (opcode) {
        case AURORA_OP_ADD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "ADD", rd, rs1, rs2);
            break;
        case AURORA_OP_SUB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SUB", rd, rs1, rs2);
            break;
        case AURORA_OP_MUL:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "MUL", rd, rs1, rs2);
            break;
        case AURORA_OP_DIV:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "DIV", rd, rs1, rs2);
            break;
        case AURORA_OP_MOD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "MOD", rd, rs1, rs2);
            break;
        case AURORA_OP_NEG:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_2reg(buffer, buffer_size, "NEG", rd, rs1);
            break;
        case AURORA_OP_AND:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "AND", rd, rs1, rs2);
            break;
        case AURORA_OP_OR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "OR", rd, rs1, rs2);
            break;
        case AURORA_OP_XOR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "XOR", rd, rs1, rs2);
            break;
        case AURORA_OP_NOT:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_2reg(buffer, buffer_size, "NOT", rd, rs1);
            break;
        case AURORA_OP_SHL:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SHL", rd, rs1, rs2);
            break;
        case AURORA_OP_SHR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SHR", rd, rs1, rs2);
            break;
        case AURORA_OP_LOAD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "LOAD", rd, rs1, rs2);
            break;
        case AURORA_OP_STORE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "STORE", rs1, rs2, rd);
            break;
        case AURORA_OP_LOADI:
            decode_i_type(instruction, &rd, &imm16);
            written = vm_format_i_type(buffer, buffer_size, "LOADI", rd, imm16);
            break;
        case AURORA_OP_LOADB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "LOADB", rd, rs1, rs2);
            break;
        case AURORA_OP_STOREB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "STOREB", rs1, rs2, rd);
            break;
        case AURORA_OP_MOVE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_2reg(buffer, buffer_size, "MOVE", rd, rs1);
            break;
        case AURORA_OP_CMP:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_2reg(buffer, buffer_size, "CMP", rs1, rs2);
            break;
        case AURORA_OP_TEST:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_2reg(buffer, buffer_size, "TEST", rs1, rs2);
            break;
        case AURORA_OP_SLT:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SLT", rd, rs1, rs2);
            break;
        case AURORA_OP_SLE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SLE", rd, rs1, rs2);
            break;
        case AURORA_OP_SEQ:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SEQ", rd, rs1, rs2);
            break;
        case AURORA_OP_SNE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = vm_format_r_type(buffer, buffer_size, "SNE", rd, rs1, rs2);
            break;
        case AURORA_OP_JMP:
            decode_j_type(instruction, &imm32);
            written = vm_format_j_type(buffer, buffer_size, "JMP", imm32);
            break;
        case AURORA_OP_JZ:
            decode_j_type(instruction, &imm32);
            written = vm_format_j_type(buffer, buffer_size, "JZ", imm32);
            break;
        case AURORA_OP_JNZ:
            decode_j_type(instruction, &imm32);
            written = vm_format_j_type(buffer, buffer_size, "JNZ", imm32);
            break;
        case AURORA_OP_JC:
            decode_j_type(instruction, &imm32);
            written = vm_format_j_type(buffer, buffer_size, "JC", imm32);
            break;
        case AURORA_OP_JNC:
            decode_j_type(instruction, &imm32);
            written = vm_format_j_type(buffer, buffer_size, "JNC", imm32);
            break;
        case AURORA_OP_CALL:
            decode_j_type(instruction, &imm32);
            written = vm_format_j_type(buffer, buffer_size, "CALL", imm32);
            break;
        case AURORA_OP_RET:
            written = vm_strcpy_ret_len(buffer, "RET", buffer_size);
            break;
        case AURORA_OP_SYSCALL:
            written = vm_strcpy_ret_len(buffer, "SYSCALL", buffer_size);
            break;
        case AURORA_OP_HALT:
            written = vm_strcpy_ret_len(buffer, "HALT", buffer_size);
            break;
        default:
            written = vm_format_unknown(buffer, buffer_size, instruction);
            break;
    }
    
    return written;
}

/* ===== Instruction Encoding ===== */

uint32_t aurora_encode_r_type(aurora_opcode_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    return ((uint32_t)opcode << 24) | ((rd & 0x0F) << 16) | ((rs1 & 0x0F) << 8) | (rs2 & 0x0F);
}

uint32_t aurora_encode_i_type(aurora_opcode_t opcode, uint8_t rd, int16_t imm) {
    return ((uint32_t)opcode << 24) | ((rd & 0x0F) << 16) | (imm & 0xFFFF);
}

uint32_t aurora_encode_j_type(aurora_opcode_t opcode, int32_t imm) {
    return ((uint32_t)opcode << 24) | (imm & 0x00FFFFFF);
}

/* ===== Device API Implementation ===== */

uint32_t aurora_vm_display_get_pixel(const AuroraVM *vm, uint32_t x, uint32_t y) {
    if (!vm || x >= AURORA_VM_DISPLAY_WIDTH || y >= AURORA_VM_DISPLAY_HEIGHT) return 0;
    return vm->display.pixels[y * AURORA_VM_DISPLAY_WIDTH + x];
}

void aurora_vm_display_set_pixel(AuroraVM *vm, uint32_t x, uint32_t y, uint32_t color) {
    if (!vm || x >= AURORA_VM_DISPLAY_WIDTH || y >= AURORA_VM_DISPLAY_HEIGHT) return;
    vm->display.pixels[y * AURORA_VM_DISPLAY_WIDTH + x] = color;
    vm->display.dirty = true;
}

bool aurora_vm_keyboard_is_key_pressed(const AuroraVM *vm, uint8_t key) {
    if (!vm) return false;
    return vm->keyboard.keys[key];
}

void aurora_vm_keyboard_set_key(AuroraVM *vm, uint8_t key, bool pressed) {
    if (!vm) return;
    vm->keyboard.keys[key] = pressed;
    
    /* Add to buffer if pressed */
    if (pressed) {
        uint32_t next_head = (vm->keyboard.buffer_head + 1) % 256;
        if (next_head != vm->keyboard.buffer_tail) {
            vm->keyboard.buffer[vm->keyboard.buffer_head] = key;
            vm->keyboard.buffer_head = next_head;
        }
    }
}

void aurora_vm_mouse_get_position(const AuroraVM *vm, int32_t *x, int32_t *y) {
    if (!vm || !x || !y) return;
    *x = vm->mouse.x;
    *y = vm->mouse.y;
}

void aurora_vm_mouse_set_position(AuroraVM *vm, int32_t x, int32_t y) {
    if (!vm) return;
    vm->mouse.x = x;
    vm->mouse.y = y;
}

uint8_t aurora_vm_mouse_get_buttons(const AuroraVM *vm) {
    if (!vm) return 0;
    return vm->mouse.buttons;
}

void aurora_vm_mouse_set_buttons(AuroraVM *vm, uint8_t buttons) {
    if (!vm) return;
    vm->mouse.buttons = buttons;
}

uint64_t aurora_vm_timer_get_ticks(const AuroraVM *vm) {
    if (!vm) return 0;
    return vm->timer.ticks;
}

void aurora_vm_timer_advance(AuroraVM *vm, uint64_t ticks) {
    if (!vm) return;
    vm->timer.ticks += ticks;
}

int aurora_vm_storage_read(const AuroraVM *vm, uint32_t offset, void *buffer, size_t size) {
    if (!vm || !buffer) return -1;
    if (offset + size > vm->storage.size) return -1;
    
    platform_memcpy(buffer, &vm->storage.data[offset], size);
    return (int)size;
}

int aurora_vm_storage_write(AuroraVM *vm, uint32_t offset, const void *buffer, size_t size) {
    if (!vm || !buffer) return -1;
    if (offset + size > vm->storage.size) return -1;
    
    platform_memcpy(&vm->storage.data[offset], buffer, size);
    return (int)size;
}

/* ===== Interrupt API Implementation ===== */

void aurora_vm_irq_enable(AuroraVM *vm, bool enabled) {
    if (!vm) return;
    vm->irq_ctrl.enabled = enabled;
}

int aurora_vm_irq_set_handler(AuroraVM *vm, uint32_t irq, uint32_t handler) {
    if (!vm || irq >= AURORA_VM_MAX_INTERRUPTS) return -1;
    vm->irq_ctrl.interrupts[irq].handler = handler;
    vm->irq_ctrl.interrupts[irq].enabled = true;
    return 0;
}

int aurora_vm_irq_trigger(AuroraVM *vm, uint32_t irq) {
    if (!vm || irq >= AURORA_VM_MAX_INTERRUPTS) return -1;
    if (!vm->irq_ctrl.enabled || !vm->irq_ctrl.interrupts[irq].enabled) return -1;
    
    /* Mark interrupt as pending - will be dispatched in next VM step */
    vm->irq_ctrl.interrupts[irq].pending = true;
    vm->irq_ctrl.active |= (1 << irq);
    
    return 0;
}

/* ===== Network API Implementation ===== */

int aurora_vm_net_send(AuroraVM *vm, const uint8_t *data, uint32_t length) {
    if (!vm || !data || length > AURORA_VM_NET_MTU) return -1;
    if (!vm->network.connected) return -1;
    
    uint32_t next = (vm->network.tx_head + 1) % AURORA_VM_NET_QUEUE_SIZE;
    if (next == vm->network.tx_tail) return -1;  /* Queue full */
    
    platform_memcpy(vm->network.tx_queue[vm->network.tx_head].data, data, length);
    vm->network.tx_queue[vm->network.tx_head].length = length;
    vm->network.tx_head = next;
    
    /* Trigger network interrupt */
    aurora_vm_irq_trigger(vm, AURORA_VM_IRQ_NETWORK);
    
    return (int)length;
}

int aurora_vm_net_recv(AuroraVM *vm, uint8_t *buffer, uint32_t max_length) {
    if (!vm || !buffer) return -1;
    if (vm->network.rx_head == vm->network.rx_tail) return 0;  /* Queue empty */
    
    uint32_t length = vm->network.rx_queue[vm->network.rx_tail].length;
    if (length > max_length) length = max_length;
    
    platform_memcpy(buffer, vm->network.rx_queue[vm->network.rx_tail].data, length);
    vm->network.rx_tail = (vm->network.rx_tail + 1) % AURORA_VM_NET_QUEUE_SIZE;
    
    return (int)length;
}

bool aurora_vm_net_is_connected(const AuroraVM *vm) {
    if (!vm) return false;
    return vm->network.connected;
}

/* ===== Thread API Implementation ===== */

int aurora_vm_thread_create(AuroraVM *vm, uint32_t entry_point, uint32_t arg) {
    if (!vm || vm->scheduler.count >= AURORA_VM_MAX_THREADS) return -1;
    
    uint32_t tid = vm->scheduler.count++;
    aurora_thread_t *thread = &vm->scheduler.threads[tid];
    
    platform_memset(thread, 0, sizeof(aurora_thread_t));
    thread->id = tid;
    thread->active = true;
    thread->waiting = false;
    thread->pc = entry_point;
    thread->sp = AURORA_VM_THREAD_STACK_SIZE - 4;
    thread->fp = thread->sp;
    thread->registers[1] = arg;  /* First argument */
    
    return (int)tid;
}

uint32_t aurora_vm_thread_current(const AuroraVM *vm) {
    if (!vm) return 0;
    return vm->scheduler.current;
}

void aurora_vm_thread_yield(AuroraVM *vm) {
    if (!vm || vm->scheduler.count <= 1) return;
    
    /* Save current thread state */
    uint32_t current = vm->scheduler.current;
    aurora_thread_t *curr_thread = &vm->scheduler.threads[current];
    
    if (curr_thread->active) {
        platform_memcpy(curr_thread->registers, vm->cpu.registers, sizeof(vm->cpu.registers));
        curr_thread->pc = vm->cpu.pc;
        curr_thread->sp = vm->cpu.sp;
        curr_thread->fp = vm->cpu.fp;
        curr_thread->flags = vm->cpu.flags;
    }
    
    /* Find next runnable thread */
    uint32_t next = (current + 1) % vm->scheduler.count;
    while (next != current) {
        if (vm->scheduler.threads[next].active && !vm->scheduler.threads[next].waiting) {
            break;
        }
        next = (next + 1) % vm->scheduler.count;
    }
    
    /* Restore next thread state */
    vm->scheduler.current = next;
    aurora_thread_t *next_thread = &vm->scheduler.threads[next];
    
    platform_memcpy(vm->cpu.registers, next_thread->registers, sizeof(vm->cpu.registers));
    vm->cpu.pc = next_thread->pc;
    vm->cpu.sp = next_thread->sp;
    vm->cpu.fp = next_thread->fp;
    vm->cpu.flags = next_thread->flags;
}

/* ===== JIT API Implementation ===== */

void aurora_vm_jit_enable(AuroraVM *vm, bool enabled) {
    if (!vm) return;
    vm->jit.enabled = enabled;
    
    if (enabled && !vm->jit.cache) {
        /* Lazy allocate JIT cache */
        vm->jit.cache = (uint8_t *)platform_malloc(vm->jit.cache_size);
        if (vm->jit.cache) {
            platform_memset(vm->jit.cache, 0, vm->jit.cache_size);
        }
    }
}

/* 
 * Generate native x86 code for a basic block of Aurora VM instructions.
 * This is a simplified JIT that generates native code for common patterns.
 * 
 * Register mapping:
 *   EAX - scratch / return value
 *   EBX - VM memory base pointer (preserved)
 *   ECX - scratch
 *   EDX - scratch
 *   ESI - VM registers base pointer (preserved)
 *   EDI - scratch
 */
static int jit_generate_native(AuroraVM *vm, uint32_t start_addr, uint32_t *instructions, 
                                uint32_t num_instr, uint8_t *native_buf, uint32_t buf_size) {
    if (!vm || !instructions || !native_buf || buf_size < 64) return -1;
    
    uint32_t pos = 0;
    
    /* Function prologue */
    native_buf[pos++] = 0x55;                   /* push ebp */
    native_buf[pos++] = 0x89; native_buf[pos++] = 0xe5;  /* mov ebp, esp */
    native_buf[pos++] = 0x53;                   /* push ebx */
    native_buf[pos++] = 0x56;                   /* push esi */
    native_buf[pos++] = 0x57;                   /* push edi */
    
    /* Load VM pointers (would come from function arguments in real implementation) */
    /* For now, just generate placeholder code */
    
    for (uint32_t i = 0; i < num_instr && pos < buf_size - 32; i++) {
        uint32_t instr = instructions[i];
        uint8_t opcode = (instr >> 24) & 0xFF;
        uint8_t rd = (instr >> 20) & 0x0F;
        uint8_t rs1 = (instr >> 16) & 0x0F;
        uint8_t rs2 = (instr >> 12) & 0x0F;
        int16_t imm = (int16_t)(instr & 0xFFFF);
        
        (void)rd; (void)rs1; (void)rs2; (void)imm;  /* Suppress warnings */
        
        switch (opcode) {
            case 0xFF:  /* Reserved/NOP - handle as no-op */
                native_buf[pos++] = 0x90;  /* nop */
                break;
                
            case AURORA_OP_ADD:
                /* mov eax, [esi + rs1*4] */
                native_buf[pos++] = 0x8b;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rs1 * 4;
                /* add eax, [esi + rs2*4] */
                native_buf[pos++] = 0x03;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rs2 * 4;
                /* mov [esi + rd*4], eax */
                native_buf[pos++] = 0x89;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rd * 4;
                break;
                
            case AURORA_OP_SUB:
                /* mov eax, [esi + rs1*4] */
                native_buf[pos++] = 0x8b;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rs1 * 4;
                /* sub eax, [esi + rs2*4] */
                native_buf[pos++] = 0x2b;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rs2 * 4;
                /* mov [esi + rd*4], eax */
                native_buf[pos++] = 0x89;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rd * 4;
                break;
                
            case AURORA_OP_LOADI:
                /* mov dword [esi + rd*4], imm */
                native_buf[pos++] = 0xc7;
                native_buf[pos++] = 0x44;
                native_buf[pos++] = 0x86;
                native_buf[pos++] = rd * 4;
                native_buf[pos++] = imm & 0xFF;
                native_buf[pos++] = (imm >> 8) & 0xFF;
                native_buf[pos++] = (imm < 0) ? 0xFF : 0x00;
                native_buf[pos++] = (imm < 0) ? 0xFF : 0x00;
                break;
                
            case AURORA_OP_HALT:
                /* Return from JIT code */
                goto epilogue;
                
            default:
                /* Unsupported opcode - bail out and interpret */
                pos = 0;
                goto epilogue;
        }
    }

epilogue:
    /* Function epilogue */
    if (pos > 0) {
        native_buf[pos++] = 0x5f;  /* pop edi */
        native_buf[pos++] = 0x5e;  /* pop esi */
        native_buf[pos++] = 0x5b;  /* pop ebx */
        native_buf[pos++] = 0x5d;  /* pop ebp */
        native_buf[pos++] = 0xc3;  /* ret */
    }
    
    (void)start_addr;  /* Used for debugging/profiling in full implementation */
    return (int)pos;
}

int aurora_vm_jit_compile_block(AuroraVM *vm, uint32_t addr) {
    if (!vm || !vm->jit.enabled || !vm->jit.cache) return -1;
    if (vm->jit.num_blocks >= 256) return -1;
    if (addr >= AURORA_VM_MEMORY_SIZE - 64) return -1;
    
    /* Check if block is already compiled */
    for (uint32_t i = 0; i < vm->jit.num_blocks; i++) {
        if (vm->jit.blocks[i].start_addr == addr && vm->jit.blocks[i].compiled) {
            return 0;  /* Already compiled */
        }
    }
    
    /* Scan for end of basic block (HALT, JMP, or branch instruction) */
    uint32_t instr_count = 0;
    uint32_t instructions[64];
    uint32_t scan_addr = addr;
    
    while (scan_addr < AURORA_VM_MEMORY_SIZE - 4 && instr_count < 64) {
        uint32_t instr = 0;
        instr |= vm->memory[scan_addr] << 24;
        instr |= vm->memory[scan_addr + 1] << 16;
        instr |= vm->memory[scan_addr + 2] << 8;
        instr |= vm->memory[scan_addr + 3];
        
        instructions[instr_count++] = instr;
        scan_addr += 4;
        
        uint8_t opcode = (instr >> 24) & 0xFF;
        /* End basic block at control flow instructions */
        if (opcode == AURORA_OP_HALT || opcode == AURORA_OP_JMP ||
            opcode == AURORA_OP_JZ || opcode == AURORA_OP_JNZ ||
            opcode == AURORA_OP_CALL || opcode == AURORA_OP_RET) {
            break;
        }
    }
    
    if (instr_count == 0) return -1;
    
    /* Find space in JIT cache */
    uint32_t required_size = instr_count * 16 + 32;  /* Estimate: 16 bytes per instruction + prologue/epilogue */
    if (vm->jit.cache_used + required_size > vm->jit.cache_size) {
        /* Cache full - clear and restart */
        aurora_vm_jit_clear_cache(vm);
    }
    
    /* Generate native code */
    uint8_t *native_ptr = vm->jit.cache + vm->jit.cache_used;
    int native_len = jit_generate_native(vm, addr, instructions, instr_count, 
                                          native_ptr, vm->jit.cache_size - vm->jit.cache_used);
    
    if (native_len <= 0) {
        /* JIT compilation failed - block will be interpreted */
        return -1;
    }
    
    /* Record compiled block */
    aurora_jit_block_t *block = &vm->jit.blocks[vm->jit.num_blocks];
    block->start_addr = addr;
    block->length = instr_count * 4;
    block->native_code = native_ptr;
    block->native_length = (uint32_t)native_len;
    block->exec_count = 0;
    block->compiled = true;
    
    vm->jit.cache_used += (uint32_t)native_len;
    vm->jit.num_blocks++;
    
    return 0;
}

void aurora_vm_jit_clear_cache(AuroraVM *vm) {
    if (!vm) return;
    
    if (vm->jit.cache) {
        platform_memset(vm->jit.cache, 0, vm->jit.cache_size);
    }
    vm->jit.cache_used = 0;
    vm->jit.num_blocks = 0;
    platform_memset(vm->jit.blocks, 0, sizeof(vm->jit.blocks));
}

/* ===== GDB Server API Implementation ===== */

/* GDB Remote Serial Protocol (RSP) helper functions */
static uint8_t gdb_hex_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static char gdb_nibble_to_hex(uint8_t nibble) {
    const char hex[] = "0123456789abcdef";
    return hex[nibble & 0x0F];
}

static uint8_t gdb_calculate_checksum(const char *data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum += (uint8_t)data[i];
    }
    return checksum;
}

/* Format GDB RSP packet: $data#checksum */
static int gdb_format_packet(const char *data, char *out_buf, size_t buf_size) {
    size_t data_len = 0;
    while (data[data_len]) data_len++;
    
    if (data_len + 5 > buf_size) return -1;  /* $...#XX */
    
    out_buf[0] = '$';
    for (size_t i = 0; i < data_len; i++) {
        out_buf[i + 1] = data[i];
    }
    out_buf[data_len + 1] = '#';
    
    uint8_t checksum = gdb_calculate_checksum(data, data_len);
    out_buf[data_len + 2] = gdb_nibble_to_hex(checksum >> 4);
    out_buf[data_len + 3] = gdb_nibble_to_hex(checksum & 0x0F);
    out_buf[data_len + 4] = '\0';
    
    return (int)(data_len + 4);
}

/* Format register value as hex string (little-endian) */
static int gdb_format_reg32(uint32_t value, char *buf) {
    for (int i = 0; i < 4; i++) {
        uint8_t byte = (value >> (i * 8)) & 0xFF;
        buf[i * 2] = gdb_nibble_to_hex(byte >> 4);
        buf[i * 2 + 1] = gdb_nibble_to_hex(byte & 0x0F);
    }
    return 8;
}

/* Parse GDB RSP packet and handle command */
static int gdb_handle_packet(AuroraVM *vm, const char *packet, char *response, size_t resp_size) {
    if (!packet || !response || resp_size < 4) return -1;
    
    /* Find packet start */
    while (*packet && *packet != '$') packet++;
    if (*packet != '$') return -1;
    packet++;
    
    /* Identify command */
    char cmd = packet[0];
    
    switch (cmd) {
        case '?':
            /* Stop reason query - return SIGTRAP (breakpoint) */
            platform_strncpy(response, "S05", resp_size);
            break;
            
        case 'g':
            /* Read all registers */
            {
                char *ptr = response;
                size_t remaining = resp_size - 1;
                
                /* Format all 16 registers */
                for (int i = 0; i < 16 && remaining >= 8; i++) {
                    int len = gdb_format_reg32(vm->cpu.registers[i], ptr);
                    ptr += len;
                    remaining -= len;
                }
                
                /* PC (as EIP for x86 GDB) */
                if (remaining >= 8) {
                    gdb_format_reg32(vm->cpu.pc, ptr);
                    ptr += 8;
                }
                
                *ptr = '\0';
            }
            break;
            
        case 'G':
            /* Write all registers */
            {
                const char *ptr = packet + 1;
                for (int i = 0; i < 16; i++) {
                    uint32_t value = 0;
                    for (int j = 0; j < 8; j += 2) {
                        uint8_t byte = (gdb_hex_to_nibble(ptr[j]) << 4) | gdb_hex_to_nibble(ptr[j + 1]);
                        value |= ((uint32_t)byte) << ((j / 2) * 8);
                    }
                    vm->cpu.registers[i] = value;
                    ptr += 8;
                }
                platform_strncpy(response, "OK", resp_size);
            }
            break;
            
        case 'm':
            /* Read memory: maddr,length */
            {
                uint32_t addr = 0, len = 0;
                const char *p = packet + 1;
                
                /* Parse address */
                while (*p && *p != ',' && *p != '#') {
                    addr = (addr << 4) | gdb_hex_to_nibble(*p++);
                }
                if (*p == ',') p++;
                
                /* Parse length */
                while (*p && *p != '#') {
                    len = (len << 4) | gdb_hex_to_nibble(*p++);
                }
                
                /* Validate and read memory */
                if (addr + len <= AURORA_VM_MEMORY_SIZE && len * 2 < resp_size) {
                    char *out = response;
                    for (uint32_t i = 0; i < len; i++) {
                        uint8_t byte = vm->memory[addr + i];
                        *out++ = gdb_nibble_to_hex(byte >> 4);
                        *out++ = gdb_nibble_to_hex(byte & 0x0F);
                    }
                    *out = '\0';
                } else {
                    platform_strncpy(response, "E01", resp_size);  /* Error */
                }
            }
            break;
            
        case 'M':
            /* Write memory: Maddr,length:data */
            {
                uint32_t addr = 0, len = 0;
                const char *p = packet + 1;
                
                /* Parse address */
                while (*p && *p != ',' && *p != '#') {
                    addr = (addr << 4) | gdb_hex_to_nibble(*p++);
                }
                if (*p == ',') p++;
                
                /* Parse length */
                while (*p && *p != ':' && *p != '#') {
                    len = (len << 4) | gdb_hex_to_nibble(*p++);
                }
                if (*p == ':') p++;
                
                /* Write memory */
                if (addr + len <= AURORA_VM_MEMORY_SIZE) {
                    for (uint32_t i = 0; i < len && *p && *(p+1); i++) {
                        vm->memory[addr + i] = (gdb_hex_to_nibble(*p) << 4) | gdb_hex_to_nibble(*(p+1));
                        p += 2;
                    }
                    platform_strncpy(response, "OK", resp_size);
                } else {
                    platform_strncpy(response, "E01", resp_size);
                }
            }
            break;
            
        case 'c':
            /* Continue execution */
            vm->cpu.halted = false;
            vm->debugger.single_step = false;
            platform_strncpy(response, "OK", resp_size);
            break;
            
        case 's':
            /* Single step */
            vm->cpu.halted = false;
            vm->debugger.single_step = true;
            aurora_vm_step(vm);
            vm->cpu.halted = true;
            platform_strncpy(response, "S05", resp_size);  /* SIGTRAP */
            break;
            
        case 'Z':
            /* Set breakpoint: Z0,addr,kind */
            {
                if (packet[1] == '0') {  /* Software breakpoint */
                    const char *p = packet + 3;
                    uint32_t addr = 0;
                    while (*p && *p != ',' && *p != '#') {
                        addr = (addr << 4) | gdb_hex_to_nibble(*p++);
                    }
                    
                    if (aurora_vm_debugger_add_breakpoint(vm, addr) == 0) {
                        platform_strncpy(response, "OK", resp_size);
                    } else {
                        platform_strncpy(response, "E01", resp_size);
                    }
                } else {
                    platform_strncpy(response, "", resp_size);  /* Unsupported */
                }
            }
            break;
            
        case 'z':
            /* Clear breakpoint: z0,addr,kind */
            {
                if (packet[1] == '0') {
                    const char *p = packet + 3;
                    uint32_t addr = 0;
                    while (*p && *p != ',' && *p != '#') {
                        addr = (addr << 4) | gdb_hex_to_nibble(*p++);
                    }
                    
                    if (aurora_vm_debugger_remove_breakpoint(vm, addr) == 0) {
                        platform_strncpy(response, "OK", resp_size);
                    } else {
                        platform_strncpy(response, "E01", resp_size);
                    }
                } else {
                    platform_strncpy(response, "", resp_size);
                }
            }
            break;
            
        case 'q':
            /* Query commands */
            if (platform_strncmp(packet + 1, "Supported", 9) == 0) {
                platform_strncpy(response, "PacketSize=1000", resp_size);
            } else if (platform_strncmp(packet + 1, "Attached", 8) == 0) {
                platform_strncpy(response, "1", resp_size);  /* Attached to existing process */
            } else {
                platform_strncpy(response, "", resp_size);  /* Empty = not supported */
            }
            break;
            
        case 'k':
            /* Kill request */
            vm->cpu.halted = true;
            vm->gdb.enabled = false;
            platform_strncpy(response, "OK", resp_size);
            break;
            
        default:
            /* Unknown command - return empty response */
            platform_strncpy(response, "", resp_size);
            break;
    }
    
    return 0;
}

int aurora_vm_gdb_start(AuroraVM *vm, int port) {
    if (!vm) return -1;
    
    /* Initialize GDB server state */
    vm->gdb.enabled = true;
    vm->gdb.connected = false;
    vm->gdb.socket_fd = port;  /* Store port for reference (would be actual socket) */
    vm->gdb.break_requested = false;
    
    /* Halt VM initially so GDB can inspect state */
    vm->cpu.halted = true;
    
    return 0;
}

void aurora_vm_gdb_stop(AuroraVM *vm) {
    if (!vm) return;
    
    vm->gdb.enabled = false;
    vm->gdb.connected = false;
    vm->gdb.socket_fd = -1;
    vm->cpu.halted = false;  /* Resume execution when GDB disconnects */
}

int aurora_vm_gdb_handle(AuroraVM *vm) {
    if (!vm || !vm->gdb.enabled) return -1;
    
    /* Check for break request */
    if (vm->gdb.break_requested) {
        vm->cpu.halted = true;
        vm->gdb.break_requested = false;
        return 1;  /* Break */
    }
    
    /* In a real implementation, this would:
     * 1. Check socket for incoming data
     * 2. Parse GDB RSP packet
     * 3. Handle command using gdb_handle_packet()
     * 4. Send response back
     * 
     * For now, we provide the infrastructure via gdb_handle_packet()
     * which can be called with received packet data.
     */
    
    return 0;
}

/* Process a raw GDB RSP packet and generate response */
int aurora_vm_gdb_process_packet(AuroraVM *vm, const char *packet, 
                                  char *response, size_t response_size) {
    if (!vm || !vm->gdb.enabled || !packet || !response) return -1;
    
    char raw_response[512];
    
    /* Handle the GDB command */
    if (gdb_handle_packet(vm, packet, raw_response, sizeof(raw_response)) < 0) {
        return -1;
    }
    
    /* Format as proper RSP packet */
    return gdb_format_packet(raw_response, response, response_size);
}
