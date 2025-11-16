/**
 * @file aurora_vm.c
 * @brief Aurora OS Virtual Machine Implementation
 */

#include "../../include/platform/aurora_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
 * Free memory (no-op for bump allocator - no free list support)
 */
static void heap_free(aurora_heap_t *heap, uint32_t addr) {
    /* Bump allocator doesn't support freeing individual allocations */
    /* This is intentionally a no-op as per simplified requirements */
    (void)heap;
    (void)addr;
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
            
            for (uint32_t i = 0; i < len; i++) {
                putchar(vm->memory[addr + i]);
            }
            fflush(stdout);
            
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
            
            uint32_t len = 0;
            while (len < max_len) {
                int c = getchar();
                if (c == EOF || c == '\n') break;
                vm->memory[addr + len++] = (uint8_t)c;
            }
            
            vm->cpu.registers[0] = len;
            return 0;
        }
        
        case AURORA_SYSCALL_GET_TIME: {
            vm->cpu.registers[0] = (uint32_t)vm->timer.ticks;
            return 0;
        }
        
        case AURORA_SYSCALL_SLEEP: {
            uint32_t milliseconds = vm->cpu.registers[1];
            /* Simple simulation - just advance timer ticks */
            vm->timer.ticks += milliseconds;
            vm->debugger.cycle_count += milliseconds;
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
            /* No-op for bump allocator - doesn't support free */
            heap_free(&vm->heap, addr);
            vm->cpu.registers[0] = 0;
            return 0;
        }
        
        case AURORA_SYSCALL_OPEN:
        case AURORA_SYSCALL_CLOSE:
        case AURORA_SYSCALL_READ_FILE:
        case AURORA_SYSCALL_WRITE_FILE:
            /* File I/O syscalls - stub implementation (not fully implemented) */
            vm->cpu.registers[0] = (uint32_t)-1;
            return 0;
        
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
    uint32_t result, operand1, operand2;
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
            
        default:
            /* Invalid opcode */
            return -1;
    }
    
    return 0;
}

/* ===== VM API Implementation ===== */

AuroraVM *aurora_vm_create(void) {
    AuroraVM *vm = (AuroraVM *)calloc(1, sizeof(AuroraVM));
    if (!vm) return NULL;
    
    return vm;
}

int aurora_vm_init(AuroraVM *vm) {
    if (!vm) return -1;
    
    /* Initialize CPU */
    memset(&vm->cpu, 0, sizeof(aurora_cpu_t));
    vm->cpu.pc = 0;
    vm->cpu.sp = AURORA_VM_MEMORY_SIZE - 4;  /* Stack grows downward */
    vm->cpu.fp = vm->cpu.sp;
    vm->cpu.halted = false;
    
    /* Initialize memory - all pages invalid by default */
    memset(vm->memory, 0, AURORA_VM_MEMORY_SIZE);
    memset(vm->pages, 0, sizeof(vm->pages));
    
    /* Set up code section (first 16KB - read/execute) */
    for (uint32_t i = 0; i < 64; i++) {
        vm->pages[i].protection = AURORA_PAGE_READ | AURORA_PAGE_EXEC | AURORA_PAGE_PRESENT;
    }
    
    /* Set up heap section (next 32KB - read/write) - bump allocator */
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
    
    /* Initialize simple timer */
    vm->timer.ticks = 0;
    
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
    free(vm);
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
    
    memcpy(&vm->memory[addr], program, size);
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
    if (!check_memory_access(vm, addr, size, AURORA_PAGE_READ)) return -1;
    
    memcpy(buffer, &vm->memory[addr], size);
    return (int)size;
}

int aurora_vm_write_memory(AuroraVM *vm, uint32_t addr, size_t size, const void *buffer) {
    if (!vm || !buffer) return -1;
    if (!check_memory_access(vm, addr, size, AURORA_PAGE_WRITE)) return -1;
    
    memcpy(&vm->memory[addr], buffer, size);
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
            written = snprintf(buffer, buffer_size, "ADD r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_SUB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SUB r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_MUL:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "MUL r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_DIV:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "DIV r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_MOD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "MOD r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_NEG:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "NEG r%d, r%d", rd, rs1);
            break;
        case AURORA_OP_AND:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "AND r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_OR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "OR r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_XOR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "XOR r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_NOT:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "NOT r%d, r%d", rd, rs1);
            break;
        case AURORA_OP_SHL:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SHL r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_SHR:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SHR r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_LOAD:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "LOAD r%d, [r%d + r%d]", rd, rs1, rs2);
            break;
        case AURORA_OP_STORE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "STORE [r%d + r%d], r%d", rs1, rs2, rd);
            break;
        case AURORA_OP_LOADI:
            decode_i_type(instruction, &rd, &imm16);
            written = snprintf(buffer, buffer_size, "LOADI r%d, %d", rd, imm16);
            break;
        case AURORA_OP_LOADB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "LOADB r%d, [r%d + r%d]", rd, rs1, rs2);
            break;
        case AURORA_OP_STOREB:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "STOREB [r%d + r%d], r%d", rs1, rs2, rd);
            break;
        case AURORA_OP_MOVE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "MOVE r%d, r%d", rd, rs1);
            break;
        case AURORA_OP_CMP:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "CMP r%d, r%d", rs1, rs2);
            break;
        case AURORA_OP_TEST:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "TEST r%d, r%d", rs1, rs2);
            break;
        case AURORA_OP_SLT:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SLT r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_SLE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SLE r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_SEQ:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SEQ r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_SNE:
            decode_r_type(instruction, &rd, &rs1, &rs2);
            written = snprintf(buffer, buffer_size, "SNE r%d, r%d, r%d", rd, rs1, rs2);
            break;
        case AURORA_OP_JMP:
            decode_j_type(instruction, &imm32);
            written = snprintf(buffer, buffer_size, "JMP 0x%X", imm32);
            break;
        case AURORA_OP_JZ:
            decode_j_type(instruction, &imm32);
            written = snprintf(buffer, buffer_size, "JZ 0x%X", imm32);
            break;
        case AURORA_OP_JNZ:
            decode_j_type(instruction, &imm32);
            written = snprintf(buffer, buffer_size, "JNZ 0x%X", imm32);
            break;
        case AURORA_OP_JC:
            decode_j_type(instruction, &imm32);
            written = snprintf(buffer, buffer_size, "JC 0x%X", imm32);
            break;
        case AURORA_OP_JNC:
            decode_j_type(instruction, &imm32);
            written = snprintf(buffer, buffer_size, "JNC 0x%X", imm32);
            break;
        case AURORA_OP_CALL:
            decode_j_type(instruction, &imm32);
            written = snprintf(buffer, buffer_size, "CALL 0x%X", imm32);
            break;
        case AURORA_OP_RET:
            written = snprintf(buffer, buffer_size, "RET");
            break;
        case AURORA_OP_SYSCALL:
            written = snprintf(buffer, buffer_size, "SYSCALL");
            break;
        case AURORA_OP_HALT:
            written = snprintf(buffer, buffer_size, "HALT");
            break;
        default:
            written = snprintf(buffer, buffer_size, "UNKNOWN (0x%08X)", instruction);
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

/* ===== Timer API Implementation ===== */

uint64_t aurora_vm_timer_get_ticks(const AuroraVM *vm) {
    if (!vm) return 0;
    return vm->timer.ticks;
}

void aurora_vm_timer_advance(AuroraVM *vm, uint64_t ticks) {
    if (!vm) return;
    vm->timer.ticks += ticks;
}
