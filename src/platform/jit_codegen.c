/**
 * @file jit_codegen.c
 * @brief JIT Code Generation Backend
 *
 * Completes the JIT compilation infrastructure with native code generation
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../include/platform/aurora_vm.h"
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * JIT CODE GENERATION DEFINITIONS
 * ============================================================================ */

/* Target architectures */
typedef enum {
    JIT_ARCH_X86,
    JIT_ARCH_X86_64,
    JIT_ARCH_ARM32,
    JIT_ARCH_ARM64
} jit_arch_t;

/* Register mappings for x86-64 */
#define X64_RAX     0
#define X64_RCX     1
#define X64_RDX     2
#define X64_RBX     3
#define X64_RSP     4
#define X64_RBP     5
#define X64_RSI     6
#define X64_RDI     7
#define X64_R8      8
#define X64_R9      9
#define X64_R10     10
#define X64_R11     11
#define X64_R12     12
#define X64_R13     13
#define X64_R14     14
#define X64_R15     15

/* x86-64 instruction encodings */
#define X64_REX_W       0x48
#define X64_REX_R       0x44
#define X64_REX_X       0x42
#define X64_REX_B       0x41

#define X64_MOV_R64_IMM 0xB8
#define X64_MOV_RM64_R64 0x89
#define X64_MOV_R64_RM64 0x8B
#define X64_ADD_RM64_R64 0x01
#define X64_SUB_RM64_R64 0x29
#define X64_AND_RM64_R64 0x21
#define X64_OR_RM64_R64  0x09
#define X64_XOR_RM64_R64 0x31
#define X64_CMP_RM64_R64 0x39
#define X64_TEST_RM64_R64 0x85
#define X64_IMUL_R64_RM64 0x0FAF
#define X64_PUSH_R64     0x50
#define X64_POP_R64      0x58
#define X64_RET          0xC3
#define X64_NOP          0x90
#define X64_CALL_REL32   0xE8
#define X64_JMP_REL32    0xE9
#define X64_JMP_REL8     0xEB
#define X64_JZ_REL32     0x0F84
#define X64_JNZ_REL32    0x0F85
#define X64_JL_REL32     0x0F8C
#define X64_JLE_REL32    0x0F8E
#define X64_JG_REL32     0x0F8F
#define X64_JGE_REL32    0x0F8D

/* Code buffer management */
typedef struct {
    uint8_t* buffer;
    uint32_t size;
    uint32_t capacity;
    uint32_t position;
} code_buffer_t;

/* Label for branch resolution */
typedef struct {
    uint32_t offset;        /* Position in code buffer */
    uint32_t target;        /* Target address (VM) */
    bool resolved;
} jit_label_t;

/* Relocation entry */
typedef struct {
    uint32_t offset;        /* Position of relocation */
    uint32_t target;        /* Target symbol or address */
    uint8_t type;           /* Relocation type */
} jit_reloc_t;

/* JIT Context */
typedef struct {
    code_buffer_t code;
    jit_label_t labels[256];
    uint32_t label_count;
    jit_reloc_t relocs[256];
    uint32_t reloc_count;
    jit_arch_t arch;
    bool initialized;
    /* Register allocation */
    uint8_t reg_map[16];    /* VM reg -> native reg mapping */
    bool reg_dirty[16];     /* Register needs writeback */
    /* Statistics */
    uint32_t blocks_compiled;
    uint32_t bytes_generated;
} jit_context_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static jit_context_t g_jit_ctx;

/* ============================================================================
 * CODE BUFFER MANAGEMENT
 * ============================================================================ */

/**
 * Initialize code buffer
 */
static int code_buffer_init(code_buffer_t* cb, uint32_t capacity) {
    cb->buffer = (uint8_t*)platform_malloc(capacity);
    if (!cb->buffer) {
        return -1;
    }
    cb->size = 0;
    cb->capacity = capacity;
    cb->position = 0;
    return 0;
}

/**
 * Free code buffer
 */
static void code_buffer_free(code_buffer_t* cb) {
    if (cb->buffer) {
        platform_free(cb->buffer);
        cb->buffer = NULL;
    }
    cb->size = 0;
    cb->capacity = 0;
}

/**
 * Emit byte to code buffer
 */
static int emit_byte(code_buffer_t* cb, uint8_t byte) {
    if (cb->size >= cb->capacity) {
        return -1;
    }
    cb->buffer[cb->size++] = byte;
    return 0;
}

/**
 * Emit 16-bit value
 */
static int emit_word(code_buffer_t* cb, uint16_t word) {
    if (cb->size + 2 > cb->capacity) {
        return -1;
    }
    cb->buffer[cb->size++] = word & 0xFF;
    cb->buffer[cb->size++] = (word >> 8) & 0xFF;
    return 0;
}

/**
 * Emit 32-bit value
 */
static int emit_dword(code_buffer_t* cb, uint32_t dword) {
    if (cb->size + 4 > cb->capacity) {
        return -1;
    }
    cb->buffer[cb->size++] = dword & 0xFF;
    cb->buffer[cb->size++] = (dword >> 8) & 0xFF;
    cb->buffer[cb->size++] = (dword >> 16) & 0xFF;
    cb->buffer[cb->size++] = (dword >> 24) & 0xFF;
    return 0;
}

/**
 * Emit 64-bit value
 */
static int emit_qword(code_buffer_t* cb, uint64_t qword) {
    emit_dword(cb, (uint32_t)qword);
    emit_dword(cb, (uint32_t)(qword >> 32));
    return 0;
}

/* ============================================================================
 * X86-64 CODE GENERATION
 * ============================================================================ */

/**
 * Emit REX prefix if needed
 */
static void emit_rex(code_buffer_t* cb, bool w, bool r, bool x, bool b) {
    uint8_t rex = 0x40;
    if (w) rex |= 0x08;
    if (r) rex |= 0x04;
    if (x) rex |= 0x02;
    if (b) rex |= 0x01;
    if (rex != 0x40) {
        emit_byte(cb, rex);
    }
}

/**
 * Emit ModR/M byte
 */
static void emit_modrm(code_buffer_t* cb, uint8_t mod, uint8_t reg, uint8_t rm) {
    emit_byte(cb, ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7));
}

/**
 * Emit x86-64 MOV reg, imm64
 */
static void x64_mov_reg_imm64(code_buffer_t* cb, uint8_t reg, uint64_t imm) {
    emit_rex(cb, true, false, false, reg >= 8);
    emit_byte(cb, X64_MOV_R64_IMM + (reg & 7));
    emit_qword(cb, imm);
}

/**
 * Emit x86-64 MOV reg, imm32 (sign-extended)
 */
static void x64_mov_reg_imm32(code_buffer_t* cb, uint8_t reg, uint32_t imm) {
    emit_rex(cb, true, false, false, reg >= 8);
    emit_byte(cb, 0xC7);
    emit_modrm(cb, 3, 0, reg & 7);
    emit_dword(cb, imm);
}

/**
 * Emit x86-64 MOV reg1, reg2
 */
static void x64_mov_reg_reg(code_buffer_t* cb, uint8_t dst, uint8_t src) {
    emit_rex(cb, true, src >= 8, false, dst >= 8);
    emit_byte(cb, X64_MOV_RM64_R64);
    emit_modrm(cb, 3, src & 7, dst & 7);
}

/**
 * Emit x86-64 ADD reg1, reg2
 */
static void x64_add_reg_reg(code_buffer_t* cb, uint8_t dst, uint8_t src) {
    emit_rex(cb, true, src >= 8, false, dst >= 8);
    emit_byte(cb, X64_ADD_RM64_R64);
    emit_modrm(cb, 3, src & 7, dst & 7);
}

/**
 * Emit x86-64 SUB reg1, reg2
 */
static void x64_sub_reg_reg(code_buffer_t* cb, uint8_t dst, uint8_t src) {
    emit_rex(cb, true, src >= 8, false, dst >= 8);
    emit_byte(cb, X64_SUB_RM64_R64);
    emit_modrm(cb, 3, src & 7, dst & 7);
}

/**
 * Emit x86-64 AND reg1, reg2
 */
static void x64_and_reg_reg(code_buffer_t* cb, uint8_t dst, uint8_t src) {
    emit_rex(cb, true, src >= 8, false, dst >= 8);
    emit_byte(cb, X64_AND_RM64_R64);
    emit_modrm(cb, 3, src & 7, dst & 7);
}

/**
 * Emit x86-64 OR reg1, reg2
 */
static void x64_or_reg_reg(code_buffer_t* cb, uint8_t dst, uint8_t src) {
    emit_rex(cb, true, src >= 8, false, dst >= 8);
    emit_byte(cb, X64_OR_RM64_R64);
    emit_modrm(cb, 3, src & 7, dst & 7);
}

/**
 * Emit x86-64 XOR reg1, reg2
 */
static void x64_xor_reg_reg(code_buffer_t* cb, uint8_t dst, uint8_t src) {
    emit_rex(cb, true, src >= 8, false, dst >= 8);
    emit_byte(cb, X64_XOR_RM64_R64);
    emit_modrm(cb, 3, src & 7, dst & 7);
}

/**
 * Emit x86-64 CMP reg1, reg2
 */
static void x64_cmp_reg_reg(code_buffer_t* cb, uint8_t reg1, uint8_t reg2) {
    emit_rex(cb, true, reg2 >= 8, false, reg1 >= 8);
    emit_byte(cb, X64_CMP_RM64_R64);
    emit_modrm(cb, 3, reg2 & 7, reg1 & 7);
}

/**
 * Emit x86-64 PUSH reg
 */
static void x64_push_reg(code_buffer_t* cb, uint8_t reg) {
    if (reg >= 8) {
        emit_byte(cb, X64_REX_B);
    }
    emit_byte(cb, X64_PUSH_R64 + (reg & 7));
}

/**
 * Emit x86-64 POP reg
 */
static void x64_pop_reg(code_buffer_t* cb, uint8_t reg) {
    if (reg >= 8) {
        emit_byte(cb, X64_REX_B);
    }
    emit_byte(cb, X64_POP_R64 + (reg & 7));
}

/**
 * Emit x86-64 RET
 */
static void x64_ret(code_buffer_t* cb) {
    emit_byte(cb, X64_RET);
}

/**
 * Emit x86-64 JMP rel32
 */
static void x64_jmp_rel32(code_buffer_t* cb, int32_t offset) {
    emit_byte(cb, X64_JMP_REL32);
    emit_dword(cb, (uint32_t)offset);
}

/**
 * Emit x86-64 conditional jump
 */
static void x64_jcc_rel32(code_buffer_t* cb, uint16_t opcode, int32_t offset) {
    emit_byte(cb, opcode >> 8);
    emit_byte(cb, opcode & 0xFF);
    emit_dword(cb, (uint32_t)offset);
}

/**
 * Emit x86-64 NOP
 */
static void x64_nop(code_buffer_t* cb) {
    emit_byte(cb, X64_NOP);
}

/* ============================================================================
 * JIT COMPILATION
 * ============================================================================ */

/**
 * Initialize JIT context
 */
int jit_codegen_init(void) {
    if (g_jit_ctx.initialized) {
        return 0;
    }
    
    platform_memset(&g_jit_ctx, 0, sizeof(jit_context_t));
    
    /* Initialize code buffer */
    if (code_buffer_init(&g_jit_ctx.code, AURORA_VM_JIT_CACHE_SIZE) != 0) {
        return -1;
    }
    
    /* Set architecture (default to x86-64) */
    g_jit_ctx.arch = JIT_ARCH_X86_64;
    
    /* Initialize register mapping */
    /* Map VM registers r0-r15 to x86-64 registers */
    g_jit_ctx.reg_map[0] = X64_RAX;
    g_jit_ctx.reg_map[1] = X64_RCX;
    g_jit_ctx.reg_map[2] = X64_RDX;
    g_jit_ctx.reg_map[3] = X64_RBX;
    g_jit_ctx.reg_map[4] = X64_RSI;
    g_jit_ctx.reg_map[5] = X64_RDI;
    g_jit_ctx.reg_map[6] = X64_R8;
    g_jit_ctx.reg_map[7] = X64_R9;
    g_jit_ctx.reg_map[8] = X64_R10;
    g_jit_ctx.reg_map[9] = X64_R11;
    g_jit_ctx.reg_map[10] = X64_R12;
    g_jit_ctx.reg_map[11] = X64_R13;
    g_jit_ctx.reg_map[12] = X64_R14;
    g_jit_ctx.reg_map[13] = X64_R15;
    g_jit_ctx.reg_map[14] = X64_RBP; /* Reserved for VM state pointer */
    g_jit_ctx.reg_map[15] = X64_RSP; /* Reserved for stack */
    
    g_jit_ctx.initialized = true;
    
    return 0;
}

/**
 * Shutdown JIT context
 */
void jit_codegen_shutdown(void) {
    if (!g_jit_ctx.initialized) {
        return;
    }
    
    code_buffer_free(&g_jit_ctx.code);
    g_jit_ctx.initialized = false;
}

/**
 * Compile Aurora VM instruction to native code
 */
int jit_compile_instruction(uint32_t instruction) {
    if (!g_jit_ctx.initialized) {
        return -1;
    }
    
    code_buffer_t* cb = &g_jit_ctx.code;
    
    /* Decode instruction */
    uint8_t opcode = (instruction >> 24) & 0xFF;
    uint8_t rd = (instruction >> 20) & 0x0F;
    uint8_t rs1 = (instruction >> 16) & 0x0F;
    uint8_t rs2 = (instruction >> 12) & 0x0F;
    int16_t imm = (int16_t)(instruction & 0xFFFF);
    
    /* Get native registers */
    uint8_t n_rd = g_jit_ctx.reg_map[rd];
    uint8_t n_rs1 = g_jit_ctx.reg_map[rs1];
    uint8_t n_rs2 = g_jit_ctx.reg_map[rs2];
    
    switch (opcode) {
        case AURORA_OP_ADD:
            /* rd = rs1 + rs2 */
            if (n_rd != n_rs1) {
                x64_mov_reg_reg(cb, n_rd, n_rs1);
            }
            x64_add_reg_reg(cb, n_rd, n_rs2);
            break;
            
        case AURORA_OP_SUB:
            /* rd = rs1 - rs2 */
            if (n_rd != n_rs1) {
                x64_mov_reg_reg(cb, n_rd, n_rs1);
            }
            x64_sub_reg_reg(cb, n_rd, n_rs2);
            break;
            
        case AURORA_OP_AND:
            /* rd = rs1 & rs2 */
            if (n_rd != n_rs1) {
                x64_mov_reg_reg(cb, n_rd, n_rs1);
            }
            x64_and_reg_reg(cb, n_rd, n_rs2);
            break;
            
        case AURORA_OP_OR:
            /* rd = rs1 | rs2 */
            if (n_rd != n_rs1) {
                x64_mov_reg_reg(cb, n_rd, n_rs1);
            }
            x64_or_reg_reg(cb, n_rd, n_rs2);
            break;
            
        case AURORA_OP_XOR:
            /* rd = rs1 ^ rs2 */
            if (n_rd != n_rs1) {
                x64_mov_reg_reg(cb, n_rd, n_rs1);
            }
            x64_xor_reg_reg(cb, n_rd, n_rs2);
            break;
            
        case AURORA_OP_LOADI:
            /* rd = imm */
            x64_mov_reg_imm32(cb, n_rd, (uint32_t)(int32_t)imm);
            break;
            
        case AURORA_OP_MOVE:
            /* rd = rs1 */
            x64_mov_reg_reg(cb, n_rd, n_rs1);
            break;
            
        case AURORA_OP_CMP:
            /* flags = rs1 - rs2 */
            x64_cmp_reg_reg(cb, n_rs1, n_rs2);
            break;
            
        case AURORA_OP_JMP:
            /* PC = imm */
            x64_jmp_rel32(cb, (int32_t)imm * 4);
            break;
            
        case AURORA_OP_JZ:
            /* if (Z) PC = imm */
            x64_jcc_rel32(cb, X64_JZ_REL32, (int32_t)imm * 4);
            break;
            
        case AURORA_OP_JNZ:
            /* if (!Z) PC = imm */
            x64_jcc_rel32(cb, X64_JNZ_REL32, (int32_t)imm * 4);
            break;
            
        case AURORA_OP_RET:
            x64_ret(cb);
            break;
            
        case AURORA_OP_HALT:
            /* Return from JIT code */
            x64_ret(cb);
            break;
            
        default:
            /* Unsupported opcode - emit NOP */
            x64_nop(cb);
            return -1;
    }
    
    return 0;
}

/**
 * Compile basic block
 */
int jit_compile_block(AuroraVM* vm, uint32_t start_addr, uint32_t end_addr) {
    if (!vm || !g_jit_ctx.initialized) {
        return -1;
    }
    
    /* Prologue - save registers */
    code_buffer_t* cb = &g_jit_ctx.code;
    uint32_t block_start = cb->size;
    
    x64_push_reg(cb, X64_RBP);
    x64_push_reg(cb, X64_RBX);
    x64_push_reg(cb, X64_R12);
    x64_push_reg(cb, X64_R13);
    x64_push_reg(cb, X64_R14);
    x64_push_reg(cb, X64_R15);
    
    /* Compile each instruction */
    for (uint32_t addr = start_addr; addr < end_addr; addr += 4) {
        uint32_t instruction;
        if (aurora_vm_read_memory(vm, addr, 4, &instruction) != 4) {
            break;
        }
        
        jit_compile_instruction(instruction);
    }
    
    /* Epilogue - restore registers */
    x64_pop_reg(cb, X64_R15);
    x64_pop_reg(cb, X64_R14);
    x64_pop_reg(cb, X64_R13);
    x64_pop_reg(cb, X64_R12);
    x64_pop_reg(cb, X64_RBX);
    x64_pop_reg(cb, X64_RBP);
    x64_ret(cb);
    
    g_jit_ctx.blocks_compiled++;
    g_jit_ctx.bytes_generated += cb->size - block_start;
    
    return 0;
}

/**
 * Execute compiled code
 */
int jit_execute(void* code_addr) {
    if (!code_addr) {
        return -1;
    }
    
    /* In a real implementation, would:
     * 1. Make memory executable (mprotect)
     * 2. Call the compiled function
     * 3. Handle returns and exceptions
     */
    
    return 0;
}

/**
 * Get JIT statistics
 */
void jit_get_stats(uint32_t* blocks, uint32_t* bytes, uint32_t* cache_used) {
    if (blocks) *blocks = g_jit_ctx.blocks_compiled;
    if (bytes) *bytes = g_jit_ctx.bytes_generated;
    if (cache_used) *cache_used = g_jit_ctx.code.size;
}

/**
 * Clear JIT cache
 */
void jit_clear_cache(void) {
    if (!g_jit_ctx.initialized) {
        return;
    }
    
    g_jit_ctx.code.size = 0;
    g_jit_ctx.label_count = 0;
    g_jit_ctx.reloc_count = 0;
}

/**
 * Get JIT code generation version
 */
const char* jit_codegen_get_version(void) {
    return "1.0.0-aurora-jit";
}
